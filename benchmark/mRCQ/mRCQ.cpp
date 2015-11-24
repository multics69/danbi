/*                                                                 --*- C++ -*-
  Copyright (C) 2012 Changwoo Min. All Rights Reserved.

  This file is part of DANBI project. 

  NOTICE: All information contained herein is, and remains the property 
  of Changwoo Min. The intellectual and technical concepts contained 
  herein are proprietary to Changwoo Min and may be covered  by patents 
  or patents in process, and are protected by trade secret or copyright law. 
  Dissemination of this information or reproduction of this material is 
  strictly forbidden unless prior written permission is obtained 
  from Changwoo Min(multics69@gmail.com). 

  mRCQ.cpp -- micro-benchmark for reserve/commit queue 
 */
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <cerrno>
#include <list>
#include <algorithm>
#include "Core/QueueFactory.h"
#include "Support/AbstractRunnable.h"
#include "Support/Thread.h"
#include "Support/Machine.h"
#include "Support/MachineConfig.h"
#include "Support/PerformanceCounter.h"
#include "Support/Debug.h"

using namespace danbi; 

static unsigned long long HWCounters[PERF_COUNT_HW_MAX];
static unsigned long long ExecTimeInMicroSec; 
static unsigned long RunCount; 

struct BenchmarkParam {
  bool ticket; 
  bool memaccess; 
  bool pinning; 
  int runningsec; 
  int elmsize; 
  int qsize; 
  int reservesize; 
  int thread; 
};

class mRCQWorker: public Thread, private AbstractRunnable {
private:
  volatile bool& Running;
  BenchmarkParam* BP; 
  AbstractReserveCommitQueue& Q; 
  ReserveCommitQueueAccessor QAcc; 

  void main() {
    volatile char Buffer[BP->elmsize];
    
    // Until running flag is on 
    while (!Running) {
      Machine::mb(); 
    }
      
    // Work 
    unsigned long runcount; 
    while (Running) {
      ++runcount; 
      int Ticket; 
      // push 
      while (Q.reservePushN(BP->reservesize, QAcc.Win, QAcc.RevInfo, 0, &Ticket)) ; 
      if (BP->memaccess) {
        for (int j = 0; j < BP->reservesize; ++j) {
          char* Elm = (char *)QAcc.getElementAt(BP->elmsize, j); 
          ::memset(Elm, j, BP->elmsize); 
        }
      }
      while (Q.commitPush(QAcc.RevInfo)) ; 

      // pop
      while (Q.reservePopN(BP->reservesize, QAcc.Win, QAcc.RevInfo, Ticket, NULL)) ; 
      if (BP->memaccess) {
        for (int j = 0; j < BP->reservesize; ++j) {
          char* Elm = (char *)QAcc.getElementAt(BP->elmsize, j); 
          ::memcpy((char *)Buffer, Elm, BP->elmsize); 
        }
      }
      while (Q.commitPop(QAcc.RevInfo)) ; 
    }

    // Reduce run count 
    Machine::atomicWordAddReturn<unsigned long>(&RunCount, runcount); 
  }

public:
  mRCQWorker(volatile bool& Running_, 
             BenchmarkParam* BP_, AbstractReserveCommitQueue& Q_)
    : Thread(static_cast<AbstractRunnable*>(this)[0]), 
      Running(Running_), BP(BP_), Q(Q_){}
};

