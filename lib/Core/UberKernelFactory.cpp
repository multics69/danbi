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

  UberKernelFactory.cpp -- objectory factory for ueber kernel
 */
#include <cstdlib>
#include "Core/UberKernelFactory.h"
#include "Core/CPUUberKernel.h"

using namespace danbi; 

AbstractUberKernel* UberKernelFactory::newUberKernel(int DeviceID, AbstractScheduler* Scheduler_) {
  if (DeviceID == AbstractPartitioner::CPU_DEVICE_INDEX)
    return new CPUUberKernel(Scheduler_); 
  assert(0 && "Not supported device."); 
  return NULL; 
}

