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

  GreenThread.h -- user-level thread 
 */
#ifndef DANBI_GREEN_THREAD_H
#define DANBI_GREEN_THREAD_H
#include <cstdlib> 
#include <cerrno>
#include <cassert>
#include "Support/Random.h"
#include "Support/Machine.h"
#include "Support/SuperScalableQueue.h"
#include "Support/AbstractGreenRunnable.h"
#include "Support/ContextSwitch.h"

namespace danbi {
template <typename GreenSchedulerTy>
class  GreenThread {
  DANBI_SSQ_ITERABLE(GreenThread<GreenSchedulerTy>); 
private:
  volatile bool Terminated; 
  GreenSchedulerTy* Scheduler; 
  AbstractGreenRunnable<GreenSchedulerTy>& Runnable; 
  int StackSize; 
  int TLSSize; 
  double* AllocMem;
  void* Stack; 

  void operator=(const AbstractGreenRunnable<GreenSchedulerTy>&); // Do not implement
  GreenThread(const AbstractGreenRunnable<GreenSchedulerTy>&); // Do not implement

  static void StartHelper(GreenThread<GreenSchedulerTy>* This) {
    This->start(); 
  }

public:
  void* SP; 
  void* TLS; 

public:
  /// Constructor
  GreenThread(AbstractGreenRunnable<GreenSchedulerTy>& Runnable_, 
              int StackSize_ = (1024*1018), int TLSSize_ = (1024*2)) 
    : Terminated(false), Runnable(Runnable_), 
      StackSize(StackSize_ + (StackSize_%Machine::CachelineSize)), 
      TLSSize(TLSSize_ + (TLSSize_%Machine::CachelineSize)), 
      AllocMem(NULL) {
    Scheduler = &GreenSchedulerTy::getInstance();
  }

  /// Initialize to fully construct an instance  
  int initialize() {
    // Allocate memory if it is not allocated. 
    if (!AllocMem) {
      int Size = StackSize + TLSSize + 4096; 
      assert( Size%Machine::CachelineSize == 0 ); 
      AllocMem = new double[ (Size + sizeof(double) - 1) / sizeof(double) ]; 
      if (AllocMem == NULL) 
        return -ENOMEM; 

      // Random coloring of stack and TLB address
      int Color = Random::randomInt() % (4096 / Machine::CachelineSize); 
      TLS = (char*)AllocMem + (Color * Machine::CachelineSize);  
      Stack = (char*)TLS + TLSSize; 
    }

    // Initilize stack frame
    // - Thread stack should be sane before registered to a scheduler. 
    SP = ::danbi_InitGreenStack(Stack, 
                                StackSize, 
                                reinterpret_cast<void*>(StartHelper), 
                                this);

    // Register to scheduler
    Scheduler->registerThread(*this); 
    return 0; 
  }

  /// Destructor
  virtual ~GreenThread() {
    delete AllocMem; 
    AllocMem = NULL; 
  }

  /// Start runnable
  int start() {
    // Run green runnable
    Terminated = false; 
    Runnable.run(); 
    assert(isStackHealthy() && "Check if stack is corrupted or not."); 

    // Deregister from scheduler
    Scheduler->deregisterThread(*this); 
    Terminated = true; 
    return 0; 
  }

  /// Check if it is terminated; 
  bool isTerminated() const {
    return Terminated; 
  }

  /// Check if a stack is healthy
  bool isStackHealthy() {
    if (SP == NULL || Stack == NULL || StackSize <= 0)
      return false; 
    return ::danbi_IsGreenStackSane(Stack, StackSize) == 0;
  }
};

} // End of danbi namespace 

#endif 
