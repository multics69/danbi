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

  DanbiCPUDeprecated.h -- deprecated header file for danbi application development
 */
#ifndef DANBI_CPU_DEPRECATED_H
#define DANBI_CPU_DEPRECATED_H
#include "Core/CPUCode.h"
#include "Core/Kernel.h"
#include "Core/QueueFactory.h"
#include "Core/ReadOnlyBuffer.h"
#include "Core/AllCPUPartitioner.h"
#include "Core/DistributedDynamicScheduler.h"

namespace danbi {
typedef AbstractReserveCommitQueue Queue; 
} // End of danbi namespace 
#endif 
