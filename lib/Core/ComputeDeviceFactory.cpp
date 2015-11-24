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

  ComputeDeviceFactory.cpp -- objectory factory for AbstractComputeDevice class
 */
#include <cstdlib>
#include "Core/ComputeDeviceFactory.h"
#include "Core/AbstractComputeDevice.h"
#include "Core/CPUComputeDevice.h"
#include "Core/AbstractUberKernel.h"
#include "Core/CPUUberKernel.h"

using namespace danbi; 

AbstractComputeDevice* ComputeDeviceFactory::newComputeDevice(int DeviceID, 
                                                              int MaxAssignableCores_, 
                                                              int NumAllKernel_, 
                                                              AbstractScheduler& Scheduler_, 
                                                              AbstractUberKernel& UberKernel_) {
  if (DeviceID == AbstractPartitioner::CPU_DEVICE_INDEX)
    return new CPUComputeDevice(MaxAssignableCores_, 
                                NumAllKernel_,
                                Scheduler_,
                                *reinterpret_cast<CPUUberKernel*>(&UberKernel_));
  assert(0 && "Not supported device."); 
  return NULL; 
}

