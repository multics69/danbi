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

  MapReduceMainMockup.h -- MapReduce main() mockup
 */
#include <unistd.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "Support/PerformanceCounter.h"

#ifdef DANBI_MAPREDUCE_MAIN_MOCKUP_H
#error "Something wrong. You already included this."
#endif 
#define DANBI_MAPREDUCE_MAIN_MOCKUP_H

#ifndef BENCHMARK_CLASS
#error "Please, define BENCHMARK_CLASS."
#endif

#ifndef BENCHMARK_CLASS_STR
#error "Please, define BENCHMARK_CLASS_STR."
#endif

static unsigned long long HWCounters[PERF_COUNT_HW_MAX];
static unsigned long long ExecTimeInMicroSec; 

static
void showResult(int cores, char* input) {
  double InstrPerCycle = (double)HWCounters[PERF_COUNT_HW_INSTRUCTIONS]/
    (double)HWCounters[PERF_COUNT_HW_CPU_CYCLES]; 

  double CyclesPerInstr = (double)HWCounters[PERF_COUNT_HW_CPU_CYCLES]/
    (double)HWCounters[PERF_COUNT_HW_INSTRUCTIONS];

  double StalledCyclesPerInstr = (double)(
    HWCounters[PERF_COUNT_HW_STALLED_CYCLES_FRONTEND] +
    HWCounters[PERF_COUNT_HW_STALLED_CYCLES_BACKEND]) / 
    (double)HWCounters[PERF_COUNT_HW_INSTRUCTIONS]; 

  ::fprintf(stdout, 
            "DANBI " BENCHMARK_CLASS_STR " MapReduce benchmark\n");
  ::fprintf(stdout, 
            "# cores(1) input(2) microsec(3)"
            " InstrPerCycle(4) CyclesPerInstr(5) StalledCyclesPerInstr(6)"
            " HW_CPU_CYCLES(7) HW_INSTRUCTIONS(8) CACHE_REFERENCES(9) HW_CACHE_MISSES(10)"
            " HW_BRANCH_INSTRUCTIONS(11) HW_BRANCH_MISSES(12) HW_BUS_CYCLES(13)"
            " HW_STALLED_CYCLES_FRONTEND(14) HW_STALLED_CYCLES_BACKEND(15)\n");
  ::fprintf(stdout, 
            "%d %s %lld %f %f %f", 
            cores, input, ExecTimeInMicroSec, 
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
void showUsage(void) {
  ::fprintf(stderr, 
            "DANBI " BENCHMARK_CLASS_STR " MapReduce benchmark\n");
  ::fprintf(stderr, 
            "  Usage: mr" BENCHMARK_CLASS_STR "Run \n"); 
  ::fprintf(stderr, 
            "      [--core maximum_number_of_assignable_cores]\n");
  ::fprintf(stderr, 
            "      [--input input_file_name]\n");
  ::fprintf(stderr, 
            "      [--log log_file_name]\n");
}

static 
int parseOption(int argc, char *argv[], int *cores, char **input, char **log) {
  struct option options[] = {
    {"core",   required_argument, 0, 'c'}, 
    {"input",   required_argument, 0, 'i'}, 
    {"log",   required_argument, 0, 'l'}, 
    {0, 0, 0, 0}, 
  }; 

  // Set default values
  *cores = 1; 
  *input = const_cast<char*>("Data/" BENCHMARK_CLASS_STR "-tiny.dat");
  *log = const_cast<char*>(BENCHMARK_CLASS_STR);

  // Parse options
  do {
    int opt = getopt_long(argc, argv, "", options, NULL); 
    if (opt == -1) return 0; 
    switch(opt) {
    case 'c':
      *cores = atoi(optarg); 
      break;
    case 'i':
      *input = optarg; 
       break;
    case 'l':
      *log = optarg; 
       break;
    case '?':
    default:
      showUsage();
      return 1; 
    };
  } while(1); 
  return 0; 
}

template <typename MapReduceApp>
int benchmark(int Cores, char* Input, char* Log) {
  int fd;
  struct stat finfo; 
  void* Data; 
  long DataLen; 

  // Load user provided data
  if ((fd = ::open(Input, O_RDONLY)) < 0) 
    return errno; 
  if (::fstat(fd, &finfo) < 0)
    return errno;
  DataLen = finfo.st_size + 1; 
  if ((Data = ::mmap(0, DataLen, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == NULL) 
    return errno;
  
  // Create and run the user application 
  {
    MapReduceApp App(const_cast<const char*>(Log), Cores, Data, DataLen);

    App.initialize();
    PerformanceCounter::start(); 
    App.run(); 
    ExecTimeInMicroSec = PerformanceCounter::stop(); 
    App.log();
  }

  // Unload the user provided data
  if (::munmap(Data, DataLen) < 0) 
    return errno; 
  if (::close(fd) < 0)
    return errno;
  return 0;
}

int main (int argc, char* argv[]) {
  PerformanceCounter PMCs[PERF_COUNT_HW_MAX];
  int cores; 
  char *log, *input;
  int ret; 

  // Parse options 
  ret = parseOption(argc, argv, &cores, &input, &log);
  if (ret) return 1; 

  // Initialize PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].initialize(i, false, true); 

  // Run benchmark 
  ret = benchmark<BENCHMARK_CLASS>(cores, input, log);
  if (ret) return 2; 

  // Read PMCs
  for (int i = 0; i < PERF_COUNT_HW_MAX; ++i)
    PMCs[i].readCounter(HWCounters + i); 

  // Print out results
  showResult(cores, input); 

  return 0; 
}
