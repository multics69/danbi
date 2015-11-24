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

  Thread.cpp -- Thread implmentation
 */
#include <cassert>
#include <cstdlib>
#include <cstdarg>
#include <cerrno>
#include "Support/Machine.h"
#include "Support/MachineConfig.h"
#include "Support/Thread.h"
#include "Support/AbstractRunnable.h"

namespace danbi {
static volatile int __ID; 
__thread int Thread::ID = -1;
__thread Thread* Thread::This;
}; 

using namespace danbi; 

int Thread::getID() {
  // Since ID is a thread local variable, there is no contention. 
  if (ID == -1) 
    ID = Machine::atomicIntInc<volatile int>(&__ID); 
  return ID; 
}

Thread::Thread(AbstractRunnable& Runnable_, int StackSize_)
  : Runnable(Runnable_), StackSize(StackSize_), CPUID(-ENOTSUP) {
  CPU_ZERO(&CPUSet);
  Runnable.This = this; 
}

Thread::~Thread() {
  // do nothing
}

int Thread::setAffinity(int NumID, ...) {
  const MachineConfig& MC = MachineConfig::getInstance(); 
  ::va_list Args; 
  ::cpu_set_t CPUSet;
  
  // Initialize CPUSet from arguments
  va_start(Args, NumID); 
  int i, cpuid;
  for(i=0; i<NumID; ++i) {
    // Check if CPU ID is valid
    cpuid = va_arg(Args, int); 
    if (cpuid >= MC.getNumHardwareThreadsInCPUs())
      return -EINVAL; 

    // Add the ID to the set 
    CPU_SET(cpuid, &CPUSet);
  }
  if (NumID == 1)
    CPUID = cpuid; 
  va_end(Args); 

  return 0; 
}

int Thread::getAffinity() {
  return CPUID;
}

static void* runRunnable(void *Runnable) {
  static_cast<AbstractRunnable*>(Runnable)->run(); 
  return NULL; 
}

int Thread::start() {
  ::pthread_attr_t Attr;
  int ret = 0;
  
  ::pthread_attr_init(&Attr); 

  // Set the requested stack size, if given.                                    
  if (StackSize != 0) {
    ret = ::pthread_attr_setstacksize(&Attr, StackSize);
    if (ret)
      goto end; 
  }

  // Construct and execute the thread.                                          
  ret = ::pthread_create(&NativeThread, &Attr, runRunnable, &Runnable); 
  if (ret)
    goto end;

  // Set CPU affinity if any
  if (CPU_COUNT(&CPUSet)) {
    ret = ::pthread_setaffinity_np(NativeThread, sizeof(cpu_set_t), &CPUSet);
    if (ret)
      goto end;
  }

 end:
  ::pthread_attr_destroy(&Attr);
  return ret;
}

int Thread::join() {
  return ::pthread_join(NativeThread, 0); 
}


