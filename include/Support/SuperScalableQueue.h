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

  SuperScalableQueue.h -- Super scalable FIFO queue with permanent sentinel node
  Supporting multiple enqueuers and multiple dequeuers
 */
#ifndef DANBI_SUPER_SCALABLE_QUEUE_H
#define DANBI_SUPER_SCALABLE_QUEUE_H
#include <cstdlib>
#include <cerrno>
#include <stddef.h>
#include "Support/Machine.h"
#include "Support/MCSLock.h"

namespace danbi {
#define DANBI_SSQ_ITERABLE(Ty)                          \
  public:                                               \
  volatile Ty* __Next;                    

template <typename Ty> 
class SuperScalableQueue {
private:  
  enum {
    PermanentSentinelSize = offsetof(Ty, __Next) + sizeof(Ty*), 
  };

  Ty* const Head __cacheline_aligned; 
  char FakePermanentSentinel[PermanentSentinelSize]; 
  MCSLock PopLock; 

  volatile Ty* Tail __cacheline_aligned; 

  void operator=(const SuperScalableQueue<Ty>&); // Do not implement
  SuperScalableQueue(const SuperScalableQueue<Ty>&); // Do not implement

#define ENABLE_SSQ_DEBUG
#ifdef ENABLE_SSQ_DEBUG
public:
  volatile int __NumElm; 
#endif 
public:
  /// Constructor 
  SuperScalableQueue() 
    : Head(reinterpret_cast<Ty*>(FakePermanentSentinel)), 
      Tail(Head) {
    Head->__Next = NULL; 

#ifdef ENABLE_SSQ_DEBUG
    __NumElm = 0; 
#endif 
    Machine::mb(); 
  }

  /// Push a node
  void push(Ty* Node) {
    Node->__Next = NULL; 
    Machine::wmb(); 
    
    // Insert tail and publish the update by linking a new to the Next of old tail. 
    Ty* OldTail = const_cast<Ty*>( 
      Machine::fasWord<volatile Ty*>((volatile Ty**)&Tail, Node) ); 
    OldTail->__Next = Node; 

#ifdef ENABLE_SSQ_DEBUG
    Machine::atomicIntInc<volatile int>(&__NumElm);
#endif 
  }

  /// Pop a node
  int pop(Ty*& Node) {
    // Is queue empty?
    Machine::rmb(); 
    if (Head == Tail)
      return -EWOULDBLOCK;

    // Acquire poplock 
    MCSLock::QNode QN;
    PopLock.lock(QN);

    // Pop a node 
    int ret; 
    do { 
      // Is queue empty?
      if (Head == Tail) {
        ret = -EWOULDBLOCK;
        break; 
      }
      
      // If concurrent euqueue of the first node is in progress, 
      // wait until it completes. 
      while (Head->__Next == NULL) Machine::rmb();
      
      // When only one item is in a queue
      Ty* tail = const_cast<Ty*>(Tail); 
      // - Since only this thread can pop() at this time, no ABA problem occurs. 
      if (tail == Head->__Next) { 
        // Get the last element by making queue empty, moving tail to head. 
        Ty* next = const_cast<Ty*>(Head->__Next); 
        if ( Machine::casWord<volatile Ty*>((volatile Ty**)&Tail, tail, Head) ) {
          // After updating Tail, anther push() can update Head->__Next to non-NULL
          // before this thread's updating it to NULL. 
          Machine::casWord<volatile Ty*>((volatile Ty**)&Head->__Next, next, NULL); 
          Node = next; 
          ret = 0; 
          break; 
        }
        // Tail is updated, i.e. another push() happened. 
        // Therefore, now, it has two or more. 
      }
      
      // When two or more items is in a queue
      // If concurrent enqueue of the second node is in progress, 
      // wait until it completes. 
      while (Head->__Next->__Next == NULL) Machine::rmb(); 
      
      // Update Head->Next
      Node = const_cast<Ty*>(Head->__Next);
      Head->__Next = Head->__Next->__Next; 
      ret = 0; 
    } while(false); 
#ifdef ENABLE_SSQ_DEBUG
    if (ret == 0)
      Machine::atomicIntDec<volatile int>(&__NumElm);
#endif 
    
    // Release poplock 
    PopLock.unlock(QN);
    return ret; 
  }

  /// Test if it is empty
  bool empty() {
    Machine::rmb(); 
    return Head == Tail; 
  }

  /// Peek pop
  volatile Ty* peekPop() {
    Machine::rmb();
    return Head->__Next; 
  }
}; 

} // End of danbi namespace 

#endif 