int benchmarkMain(BenchmarkParam *BP) {
  // Create a queue
  AbstractReserveCommitQueue* Q = QueueFactory::newQueue(
    BP->elmsize, BP->qsize, 
    BP->ticket, false, false, BP->ticket,
    false, false); 
  if (!Q)  return -ENOMEM; 

  // Create a thread
  volatile bool Running = false; 
  Machine::mb(); 
  int Cores = MachineConfig::getInstance().getNumHardwareThreadsInCPUs();
  std::list<mRCQWorker*> Workers; 
  for (int i = 0; i < BP->thread; ++i) {
    mRCQWorker* Worker = new mRCQWorker(Running, BP, *Q); 
    if (!Worker) return -ENOMEM; 
    Workers.push_back(Worker); 
    if (BP->pinning)
      Worker->setAffinity(1, i % Cores); 
    Worker->start(); 
  }

  // Kick 
  Running = true; 
  Machine::mb(); 

  // Wait until the end 
  PerformanceCounter::start(); 
  ::usleep(BP->runningsec*1000*1000);
  Running = false; 
  Machine::mb(); 
  for(std::list<mRCQWorker*>::iterator 
        i = Workers.begin(), e = Workers.end(); i != e; ++i)
    (*i)->join();
  ExecTimeInMicroSec = PerformanceCounter::stop(); 
  
  // Clean up 
  delete Q; 
  return 0; 
}

static 
void showUsage(void) {
  ::fprintf(stderr, 
            "DANBI micro-benchmark for Reserve/Commit Queue\n");
  ::fprintf(stderr, 
            "  Usage: mRCQRun \n"); 
  ::fprintf(stderr, 
            "      [--ticket {yes|no}]\n"); 
  ::fprintf(stderr, 
            "      [--memaccess {yes|no}]\n"); 
  ::fprintf(stderr, 
            "      [--pinning {yes|no}]\n"); 
  ::fprintf(stderr, 
            "      [--runningsec benchmark_tim_in_seconds]\n"); 
  ::fprintf(stderr, 
            "      [--elmsize queue_element_size]\n"); 
  ::fprintf(stderr, 
            "      [--qsize max_number_of_elements]\n");
  ::fprintf(stderr, 
            "      [--reservesize number_of_reserving_elements]\n");
  ::fprintf(stderr, 
            "      [--thread number_of_thread]\n"); 
}

static
void showResult(BenchmarkParam *BP) {
  double InstrPerCycle = (double)HWCounters[PERF_COUNT_HW_INSTRUCTIONS]/
    (double)HWCounters[PERF_COUNT_HW_CPU_CYCLES]; 

  double CyclesPerInstr = (double)HWCounters[PERF_COUNT_HW_CPU_CYCLES]/
    (double)HWCounters[PERF_COUNT_HW_INSTRUCTIONS];

  double StalledCyclesPerInstr = (double)(
    HWCounters[PERF_COUNT_HW_STALLED_CYCLES_FRONTEND] +
    HWCounters[PERF_COUNT_HW_STALLED_CYCLES_BACKEND]) / 
    (double)HWCounters[PERF_COUNT_HW_INSTRUCTIONS]; 

  double Throughput =  (((double)BP->elmsize*(double)BP->reservesize*(double)RunCount) /
                        (1024.0*1024.0)) / ((double)ExecTimeInMicroSec/(1000.0*1000.0)); 

  double MSecFor1MillPushPop = ((double)ExecTimeInMicroSec*1000.0)/(double)RunCount;

  ::fprintf(stdout, 
            "# DANBI RCQ microbenchmark\n");
  ::fprintf(stdout, 
            "# ticket: %s memaccess: %s pinning: %s qsize: %d runningsec: %d\n", 
            BP->ticket ? "yes" : "no", 
            BP->memaccess ? "yes" : "no", 
            BP->pinning ? "yes" : "no", 
            BP->qsize, 
            BP->runningsec);
  ::fprintf(stdout, 
            "# threads(1) elmsize(2) reservesize(3) MSecFor1MillPushPop(4) throughput(5)"
            " InstrPerCycle(6) CyclesPerInstr(7) StalledCyclesPerInstr(8)"
            " HW_CPU_CYCLES(9) HW_INSTRUCTIONS(10) CACHE_REFERENCES(11) HW_CACHE_MISSES(12)"
            " HW_BRANCH_INSTRUCTIONS(13) HW_BRANCH_MISSES(14) HW_BUS_CYCLES(15)"
            " HW_STALLED_CYCLES_FRONTEND(16) HW_STALLED_CYCLES_BACKEND(17)\n");
  ::fprintf(stdout, 
            "%d %d %d %f %f %f %f", 
            BP->thread, BP->elmsize, BP->reservesize, MSecFor1MillPushPop, Throughput, 
            InstrPerCycle, CyclesPerInstr, StalledCyclesPerInstr); 
  ::fprintf(stdout, 
            " %lld %lld %lld %lld", 
            HWCounters[PERF_COUNT_HW_CPU_CYCLES], 
            HWCounters[PERF_COUNT_HW_INSTRUCTIONS], 
            HWCounters[PERF_COUNT_HW_CACHE_REFERENCES], 
            HWCounters[PERF_COUNT_HW_CACHE_MISSES]); 
  ::fprintf(stdout, 
            " %lld %lld %lld", 
            HWCounters[PERF_COUNT_HW_BRANCH_INSTRUCTIONS], 
            HWCounters[PERF_COUNT_HW_BRANCH_MISSES],
            HWCounters[PERF_COUNT_HW_BUS_CYCLES]); 
  ::fprintf(stdout, 
            " %lld %lld\n", 
            HWCounters[PERF_COUNT_HW_STALLED_CYCLES_FRONTEND], 
            HWCounters[PERF_COUNT_HW_STALLED_CYCLES_BACKEND]); 
}

