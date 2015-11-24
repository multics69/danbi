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

  GreenScheduler.h -- green thread scheduler
 */ 
#ifndef DANBI_GREEN_SCHEDULER_H
#define DANBI_GREEN_SCHEDULER_H
#include <cassert>
#include <cerrno>
#include <list>
#include "Support/Thread.h"
#include "Support/AbstractRunnable.h"
#include "Support/GreenThread.h"
#include "Support/ContextSwitch.h"
#include "Support/MachineConfig.h"
#include "Support/BranchHint.h"

namespace danbi {

template <typename GreenSchedulePolicyTy>
class GreenScheduler : public AbstractRunnable {
public:
  static __thread GreenThread< GreenScheduler<GreenSchedulePolicyTy> >* RunningThread; 
private:
  GreenSchedulePolicyTy* Policy; 
  std::list<Thread*> NativeThreads; 
  bool Pinning; 
  
  void operator=(const GreenScheduler<GreenSchedulePolicyTy>&); // Do not implement
  GreenScheduler(const GreenScheduler<GreenSchedulePolicyTy>&); // Do not implement

  /// Constructor
  GreenScheduler()
    : Pinning(false) {
    Policy = &GreenSchedulePolicyTy::getInstance();
  }

  /// Destructor 
  ~GreenScheduler() {
    destruct(); 
  }

  /// Actually destruct and reinit the object
  void destruct() {
    // Make sure if all native threads are terminated
    join(); 

    // delete *
    for(std::list<Thread*>::iterator 
          i = NativeThreads.begin(), e = NativeThreads.end(); 
        i != e; ++i) {
      delete *i; 
    }

    // Clear variables
    Pinning = false; 
  }

  /// Schedule out helper
  static void ScheduleOutHelper(void* Thread_) {
    GreenSchedulePolicyTy::getInstance().scheduleOutThread(
      static_cast<GreenThread< GreenScheduler<GreenSchedulePolicyTy> >*>(
        Thread_));
  }

  /// Runnable of native threads
  /// - Keep launching green threads until there is no. 
  /// ============================ WARNING ============================
  /// - SHOULD BE THREAD-REENTRANT
  /// - NEVER EVER USE STACK VARIABLES, SINCE STACK COULD BE INSANE. 
  /// ============================ WARNING ============================
  virtual void main() {
    do {
      static __thread 
        GreenThread< GreenScheduler<GreenSchedulePolicyTy> >* Next; 
      Next = Policy->selectNext(); 
      if ( unlikely(!Next) )
        return; 
      assert( Next->isStackHealthy() && "Thread stack is corrupted.");
      RunningThread = Next; 
      ::danbi_StartGreenThread(Next->SP);
    } while (true); 
  }

public:
  /// Get singleton instance
  static GreenScheduler& getInstance() {
    static GreenScheduler Instance; 
    return Instance; 
  }

  /// Register a green thread
  void registerThread(GreenThread< 
                          GreenScheduler<GreenSchedulePolicyTy> >& Thread_) {
    Policy->registerThread(Thread_); 
  }

  /// Deregister a green thread
  void deregisterThread(GreenThread< 
                            GreenScheduler<GreenSchedulePolicyTy> >& Thread_) {
    Policy->deregisterThread(Thread_); 
  }

  /// Yield voluntarily
  void yield(GreenThread<
               GreenScheduler<GreenSchedulePolicyTy> >& Thread_) {
    assert( Thread_.isStackHealthy() && "Thread stack is corrupted."); 

    // Select new thread
    GreenThread< GreenScheduler<GreenSchedulePolicyTy> >*
      Next = Policy->selectNext(&Thread_); 

    // If there is a new thread, switch to the new thread
    if ( likely(Next && Next != &Thread_) ) {
      RunningThread = Next; 
      ::danbi_ContextSwitchWithCallback(
        &Thread_.SP, Next->SP,
        GreenScheduler<GreenSchedulePolicyTy>::ScheduleOutHelper, &Thread_); 
    }
  }

  /// Start scheduling
  int start() {
    int NumHWThreads = MachineConfig::getInstance().getNumHardwareThreadsInCPUs(); 
    int StartingCore = ::random() % NumHWThreads; 
    int Count = 0; 
    for(std::list<Thread*>::iterator 
          i = NativeThreads.begin(), e = NativeThreads.end(); 
        i != e; ++i, ++Count) {
      int ret; 

      // If Pinning is set, pin a thread sequentially from random core
      if (Pinning) {
        ret = (*i)->setAffinity(1, (StartingCore + Count) % NumHWThreads); 
        if (ret) return ret; 
      }

      // Start kernel 
      ret = (*i)->start();
      if (ret) return ret; 
    }
    return 0; 
  }

  /// Join until all native threads are terminated
  int join() {
    for(std::list<Thread*>::iterator 
          i = NativeThreads.begin(), e = NativeThreads.end(); 
        i != e; ++i) {
      int ret = (*i)->join(); 
      if (ret) return ret; 
    }
    NativeThreads.clear(); 
    return 0; 
  }

  /// Create the number of native threads
  int createNativeThreads(int Num, bool Pinning_ = false) {
    // If there are already create threads, clear all. 
    if ( NativeThreads.empty() )
      destruct(); 
    assert( NativeThreads.empty() ); 

    // Create threads
    for(int i = 0; i < Num; ++i) {
      Thread* New = new Thread(*this, Thread::MinStackSize); 
      if ( !New ) return -ENOMEM;
      NativeThreads.push_back(New); 
    }
    Pinning = Pinning_; 
    return 0; 
  }
};

template<typename GreenSchedulePolicyTy>
  __thread GreenThread< GreenScheduler<GreenSchedulePolicyTy> >* 
  GreenScheduler<GreenSchedulePolicyTy>::RunningThread;
} // End of danbi namespace

#endif 
