/*                                                                 --*- C++ -*-
  Copyright (C) 2013 Changwoo Min. All Rights Reserved.

  This file is part of DANBI project. 

  NOTICE: All information contained herein is, and remains the property 
  of Changwoo Min. The intellectual and technical concepts contained 
  herein are proprietary to Changwoo Min and may be covered  by patents 
  or patents in process, and are protected by trade secret or copyright law. 
  Dissemination of this information or reproduction of this material is 
  strictly forbidden unless prior written permission is obtained 
  from Changwoo Min(multics69@gmail.com). 

  mCuBoQ2.cpp -- micro-benchmark for CuBoQ 
                 for comparison with the work of Morrison and Afek, PPoPP 2013. 
 */
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <cerrno>
#include <list>
#include <algorithm>
#include <pthread.h>
#include <sys/time.h>
#include <papi.h>
#include "Support/CuBoQueue.h"
#include "Support/AbstractRunnable.h"
#include "Support/Thread.h"
#include "Support/Machine.h"
#include "Support/MachineConfig.h"
#include "Support/FKMemPool.h"
#include "Support/Random.h"
#include "Support/CLHLock.h"

using namespace danbi; 

//#define _TRACK_CPU_COUNTERS

#define MAX_THREADS 128
//#define POOL_SIZE 4096
#define POOL_SIZE (BP->per_thread_initelms + BP->per_thread_runs)
#define PAPI_FAILURE -10

static pthread_barrier_t Barr;
static unsigned long long ExecTimeInMicroSec; 
static struct timeval StartTime, StopTime;
static CLHLock PAPILock(MAX_THREADS);

struct ThreadExecStat {
  int PopRound, PopOps, PushOps; 
  int CASTail;
  int CASOps, FASOps;
  int WaitCount;
  unsigned long long PushClocks, PopClocks, PopCombineClocks; 
  unsigned long long WaitClocks, WaitMaxClocks;
};
static struct ThreadExecStat ThreadExecStatArray[MAX_THREADS] __cacheline_aligned;
static __thread int cpu_events = PAPI_NULL;
static long long cpu_values[MAX_THREADS][4] __cacheline_aligned;

struct BenchmarkParam {
  int initelms; 
  int per_thread_initelms; 
  int thread; 
  int partition;
  long total_runs; 
  long per_thread_runs;
  char work_type;
  int max_work; 
};

class Element {
  DANBI_CUBOQ_ITERABLE(Element); 
public:
  int value; 
} __cacheline_aligned; 

static void init_cpu_counters(void) {
#ifdef _TRACK_CPU_COUNTERS
  unsigned long int tid;

  if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
    exit(PAPI_FAILURE);
#endif
}
 
static void start_cpu_counters(int id) {
#ifdef _TRACK_CPU_COUNTERS
  PAPILock.lock(id);
  if (PAPI_create_eventset(&cpu_events) != PAPI_OK) {
    fprintf(stderr, "PAPI ERROR: unable to initialize performance counters\n");
    exit(PAPI_FAILURE);
  }
  if (PAPI_add_event(cpu_events, PAPI_L1_DCM) != PAPI_OK) {
    fprintf(stderr, "PAPI ERROR: unable to create event for L1 data cache misses\n");
    exit(PAPI_FAILURE);
  }
  if (PAPI_add_event(cpu_events, PAPI_L2_DCM) != PAPI_OK) {
    fprintf(stderr, "PAPI ERROR: unable to create event for L2 data cache misses\n");
    exit(PAPI_FAILURE);
  }
  if (PAPI_add_event(cpu_events, 0x40000011) != PAPI_OK) {
    fprintf(stderr, "PAPI ERROR: unable to create event for L3 cache misses\n");
    exit(PAPI_FAILURE);
  }
  if (PAPI_start(cpu_events) != PAPI_OK) {
    fprintf(stderr, "PAPI ERROR: unable to start performance counters\n");
    exit(PAPI_FAILURE);
  }
  PAPILock.unlock(id);
#endif
}

