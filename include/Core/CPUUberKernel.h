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

  CPUUberKernel.h -- Native CPU uber kernel 
 */
#ifndef DANBI_CPU_UBER_KERNEL_H
#define DANBI_CPU_UBER_KERNEL_H
#include <cerrno>
#include "Support/AbstractGreenRunnable.h"
#include "Support/GreenScheduler.h"
#include "Core/AbstractUberKernel.h"
#include "Core/SchedulingEvent.h"
#include "Core/AbstractPartitioner.h"
#include "Core/CPUSchedulePolicyAdapter.h"

namespace danbi {
class Kernel; 
class CPUSchedulePolicyAdapter; 

class CPUUberKernel 
  : virtual public AbstractUberKernel, 
    virtual public AbstractGreenRunnable< GreenScheduler<CPUSchedulePolicyAdapter> > {
private:
  CPUSchedulePolicyAdapter& CPUSchedulePolicy; 
  volatile int KickOff;
  void operator=(const CPUUberKernel&); // Do not implement
  CPUUberKernel(const CPUUberKernel&); // Do not implement

  /// Run 
  virtual void main(); 
public:
  /// Constructor
  CPUUberKernel(AbstractScheduler* Scheduler_);

  /// Destructor 
  virtual ~CPUUberKernel(); 

  /// Uberkernelize
  virtual int uberkernelize(); 
  
  /// Load to the device
  virtual int loadToDevice(int DeviceIndex = AbstractPartitioner::CPU_DEVICE_INDEX); 

  /// Kick off
  void kickoff() {
    KickOff = 1; 
    Machine::wmb();
  }
};

} // End of danbi namespace 

#endif 
