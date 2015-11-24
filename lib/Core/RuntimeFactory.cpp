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

  RuntimeFactory.cpp -- Object factory class for danbi runtime
 */
#include <cstdlib>
#include "Core/RuntimeFactory.h"
#include "Core/AllCPUPartitioner.h"
#include "Core/DistributedDynamicScheduler.h"

using namespace danbi; 

Runtime* RuntimeFactory::newRuntime(Program* Pgm, int NumCore) {
  Runtime* Rtm; 
  AllCPUPartitioner* Partitioner;
  DistributedDynamicScheduler* Scheduler;

  if ( !(Rtm = new Runtime()) ) return NULL;
  if ( !(Partitioner = new AllCPUPartitioner(*Rtm, *Pgm, NumCore)) ) goto error; 
  if ( !(Scheduler = new DistributedDynamicScheduler(*Rtm, *Pgm)) ) goto error;
  if ( Rtm->loadProgram(Pgm, Partitioner, Scheduler) ) goto error;

  return Rtm; 
 error:
  delete Rtm; 
  return NULL; 
}

