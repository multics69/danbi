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

  CPUComputeDevice.cpp -- CPU Compute device
 */
#include <algorithm>
#include "Support/MachineConfig.h"
#include "Core/CPUComputeDevice.h"
#include "Core/CPUSchedulePolicyAdapter.h"
#include "Core/CPUUberKernel.h"

using namespace danbi; 

CPUComputeDevice::CPUComputeDevice(int MaxAssignableCores_, 
                                   int NumAllKernel_, 
                                   AbstractScheduler& Scheduler_, 
                                   CPUUberKernel& UberKernel_)
  : AbstractComputeDevice(MaxAssignableCores_, NumAllKernel_, Scheduler_, UberKernel_), 
    CPUs(GreenScheduler<CPUSchedulePolicyAdapter>::getInstance()) {
}

CPUComputeDevice::~CPUComputeDevice() {
  // Do nothing
}

int CPUComputeDevice::prepare() {
  CPUSchedulePolicyAdapter& SchedulePolicy = CPUSchedulePolicyAdapter::getInstance(); 
  int ret = 0; 

  // Initialize policy adapter
  ret = SchedulePolicy.initialize(NumAllKernel, 
                                  Scheduler, 
                                  *reinterpret_cast<CPUUberKernel*>(&UberKernel)); 
  if (ret)
    return ret;

  // Create native threads
  int NumNativeThreads = std::min(
    MaxAssignableCores,
    MachineConfig::getInstance().getNumHardwareThreadsInCPUs()); 
  ret = CPUs.createNativeThreads(NumNativeThreads, true); 
  if (ret)
    return ret; 

  // Start green thread scheduler
  ret = CPUs.start();

  // Wait for a while for each native thread 
  // to initialize and create corresponding a green thread
  if (!ret)
    ::usleep(1000*100); // 100 msec
  return ret; 
}

int CPUComputeDevice::start() {
  // kick off the created native threads
  CPUSchedulePolicyAdapter::getInstance().kickoff();
  return 0; 
}

int CPUComputeDevice::join() {
  return CPUs.join(); 
}
