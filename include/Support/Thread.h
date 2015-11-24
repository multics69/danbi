/*                                                                 --*- C++ -*-
  Copyright (C) 2012, 2013 Changwoo Min. All Rights Reserved.

  This file is part of DANBI project. 

  NOTICE: All information contained herein is, and remains the property 
  of Changwoo Min. The intellectual and technical concepts contained 
  herein are proprietary to Changwoo Min and may be covered  by patents 
  or patents in process, and are protected by trade secret or copyright law. 
  Dissemination of this information or reproduction of this material is 
  strictly forbidden unless prior written permission is obtained 
  from Changwoo Min(multics69@gmail.com). 

  Thread.h -- Thread class 
 */
#ifndef DANBI_THREAD_H
#define DANBI_THREAD_H
#include <cassert>
#include <pthread.h>
#include <limits.h>
#include <errno.h>

namespace danbi {
class AbstractRunnable; 

class Thread {
  friend class AbstractRunnable; 
private:
  static __thread Thread* This; 
  static __thread int ID; 
  AbstractRunnable& Runnable; 
  ::pthread_t NativeThread;  
  ::cpu_set_t CPUSet;
  int StackSize; 
  int CPUID; 

  void operator=(const Thread&); // Do not implement
  Thread(const Thread&); // Do not implement

public:
  enum {
    MinStackSize = PTHREAD_STACK_MIN, 
  }; 

  /// Constractor
  Thread(AbstractRunnable& Runnable_, int StackSize_=0);

  /// Destructor
  virtual ~Thread();

  /// set CPU affinity
  int setAffinity(int NumID, ...); 

  /// get CPU affinity
  int getAffinity(); 

  /// Start runnable
  int start(); 
  
  /// Wait until for this thread to die
  int join(); 

  /// get thread ID
  static int getID(); 

  /// get instance of running thread
  static inline Thread* getCurrent() {
    return This;
  }
};

} // End of danbi namespace
#endif 
