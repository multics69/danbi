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

  FIFOSchedulePolicy.h -- FIFO scheduler for green thread
 */
#ifndef DANBI_FIFO_SCHEDULE_POLICY_H
#define DANBI_FIFO_SCHEDULE_POLICY_H
#include "Support/SuperScalableQueue.h"
#include "Support/GreenScheduler.h"
#include "Support/GreenThread.h"

namespace danbi {

class FIFOSchedulePolicy {
private:
  SuperScalableQueue< 
    GreenThread<GreenScheduler<FIFOSchedulePolicy>> > ReadyQueue; 

  void operator=(const FIFOSchedulePolicy&); // Do not implement
  FIFOSchedulePolicy(const FIFOSchedulePolicy&); // Do not implement
  
  /// Constructor
  FIFOSchedulePolicy();

  /// Destructor
  virtual ~FIFOSchedulePolicy();

public:
  /// Get singleton instance
  static FIFOSchedulePolicy& getInstance(); 
  
  /// Register a green thread
  void registerThread(GreenThread< 
                        GreenScheduler<FIFOSchedulePolicy> >& Thread_) {
    ReadyQueue.push(&Thread_); 
  }

  /// Deregister a green thread
  void deregisterThread(GreenThread< 
                          GreenScheduler<FIFOSchedulePolicy> >& Thread_) {
    // Do nothing, 
    // since running thread already does not exist in the ready queue. 
  }

  /// Schedule out a thread
  void scheduleOutThread(GreenThread< 
                           GreenScheduler<FIFOSchedulePolicy> >* Thread_) {
      // Put the old to the ready queue
    ReadyQueue.push(Thread_); 
  }

  /// Select next schedulable green thread
  GreenThread< GreenScheduler<FIFOSchedulePolicy> >* selectNext(
    GreenThread< GreenScheduler<FIFOSchedulePolicy> >* Thread_ = NULL) {
    // Pop a new from the ready queue
    GreenThread< GreenScheduler<FIFOSchedulePolicy> >* Next = NULL; 
    if (ReadyQueue.pop(Next)) {
      // If pop fails, try this. 
      Next = Thread_;
    }
    return Next; 
  }
}; 


} // End of danbi namespace 

#endif 
