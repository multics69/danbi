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

  CPUSchedulePolicyAdapter.h 
  -- CPU schedule policy adapter between GreenScheduler and AbstractScheduler
 */
#ifndef DANBI_CPU_SCHEDULE_POLICY_ADAPTER_H
#define DANBI_CPU_SCHEDULE_POLICY_ADAPTER_H
#include <cstdlib>
#include "Support/GreenScheduler.h"
#include "Support/GreenThread.h"
#include "Support/AbstractGreenRunnable.h"
#include "Support/SuperScalableQueue.h"
#include "Support/MSQueue.h"
#include "Support/LSQueue.h"
#include "Core/DistributedDynamicScheduler.h"

namespace danbi {
class CPUSchedulePolicyAdapter; 
class CPUUberKernel;

class CPUSchedulePolicyAdapter {
private:
#ifdef DNABI_READY_QUEUE_USE_MSQ
  typedef MSQueue< 
    GreenThread<GreenScheduler<CPUSchedulePolicyAdapter>> > ConcurrentQueue; 
#elif defined DNABI_READY_QUEUE_USE_LSQ
  typedef LSQueue< 
    GreenThread<GreenScheduler<CPUSchedulePolicyAdapter>> > ConcurrentQueue; 
#else
  typedef SuperScalableQueue< 
    GreenThread<GreenScheduler<CPUSchedulePolicyAdapter>> > ConcurrentQueue; 
#endif 

private:
  int NumAllKernel; 
  DistributedDynamicScheduler* Scheduler; // Bound to DistributedDynamicScheduler
  CPUUberKernel* UberKernel; 
  ConcurrentQueue* ReadyQueues; 
  ConcurrentQueue  RecycleBin; 

  void operator=(const CPUSchedulePolicyAdapter&); // Do not implement
  CPUSchedulePolicyAdapter(const CPUSchedulePolicyAdapter&); // Do not implement
  
  /// Constructor
  CPUSchedulePolicyAdapter(); 

  /// Destructor
  virtual ~CPUSchedulePolicyAdapter();

  /// Actually destruct and reinit. the object 
  void destruct();

  /// Select a kernel 
  bool selectKernel(
    GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* Thread_, 
    int RetryCount);

  /// Select a thread for a kernel
  GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* 
  selectThreadForKernel();

  /// Create a new green thread
  GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* newGreenThread(); 

  /// Test if it keeps going with this thread instance or not 
  bool keepGoingWithThisThread(SchedulingEventKind Event, int KernelIndex); 

  /// Logging
  void LogScheduleEvent(const char* Msg, int OldKernel,
                        GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* NewThread);

public:
  /// Get singleton instance
  static CPUSchedulePolicyAdapter& getInstance(); 

  /// Register AbstractScheduler and UberKernel
  int initialize(int NumAllKernel_, 
                 AbstractScheduler& Scheduler_, 
                 CPUUberKernel& UberKernel_); 
  
  /// Register a green thread
  void registerThread(GreenThread< 
                        GreenScheduler<CPUSchedulePolicyAdapter> >& Thread_);

  /// Deregister runnable green thread
  void deregisterThread(GreenThread< 
                          GreenScheduler<CPUSchedulePolicyAdapter> >& Thread_);

  /// Schedule out a thread
  void scheduleOutThread(GreenThread< 
                           GreenScheduler<CPUSchedulePolicyAdapter> >* Thread_); 

  /// Select next schedulable green thread
  GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* selectNext(
    GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* Thread_ = NULL);

  /// Check thread termination condition
  void checkTerminationCondition(); 

  /// Decide if it keeps running this kernel or not
  bool keepContinue(); 

  /// Kick off 
  void kickoff();
}; 


} // End of danbi namespace 

#endif 
