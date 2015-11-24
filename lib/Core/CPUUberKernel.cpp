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

  CPUUberKernel.cpp -- implementation of native CPU uber kernel 
 */
#include <cstdlib>
#include <cassert>
#include "Core/CPUUberKernel.h"
#include "Core/Kernel.h"
#include "Core/ReadOnlyBuffer.h"
#include "Core/AbstractCode.h"
#include "Core/SystemContext.h"
#include "Core/AbstractPartitioner.h"
#include "Core/DistributedDynamicScheduler.h"
#include "Core/CPUSchedulePolicyAdapter.h"

using namespace danbi; 

CPUUberKernel::CPUUberKernel(AbstractScheduler* Scheduler_) 
  : AbstractUberKernel(Scheduler_), 
    CPUSchedulePolicy( CPUSchedulePolicyAdapter::getInstance() ), 
    KickOff(0) {
  Machine::wmb();
}

CPUUberKernel::~CPUUberKernel() {
}

int CPUUberKernel::uberkernelize() {
  // Check if all codes support CPU execution
  for(std::set<AbstractCode*>::iterator
        i = Codes.begin(), e = Codes.end(); 
      i != e; ++i) {
    int ret = (*i)->supportDevice(AbstractPartitioner::CPU_DEVICE_INDEX); 
    if (ret) return ret; 
  }
  return 0; 
}

int CPUUberKernel::loadToDevice(int DeviceIndex) {
  // Check if the device is proper
  if (AbstractPartitioner::CPU_DEVICE_INDEX != DeviceIndex) 
    return -ENOTSUP; 

  // Load codes to the device
  for(std::set<AbstractCode*>::iterator
        i = Codes.begin(), e = Codes.end(); 
      i != e; ++i) {
    int ret = (*i)->loadToDevice(AbstractPartitioner::CPU_DEVICE_INDEX); 
    if (ret) return ret; 
  }

  // Load read-only buffers to the device 
  for(std::set<ReadOnlyBuffer*>::iterator
        i = ReadOnlyBuffers.begin(), e = ReadOnlyBuffers.end(); 
      i != e; ++i) {
    int ret = (*i)->loadToDevice(AbstractPartitioner::CPU_DEVICE_INDEX); 
    if (ret) return ret; 
  }

  // Drops all no longer used elements
  Kernels.clear(); 
  Codes.clear(); 
  ReadOnlyBuffers.clear(); 
  return 0; 
}

void CPUUberKernel::main() {
  // Wait until it gets kicked off
  while (!KickOff) 
    Machine::rmb(); 

  // Excute the real work 
  do {
    Scheduler->initKernelExecution(); 
    SystemContext::RunningKernel->run(); 
  } while( CPUSchedulePolicy.keepContinue() ); 
}