static void stop_cpu_counters(int id) {
#ifdef _TRACK_CPU_COUNTERS
  PAPILock.lock(id);
  if (PAPI_read(cpu_events, cpu_values[id]) != PAPI_OK) {
    fprintf(stderr, "PAPI ERROR: unable to read counters\n");
    exit(PAPI_FAILURE);
  }
  if (PAPI_stop(cpu_events, cpu_values[id]) != PAPI_OK) {
    fprintf(stderr, "PAPI ERROR: unable to stop counters\n");
    exit(PAPI_FAILURE);
  }
  PAPILock.unlock(id);
#endif
}

class mCuBoQWorker: public Thread, private AbstractRunnable {
private:
  volatile bool& Running;
  BenchmarkParam* BP; 
  CuBoQueue<Element>& Q; 

  void main() {
    CuBoQueue<Element>::Statistics LocalStat;
    Element* E;
    int id = Thread::getID();
    int cpuid = Thread::getCurrent()->getAffinity();
    cpu_set_t mask;

    // Pinning the thread. 
    CPU_ZERO(&mask);
    CPU_SET(cpuid, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
      printf("Could not set CPU affinity: %d\n", cpuid);
      exit(-1);
    }
    Q.registerThread(id); 

    // Alloc mempool
    FKMemPool *mempool= new FKMemPool(sizeof(Element), POOL_SIZE, true); 

    // Push initial elements
    for (long i = 0; i < BP->per_thread_initelms; ++i) {
      E = (Element*)mempool->malloc();
      Q.push(E, id); 
    }
    Machine::mb();

    // Wait for barrier
    pthread_barrier_wait(&Barr);
    start_cpu_counters(id);
    if (id == BP->thread) 
      ::gettimeofday(&StartTime, NULL); 
      
    // Work 
    switch (BP->work_type) {
    case 'p': {
      for (long i = 0; i < BP->per_thread_runs; ++i)  {
        int rnum;
        volatile int j; 

        E = (Element*)mempool->malloc();
        E->value = id; 
        Q.push(E, id); 
        rnum = Random::randomIntRange(1, BP->max_work); 
        for(j = 0; j < rnum; ++j) ; 

        if (!Q.pop(E, id)) 
          mempool->free(E);
        rnum = Random::randomIntRange(1, BP->max_work); 
        for(j = 0; j < rnum; ++j) ; 
      }
    }
      break; 

    case '5': {
      long per_thread_ops = BP->per_thread_runs * 2; 
      for (long i = 0; i < per_thread_ops; ++i)  {
        int rnum;
        volatile int j; 
        switch (Random::randomBool()) {
        case true: {
          E = (Element*)mempool->malloc();
          E->value = id; 
          Q.push(E, id); 
          rnum = Random::randomIntRange(1, BP->max_work); 
          for(j = 0; j < rnum; ++j) ; 
        }
          break;
          
        case false: {
          if (!Q.pop(E, id)) 
            mempool->free(E);
          rnum = Random::randomIntRange(1, BP->max_work); 
          for(j = 0; j < rnum; ++j) ; 
        }
          break;
        }
      }
    }
      break; 
    }
    stop_cpu_counters(id);

    // Stat
    Q.getStat(LocalStat);
    ThreadExecStatArray[id].PopRound = LocalStat.PopRound; 
    ThreadExecStatArray[id].PopOps = LocalStat.PopOps; 
    ThreadExecStatArray[id].PushOps = LocalStat.PushOps; 
    ThreadExecStatArray[id].CASTail = LocalStat.CASTail; 
    ThreadExecStatArray[id].WaitCount = LocalStat.WaitCount;
    ThreadExecStatArray[id].PushClocks = LocalStat.PushClocks; 
    ThreadExecStatArray[id].PopClocks = LocalStat.PopClocks; 
    ThreadExecStatArray[id].PopCombineClocks = LocalStat.PopCombineClocks; 
    ThreadExecStatArray[id].WaitClocks = LocalStat.WaitClocks;
    ThreadExecStatArray[id].WaitMaxClocks = LocalStat.WaitMaxClocks;
  }

public:
  mCuBoQWorker(volatile bool& Running_, BenchmarkParam* BP_, 
               CuBoQueue<Element>& Q_)
    : Thread(static_cast<AbstractRunnable*>(this)[0]), 
      Running(Running_), BP(BP_), Q(Q_) {}
};

