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

  MSQueue.h -- Michael-Scott Queue
 */
#ifndef DANBI_MS_QUEUE_H
#define DANBI_MS_QUEUE_H
#include <cstdlib>
#include <cerrno>
#include <stddef.h>
#include "Support/Machine.h"
#include "Support/TaggedPointer.h"
#include "Support/BackoffDelay.h"

namespace danbi {
template <typename Ty> 
struct MSQNode {
  TaggedPointer< MSQNode<Ty> > Next; 
  Ty* Value; 
}; 

template <typename Ty> 
class MSQueue {
private:
  TaggedPointer< MSQNode<Ty> > Head;
  TaggedPointer< MSQNode<Ty> > Tail;

  void operator=(const MSQueue<Ty>&); // Do not implement
  MSQueue(const MSQueue<Ty>&); // Do not implement

public:
  /// Constructor 
  MSQueue() {
    MSQNode<Ty>* NewNode = new MSQNode<Ty>(); 
    NewNode->Next.P = NULL; 
    NewNode->Next.T = 0; 
    TaggedPointer< MSQNode<Ty> > NewTag(NewNode, 0); 
    Head = Tail = NewTag; 
    Machine::mb(); 
  }
  
  /// Destructor
  /// TODO: delete all the MSQNode

  /// Push a node
  void push(Ty* Node) {
    TaggedPointer< MSQNode<Ty> > tail; 
    TaggedPointer< MSQNode<Ty> > next; 
    MSQNode<Ty>* NewNode = new MSQNode<Ty>(); 
    NewNode->Value = Node; 
    NewNode->Next.P = NULL; 
    NewNode->Next.T = 0; 
    Machine::wmb(); 

#ifdef DANBI_ENABLE_BACKOFF_DELAY
    int Retry = 0; 
#endif
    for(;;) {
#ifdef DANBI_ENABLE_BACKOFF_DELAY
      // Backoff dealy if needed
      BackoffDelay::delay(Retry++); 
#endif

      // Read vars. 
      Machine::atomicRead2Word< TaggedPointer< MSQNode<Ty> > >(&Tail, &tail); 
      Machine::atomicRead2Word< TaggedPointer< MSQNode<Ty> > >(
        const_cast< TaggedPointer<MSQNode<Ty>>* >(&tail.P->Next), &next); 
      // Is tail still consistent?
      if ( Machine::cas2Word< TaggedPointer< MSQNode<Ty> > >(&Tail, tail, tail) ) {
        // Was tail pointing to the last node?
        if (next.P == NULL) {
          TaggedPointer< MSQNode<Ty> > NewTag(NewNode, next.T + 1); 
          // Try to link the new node at the end
          if ( Machine::cas2Word< TaggedPointer< MSQNode<Ty> > >(
                 const_cast< TaggedPointer<MSQNode<Ty>>* >(&tail.P->Next), next, NewTag) )
            break; 
        }
        // Tail was not pointing to the last node
        else {
          // Try to swing tail to the next node 
          TaggedPointer< MSQNode<Ty> > NewTag(next.P, tail.T + 1); 
          Machine::cas2Word< TaggedPointer< MSQNode<Ty> > >(&Tail, tail, NewTag); 
        }
      }
    }
    TaggedPointer< MSQNode<Ty> > NewTag(NewNode, tail.T + 1); 
    Machine::cas2Word< TaggedPointer< MSQNode<Ty> > >(&Tail, tail, NewTag); 
  }

  /// Pop a node
  int pop(Ty*& Node) {
    TaggedPointer< MSQNode<Ty> > head;
    TaggedPointer< MSQNode<Ty> > tail;
    TaggedPointer< MSQNode<Ty> > next;

#ifdef ENABLE_BACKOFF_DELAY
    int Retry = 0; 
#endif
    for (;;) {
#ifdef ENABLE_BACKOFF_DELAY
      // Backoff dealy if needed
      BackoffDelay::delay(Retry++); 
#endif

      // Read vars. 
      Machine::atomicRead2Word< TaggedPointer< MSQNode<Ty> > >(&Head, &head); 
      Machine::atomicRead2Word< TaggedPointer< MSQNode<Ty> > >(&Tail, &tail); 
      Machine::atomicRead2Word< TaggedPointer< MSQNode<Ty> > >(
        const_cast< TaggedPointer<MSQNode<Ty>>* >(&head.P->Next), &next); 
      // Is head still consistent?
      if ( Machine::cas2Word< TaggedPointer< MSQNode<Ty> > >(&Head, head, head) ) {
        // Is queue empty or tail falling behind? 
        if (head.P == tail.P) {
          // Is queue empty?
          if (next.P == NULL) 
            return -EWOULDBLOCK; 
          // Tail is falling behind. Try to adbance it. 
          TaggedPointer< MSQNode<Ty> > NewTag(next.P, tail.T + 1); 
          Machine::cas2Word< TaggedPointer< MSQNode<Ty> > >(&Tail, tail, NewTag); 
        }
        // No need to deal with tail 
        else {
          // Read value before CAS, otherwise another pop() might free the next node. 
          Node = next.P->Value; 
          // Try to swing head to the next node. 
          TaggedPointer< MSQNode<Ty> > NewTag(next.P, head.T + 1); 
          if ( Machine::cas2Word< TaggedPointer< MSQNode<Ty> > >(&Head, head, NewTag) )
            break; 
        }
      }
    }
    delete head.P; 
    return 0; 
  }

  /// Test if it is empty
  bool empty() {
    Machine::rmb(); 
    return Head.P == Tail.P;
  }

  /// Peek pop
  volatile Ty* peekPop() {
    Machine::rmb();
    return Head.P->Next; 
  }
}; 

} // End of danbi namespace 

#endif 
