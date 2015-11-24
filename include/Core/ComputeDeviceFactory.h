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

  ComputeDeviceFactory.h -- object factory for AbstractComputeDevice class
 */
#ifndef DANBI_COMPUTE_DEVICE_FACTORY_H
#define DANBI_COMPUTE_DEVICE_FACTORY_H
#include "Core/AbstractComputeDevice.h"

namespace danbi {
class AbstractScheduler; 
class AbstractUberKernel; 
class AbstractComputeDevice; 

class ComputeDeviceFactory {
public:
  static AbstractComputeDevice* newComputeDevice(int DeviceID, 
                                                 int MaxAssignableCores_, 
                                                 int NumAllKernel_, 
                                                 AbstractScheduler& Scheduler_, 
                                                 AbstractUberKernel& UberKernel_);
};

} // End of danbi namespace

#endif 