static 
int parseOption(int argc, char *argv[], BenchmarkParam* BP) {
  struct option options[] = {
    {"ticket", required_argument, 0, 'k'}, 
    {"memaccess", required_argument, 0, 'm'}, 
    {"pinning", required_argument, 0, 'p'}, 
    {"runningsec", required_argument, 0, 's'}, 
    {"elmsize", required_argument, 0, 'e'}, 
    {"qsize",   required_argument, 0, 'q'}, 
    {"reservesize",   required_argument, 0, 'v'}, 
    {"thread",   required_argument, 0, 't'}, 
    {0, 0, 0, 0}, 
  }; 

  // Set default values
  BP->ticket = true; 
  BP->memaccess = false; 
  BP->pinning = true; 
  BP->runningsec = 30; 
  BP->elmsize = sizeof(int); 
  BP->qsize = 1024; 
  BP->reservesize = 1; 
  BP->thread = 1; 

  // Parse options
  do {
    int opt = getopt_long(argc, argv, "", options, NULL); 
    if (opt == -1) break;
    switch(opt) {
    case 'k':
      BP->ticket = optarg[0] == 'y';
      break;
    case 'm':
      BP->memaccess = optarg[0] == 'y';
      break;
    case 'p':
      BP->pinning = optarg[0] == 'y';
      break;
    case 's':
      BP->runningsec = atoi(optarg); 
      break;
    case 'e':
      BP->elmsize = atoi(optarg); 
      break;
    case 'q':
      BP->qsize = atoi(optarg); 
      break;
    case 'v':
      BP->reservesize = atoi(optarg); 
      break;
    case 't':
      BP->thread = atoi(optarg); 
      break;
    case '?':
    default:
      showUsage();
      return 1; 
    };
  } while(1); 

  if (BP->qsize == 0) {
    // Queue size should be large enough to smash all LLC, 512MB in default. 
    BP->qsize = (512*1024*1024)/BP->elmsize; 
  }
  return 0; 
}

int main(int argc, char* argv[]) {
  PerformanceCounter PMCs[PERF_COUNT_HW_MAX];
  BenchmarkParam BP; 
  int ret; 

  // Parse options 
  ret = parseOption(argc, argv, &BP);
  if (ret) return 1; 

  // Initialize PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i) {
    ret = PMCs[i].initialize(i, false, true); 
    if (ret) return ret; 
  }

  // Run benchmark 
  ret = benchmarkMain(&BP);
  if (ret) return 2; 

  // Read PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i) {
    ret = PMCs[i].readCounter(HWCounters + i); 
    if (ret) return ret; 
  }

  // Print out results
  showResult(&BP);

  return 0; 
}