int benchmarkMain(BenchmarkParam *BP) {
  // Create a queue
  void * AlignedMem = ::memalign(Machine::CachelineSize, sizeof(CuBoQueue<Element>));
  CuBoQueue<Element>* Q = new(AlignedMem) CuBoQueue<Element>(BP->partition); 
  if (!Q)  return -ENOMEM; 

  // Barrier init. 
  if (pthread_barrier_init(&Barr, NULL, BP->thread)) {
    printf("Fail to init barrier\n"); 
    return -1; 
  }

  // Create a thread
  volatile bool Running = false; 
  Machine::mb(); 
  int Cores = MachineConfig::getInstance().getNumHardwareThreadsInCPUs();
  std::list<mCuBoQWorker*> Workers; 
  for (int i = 0; i < BP->thread; ++i) {
    mCuBoQWorker* Worker = new mCuBoQWorker(Running, BP, *Q); 
    if (!Worker) return -ENOMEM; 
    Workers.push_back(Worker); 
    Worker->setAffinity(1, i % Cores); 
    Worker->start(); 
  }

  // Wait until the end 
  for(std::list<mCuBoQWorker*>::iterator 
        i = Workers.begin(), e = Workers.end(); i != e; ++i)
    (*i)->join();
  ::gettimeofday(&StopTime, NULL);
  ExecTimeInMicroSec = ((StopTime.tv_sec - StartTime.tv_sec) * 1000000) +
    (StopTime.tv_usec - StartTime.tv_usec); 

  printf("# msec: %lld\n", ExecTimeInMicroSec/1000);
  
  // Clean up 
  Q->~CuBoQueue<Element>();
  ::free(AlignedMem);
  return 0; 
}

static 
void showUsage(void) {
  ::fprintf(stderr, 
            "DANBI micro-benchmark for Super Scalable Queue with Permanent Sentinel Node"); 
  ::fprintf(stderr, 
            "  Usage: mCuBoQRun \n"); 
  ::fprintf(stderr, 
            "      [--partition number_of_partition]\n"); 
  ::fprintf(stderr, 
            "      [--runs number_of_total_runs]\n"); 
  ::fprintf(stderr, 
            "      [--initelms initial_number_of_elements]\n");
  ::fprintf(stderr, 
            "      [--type {pairs | 50}]\n");
  ::fprintf(stderr, 
            "      [--maxwork maxwork_count]\n");
  ::fprintf(stderr, 
            "      [--thread number_of_thread]\n"); 
}

