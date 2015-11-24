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

  UberKernelFactory.h -- object factory for AbstractUberKernel class
 */
#ifndef DANBI_UBERKERNEL_FACTORY_H
#define DANBI_UBERKERNEL_FACTORY_H
#include "Core/AbstractUberKernel.h"
#include "Core/AbstractPartitioner.h"

namespace danbi {
class UberKernelFactory {
public:
  static AbstractUberKernel* newUberKernel(int DeviceID, AbstractScheduler* Scheduler_); 
};

} // End of danbi namespace

#endif 
