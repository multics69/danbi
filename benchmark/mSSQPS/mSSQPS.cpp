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

  mSSQPS.cpp -- micro-benchmark for SSQPS
 */
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <cerrno>
#include <list>
#include <algorithm>
#include "Support/SuperScalableQueue.h"
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
  bool pinning; 
  int runningsec; 
  int initelms; 
  int thread; 
};

class Element {
  DANBI_SSQ_ITERABLE(Element); 
public:
  int value; 
} __cacheline_aligned; 

class mSSQPSWorker: public Thread, private AbstractRunnable {
private:
  volatile bool& Running;
  BenchmarkParam* BP; 
  SuperScalableQueue<Element>& Q; 
  Element* Elm; 

  void main() {
    // Until running flag is on 
    while (!Running) {
      Machine::mb(); 
    }
      
    // Work 
    unsigned long runcount = 0; 
    Element* E = Elm; 
    while (Running) {
      ++runcount; 
      Q.push(E); 
      while ( Q.pop(E) ) ; 
    }
    Q.push(E); 
    ++runcount; 

    // Reduce run count 
    Machine::atomicWordAddReturn<unsigned long>(&RunCount, runcount); 
  }

public:
  mSSQPSWorker(volatile bool& Running_, BenchmarkParam* BP_, 
               SuperScalableQueue<Element>& Q_, Element* Elm_)
    : Thread(static_cast<AbstractRunnable*>(this)[0]), 
      Running(Running_), BP(BP_), Q(Q_), Elm(Elm_) {}
};

int benchmarkMain(BenchmarkParam *BP) {
  // Create a queue
  SuperScalableQueue<Element>* Q = new SuperScalableQueue<Element>(); 
  if (!Q)  return -ENOMEM; 

  // Create initial elements
  for (int i = 0; i < BP->initelms; ++i) {
    Element* Elm = new Element(); 
    if (!Elm) return -ENOMEM; 
    Q->push(Elm); 
  }

  // Create a thread
  volatile bool Running = false; 
  Machine::mb(); 
  int Cores = MachineConfig::getInstance().getNumHardwareThreadsInCPUs();
  std::list<mSSQPSWorker*> Workers; 
  for (int i = 0; i < BP->thread; ++i) {
    Element* Elm = new Element(); 
    if (!Elm) return -ENOMEM; 
    mSSQPSWorker* Worker = new mSSQPSWorker(Running, BP, *Q, Elm); 
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
  for(std::list<mSSQPSWorker*>::iterator 
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
            "DANBI micro-benchmark for Super Scalable Queue with Permanent Sentinel Node"); 
  ::fprintf(stderr, 
            "  Usage: mSSQPSRun \n"); 
  ::fprintf(stderr, 
            "      [--pinning {yes|no}]\n"); 
  ::fprintf(stderr, 
            "      [--runningsec benchmark_tim_in_seconds]\n"); 
  ::fprintf(stderr, 
            "      [--initelms initial_number_of_elements]\n");
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

  double MSecFor1MillPushPop = ((double)ExecTimeInMicroSec*1000.0)/(double)RunCount;

  ::fprintf(stdout, 
            "# DANBI SSQPS microbenchmark\n");
  ::fprintf(stdout, 
            "# pinning: %s initelms: %d runningsec: %d\n", 
            BP->pinning ? "yes" : "no", 
            BP->initelms, 
            BP->runningsec);
  ::fprintf(stdout, 
            "# threads(1) MSecFor1MillPushPop(2)"
            " InstrPerCycle(3) CyclesPerInstr(4) StalledCyclesPerInstr(5)"
            " HW_CPU_CYCLES(6) HW_INSTRUCTIONS(7) CACHE_REFERENCES(8) HW_CACHE_MISSES(9)"
            " HW_BRANCH_INSTRUCTIONS(10) HW_BRANCH_MISSES(13) HW_BUS_CYCLES(11)"
            " HW_STALLED_CYCLES_FRONTEND(12) HW_STALLED_CYCLES_BACKEND(13)\n");
  ::fprintf(stdout, 
            "%d %f %f %f %f", 
            BP->thread, MSecFor1MillPushPop, 
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
    {"pinning", required_argument, 0, 'p'}, 
    {"runningsec", required_argument, 0, 's'}, 
    {"initelms",   required_argument, 0, 'i'}, 
    {"thread",   required_argument, 0, 't'}, 
    {0, 0, 0, 0}, 
  }; 

  // Set default values
  BP->pinning = true; 
  BP->runningsec = 1;
  BP->initelms = 0;
  BP->thread = 2;

  // Parse options
  do {
    int opt = getopt_long(argc, argv, "", options, NULL); 
    if (opt == -1) break;
    switch(opt) {
    case 'p':
      BP->pinning = optarg[0] == 'y';
      break;
    case 's':
      BP->runningsec = atoi(optarg); 
      break;
    case 'i':
      BP->initelms = atoi(optarg); 
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

