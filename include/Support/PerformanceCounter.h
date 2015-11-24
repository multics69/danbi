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

  PerformanceCounter.h -- H/W performance counter
 */
#ifndef DANBI_PERFORMANCE_COUNTER_H
#define DANBI_PERFORMANCE_COUNTER_H
#include <sys/time.h>
#include <linux/perf_event.h>

namespace danbi {

class PerformanceCounter {
private:
  static struct timeval StartTime; 
  static struct timeval StopTime; 

  struct perf_event_attr Attr; 
  int Fd; 
 
  void operator=(const PerformanceCounter&); // Do not implement
  PerformanceCounter(const PerformanceCounter&); // Do not implement

public:
  /// Constructor
  PerformanceCounter(); 

  /// Initialize 
  /// - HWEvent is defined at linux/perf_event.h
  int initialize(int HWEvent, bool ExcludeUser, bool ExcludeKernel); 

  /// Destructor
  ~PerformanceCounter(); 

  /// Read measured performance counter
  int readCounter(unsigned long long* Counter); 
  
  /// Start to measure all the performance counters created
  static void start(); 

  /// Stop to measure all the performance counters created
  /// Returns time taken between start() and stop() in micro seconds
  static unsigned long long stop(); 
};

} // End of danbi namespace 

#endif 

