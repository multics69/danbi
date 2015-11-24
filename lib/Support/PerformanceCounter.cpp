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

  PerformanceCounter.cpp -- implementation of HW performance counter 
 */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <stdbool.h>
#include <cerrno>
#include "Support/PerformanceCounter.h"

using namespace danbi; 

struct timeval PerformanceCounter::StartTime; 
struct timeval PerformanceCounter::StopTime; 

PerformanceCounter::PerformanceCounter() : Fd(-1) {}

int PerformanceCounter::initialize(int HWEvent, bool ExcludeUser, bool ExcludeKernel) {
  // Set attributes
  ::memset(&Attr, 0, sizeof(Attr)); 
  Attr.type = PERF_TYPE_HARDWARE; 
  Attr.disabled = 1; // initially counter is disabled
  Attr.exclude_hv = 1; // never count on hypervisor
  Attr.exclude_idle = 0; // never count when idle
  Attr.inherit = 1; // monitor a new descent tasks
  Attr.config = HWEvent; 
  Attr.exclude_user = ExcludeUser; 
  Attr.exclude_kernel = ExcludeKernel; 
  Attr.size = sizeof(Attr); 

  // Open a FD
  Fd = ::syscall(__NR_perf_event_open, &Attr, 
                 0 /* current process */, 
                 -1 /* count on all CPUs */, -1, 0); 
  if (Fd < 0) 
    return -EINVAL; 
  return 0; 
}

PerformanceCounter::~PerformanceCounter() {
  if (Fd < 0) 
    ::close(Fd); 
}

int PerformanceCounter::readCounter(unsigned long long* Counter) {
  int ret = ::read(Fd, Counter, sizeof(*Counter)); 
  if (ret == -1) return -1; 
  return 0; 
}

void PerformanceCounter::start(void)
{
  ::gettimeofday(&StartTime, NULL); 
  ::prctl(PR_TASK_PERF_EVENTS_ENABLE); 
}

unsigned long long PerformanceCounter::stop(void)
{
  ::prctl(PR_TASK_PERF_EVENTS_DISABLE);
  ::gettimeofday(&StopTime, NULL); 
  return ((StopTime.tv_sec - StartTime.tv_sec) * 1000000) +
    (StopTime.tv_usec - StartTime.tv_usec); 
}
