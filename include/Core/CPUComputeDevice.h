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

  CPUComputeDevice.h -- CPU compute device
 */
#ifndef DANBI_CPU_COMPUTE_DEVICE_H
#define DANBI_CPU_COMPUTE_DEVICE_H
#include "Support/GreenScheduler.h"
#include "Core/AbstractComputeDevice.h"
#include "Core/CPUSchedulePolicyAdapter.h"

namespace danbi {
class NativeCPUUberKernel; 

class CPUComputeDevice : public AbstractComputeDevice {
private:
  GreenScheduler<CPUSchedulePolicyAdapter>& CPUs; 

  void operator=(const CPUComputeDevice&); // Do not implement
  CPUComputeDevice(const CPUComputeDevice&); // Do not implement

public:
  /// Constructor 
  CPUComputeDevice(int MaxAssignableCores_, 
                   int NumAllKernel_, 
                   AbstractScheduler& Scheduler_, 
                   CPUUberKernel& UberKernel_); 

  /// Destructor
  virtual ~CPUComputeDevice();

  /// Prepare execution 
  int prepare(); 

  /// Start execution 
  int start(); 

  /// Join
  int join(); 
}; 

} // End of danbi namespace 

#endif 