static
void showResult(BenchmarkParam *BP) {
  double MOpsPerSec = (double)(BP->total_runs * 2.0)/(double)ExecTimeInMicroSec;
  ThreadExecStat Stat; 
  int TotalRuns = BP->thread * BP->per_thread_runs;
  long long total_cpu_values[4];
  int TotalAtomicOps;

  // Reduce the collected per thread statistics
  ::memset(&Stat, 0, sizeof(Stat));
  for (int i = 0; i <= BP->thread; ++i) {
    Stat.PopRound += ThreadExecStatArray[i].PopRound;
    Stat.PopOps += ThreadExecStatArray[i].PopOps;
    Stat.PushOps += ThreadExecStatArray[i].PushOps;
    Stat.CASTail += ThreadExecStatArray[i].CASTail;
    Stat.WaitCount += ThreadExecStatArray[i].WaitCount;
    Stat.PushClocks += ThreadExecStatArray[i].PushClocks;
    Stat.PopClocks += ThreadExecStatArray[i].PopClocks;
    Stat.PopCombineClocks += ThreadExecStatArray[i].PopCombineClocks;
    Stat.WaitClocks += ThreadExecStatArray[i].WaitClocks;
    if (Stat.WaitMaxClocks < ThreadExecStatArray[i].WaitClocks)
      Stat.WaitMaxClocks = ThreadExecStatArray[i].WaitClocks;
  }
  Stat.CASOps = Stat.CASTail;
  Stat.FASOps = Stat.PushOps + Stat.PopOps + Stat.PopRound;

  for (int j = 0; j < 4; j++) {
    total_cpu_values[j] = 0;
    for (int i = 0; i < BP->thread; i++)
      total_cpu_values[j] += cpu_values[i][j];
  }
  TotalAtomicOps = Stat.CASOps + Stat.FASOps + Stat.CASTail;

  ::fprintf(stdout, 
            "# DANBI CuBoQ microbenchmark\n");
  ::fprintf(stdout, 
            "# initelms: %d total_runs: %ld partiton: %d worktype: %c maxwork: %d\n", 
            BP->initelms, 
            BP->total_runs, 
            BP->partition, 
            BP->work_type, 
            BP->max_work);
  ::fprintf(stdout, 
            "# Stat.PushOps = %d Stat.PopOps = %d Stat.PopRound = %d\n", 
            Stat.PushOps, Stat.PopOps, Stat.PopRound);
  ::fprintf(stdout, 
            "# threads(1) MOpsPerSec(2) AvgCombining(3) AtomicPerOp(4) "
            " L1D$Miss(5) L2D$Miss(6) L3$Miss(7) "
            " CASPerOp(8) FASPerOp(9)"
            " CASTailRatioPerPop(10)"
            " PuchUSec(11)"
            " PopUSec(12)"
            " PopCombineUSec(13)"
            " WaitingCountPerMOps(14) PercentOfWaitingTime(15)"
            " \n");
  ::fprintf(stdout, 
            "%d %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.8f %.8f\n", 
            BP->thread, 
            MOpsPerSec, 
            double(Stat.PushOps + Stat.PopOps)/double(Stat.PopRound),
            double(TotalAtomicOps)/double(Stat.PushOps + Stat.PopOps), // 4 
            double(total_cpu_values[0])/(Stat.PushOps + Stat.PopOps), 
            double(total_cpu_values[1])/(Stat.PushOps + Stat.PopOps), 
            double(total_cpu_values[2])/(Stat.PushOps + Stat.PopOps), // 7
            double(Stat.CASOps)/double(Stat.PushOps + Stat.PopOps),
            double(Stat.FASOps)/double(Stat.PushOps + Stat.PopOps), // 9
            double(Stat.CASTail)/double(Stat.PopOps), 
            Machine::cyclesToUSec(Stat.PushClocks), // 11
            Machine::cyclesToUSec(Stat.PopClocks), 
            Machine::cyclesToUSec(Stat.PopCombineClocks), 
            double(Stat.WaitCount * 1000000.0) / double(BP->total_runs * 2.0), // 14
            Machine::cyclesToUSec(Stat.WaitClocks) / (double(ExecTimeInMicroSec) * 100.0 * BP->thread));
}

static 
int parseOption(int argc, char *argv[], BenchmarkParam* BP) {
  struct option options[] = {
    {"initelms",   required_argument, 0, 'i'}, 
    {"thread",   required_argument, 0, 't'}, 
    {"runs",   required_argument, 0, 'r'}, 
    {"partition",   required_argument, 0, 'n'}, 
    {"type",   required_argument, 0, 'y'}, 
    {"maxwork",   required_argument, 0, 'x'}, 
    {0, 0, 0, 0}, 
  }; 

  // Set default values
  BP->initelms = 0;
  BP->thread = 2;
  BP->partition = 4;
  BP->total_runs = 10000000;
  BP->work_type = 'p';
  BP->max_work = 64;

  // Parse options
  do {
    int opt = getopt_long(argc, argv, "", options, NULL); 
    if (opt == -1) break;
    switch(opt) {
    case 'i':
      BP->initelms = atoi(optarg); 
      break;
    case 't':
      BP->thread = atoi(optarg); 
      break;
    case 'r':
      BP->total_runs = atoi(optarg); 
      break;
    case 'n':
      BP->partition = atoi(optarg); 
      break;
    case 'y':
      BP->work_type = optarg[0]; 
      break;
    case 'x':
      BP->max_work = atoi(optarg); 
      break;
    case '?':
    default:
      showUsage();
      return 1; 
    };
  } while(1); 
  BP->per_thread_initelms = BP->initelms / BP->thread; 
  BP->per_thread_runs = BP->total_runs / BP->thread;
  return 0; 
}

int main(int argc, char* argv[]) {
  BenchmarkParam BP; 
  int ret; 

  // Parse options 
  ret = parseOption(argc, argv, &BP);
  if (ret) return 1; 

  // init cpu counters
  init_cpu_counters();

  // Run benchmark
  ret = benchmarkMain(&BP);
  if (ret) return 2; 

  // Print out results
  showResult(&BP);

  return 0; 
}

