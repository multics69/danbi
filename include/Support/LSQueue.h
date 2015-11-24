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

  LSQueue.h -- Ladan-Mozes-Shavit Queue
 */
#ifndef DANBI_LS_QUEUE_H
#define DANBI_LS_QUEUE_H
#include <cstdlib>
#include <cerrno>
#include <stddef.h>
#include "Support/Machine.h"
#include "Support/TaggedPointer.h"
#include "Support/BackoffDelay.h"

namespace danbi {
template <typename Ty> 
struct LSQNode {
  TaggedPointer< LSQNode<Ty> > Next; 
  TaggedPointer< LSQNode<Ty> > Prev; 
  Ty* Value; 
}; 

template <typename Ty> 
class LSQueue {
private:
  TaggedPointer< LSQNode<Ty> > Head;
  TaggedPointer< LSQNode<Ty> > Tail;

  void operator=(const LSQueue<Ty>&); // Do not implement
  LSQueue(const LSQueue<Ty>&); // Do not implement

  void fixList(TaggedPointer< LSQNode<Ty> >& tail, TaggedPointer< LSQNode<Ty> >& head) {
    TaggedPointer< LSQNode<Ty> > curNode; 
    TaggedPointer< LSQNode<Ty> > curNodeNext; 

    curNode = tail; 
    while ( Machine::cas2Word< TaggedPointer< LSQNode<Ty> > >(&Head, head, head) &&
            (curNode != head) ) {
      Machine::atomicWrite2Word< TaggedPointer< LSQNode<Ty> > >(
        &curNodeNext, curNode.P->Next);
      TaggedPointer< LSQNode<Ty> > NewTag(curNode.P, curNode.T - 1);
      Machine::atomicWrite2Word< TaggedPointer< LSQNode<Ty> > >(
        const_cast< TaggedPointer< LSQNode<Ty> >* >(&curNodeNext.P->Prev), NewTag); 
  
      curNode.P = curNodeNext.P; 
     --curNode.T;
    }
  }

public:
  /// Constructor 
  LSQueue() {
    LSQNode<Ty>* NewNode = new LSQNode<Ty>(); 
    NewNode->Next.P = NULL; 
    NewNode->Next.T = 0; 
    TaggedPointer< LSQNode<Ty> > NewTag(NewNode, 0); 
    Head = Tail = NewTag; 
    Machine::mb(); 
  }
  
  /// Destructor
  /// TODO: delete all the LSQNode

  /// Push a node
  void push(Ty* Node) {
    TaggedPointer< LSQNode<Ty> > tail; 
    LSQNode<Ty>* NewNode = new LSQNode<Ty>(); 
    NewNode->Value = Node; 
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
      Machine::atomicRead2Word< TaggedPointer< LSQNode<Ty> > >(&Tail, &tail); 
      TaggedPointer< LSQNode<Ty> > NewNodeTag(tail.P, tail.T + 1); 
      NewNode->Next = NewNodeTag; 
      TaggedPointer< LSQNode<Ty> > NewTailTag(NewNode, tail.T + 1);
      if ( Machine::cas2Word< TaggedPointer< LSQNode<Ty> > >(&Tail, tail, NewTailTag) ) {
        TaggedPointer< LSQNode<Ty> > NewPrevTag(NewNode, tail.T);
        const_cast< LSQNode<Ty>* >(tail.P)->Prev = NewPrevTag; 
        break; 
      }
    }
  }

  /// Pop a node
  int pop(Ty*& Node) {
    TaggedPointer< LSQNode<Ty> > head;
    TaggedPointer< LSQNode<Ty> > tail;
    TaggedPointer< LSQNode<Ty> > firstNodePrev; 

#ifdef ENABLE_BACKOFF_DELAY
    int Retry = 0; 
#endif
    for (;;) {
#ifdef ENABLE_BACKOFF_DELAY
      // Backoff dealy if needed
      BackoffDelay::delay(Retry++); 
#endif

      // Read vars. 
      Machine::atomicRead2Word< TaggedPointer< LSQNode<Ty> > >(&Head, &head); 
      Machine::atomicRead2Word< TaggedPointer< LSQNode<Ty> > >(&Tail, &tail); 
      Machine::atomicRead2Word< TaggedPointer< LSQNode<Ty> > >(
        const_cast< TaggedPointer<LSQNode<Ty>>* >(&head.P->Prev), &firstNodePrev); 
      // Is head still consistent?
      if ( Machine::cas2Word< TaggedPointer< LSQNode<Ty> > >(&Head, head, head) ) {
        // Queue not empty?
        if (tail != head) {
          if (firstNodePrev.T != head.T) {
            fixList(tail, head); 
            continue; 
          }
          Node = firstNodePrev.P->Value; 
          TaggedPointer< LSQNode<Ty> > NewTag(firstNodePrev.P, head.T + 1); 
          if ( Machine::cas2Word< TaggedPointer< LSQNode<Ty> > >(&Head, head, NewTag) ) {
            delete head.P; 
            break; 
          }
        }
        else
          return -EWOULDBLOCK;
      }
    }
    return 0; 
  }
}; 

} // End of danbi namespace 

#endif 
