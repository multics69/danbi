/*                                                                 --*- C++ -*-
  Copyright (C) 2013 Changwoo Min. All Rights Reserved.

  This file is part of DANBI project. 

  NOTICE: All information contained herein is, and remains the property 
  of Changwoo Min. The intellectual and technical concepts contained 
  herein are proprietary to Changwoo Min and may be covered  by patents 
  or patents in process, and are protected by trade secret or copyright law. 
  Dissemination of this information or reproduction of this material is 
  strictly forbidden unless prior written permission is obtained 
  from Changwoo Min(multics69@gmail.com). 

  CuBoQueue.h -- Combining pOp and Concurrent pUsh Queue
 */
#ifndef DANBI_CUBO_QUEUE_H
#define DANBI_CUBO_QUEUE_H
#include <cstdlib>
#include <cerrno>
#include <cassert>
#include <stddef.h>
#include <malloc.h>
#include <limits.h>
#include <string.h>
#include "Support/Machine.h"
#include "Support/BranchHint.h"
#include "Support/NUMACLHLock.h"
#include "Support/PerCPUData.h"
#include "Support/Thread.h"

#define DANBI_CUBOQ_COLLECT_STAT 1 // should be 0 at the production mode 
#define DANBI_CUBOQ_COLLECT_STAT_TIME 1 // should be 0 at the production mode 
#define DANBI_CUBOQ_COLLECT_WAITING 0 // should be 0 at the production mode 
#define DANBI_CUBOQ_NO_POP_COMBINE 0 // should be 0 at the production mode
#define DANBI_CUBOQ_NO_CLUSTER_OPT 0 // should be 0 at the production mode

#if DANBI_CUBOQ_COLLECT_STAT
#define DANBI_CUBOQ_STAT(x) x
#else
#define DANBI_CUBOQ_STAT(x)
#endif 

#if DANBI_CUBOQ_COLLECT_STAT && DANBI_CUBOQ_COLLECT_STAT_TIME
#define DANBI_CUBOQ_START_TSC(x) unsigned long long ___tsc = Machine::rdtsc(), ___diff;
#define DANBI_CUBOQ_END_TSC(x)   (x) += (___diff = Machine::rdtsc() - ___tsc);
#define DANBI_CUBOQ_UPDATE_MAX_TSC(x)   if (unlikely(x < ___diff)) (x) = ___diff; 
#else
#define DANBI_CUBOQ_START_TSC(x)
#define DANBI_CUBOQ_END_TSC(x)
#define DANBI_CUBOQ_UPDATE_MAX_TSC(x)
#endif 

namespace danbi {
#define DANBI_CUBOQ_ITERABLE(Ty)                        \
  public:                                               \
  volatile Ty* __Next; 

template <typename Ty> 
class CuBoQueue {
public:
  struct Statistics {
    int PushOps, PopOps; 
    int PopRound; 
    int CASTail;
    int WaitCount; 
    unsigned long long PushClocks, PopClocks, PopCombineClocks; 
    unsigned long long WaitClocks, WaitMaxClocks;
  };
private:  
  enum StatusKind {
    Wait = 0, Done, Combine, 
  }; 

  struct Request {
    volatile Request* Next;
    volatile Ty* Node; 
    volatile int Ret; 
    volatile int Status;  // {StatusKine::Wait, StatusKine::Done, StatusKine::Combine}
  } __cacheline_aligned;

  enum {
    PermanentSentinelSize = offsetof(Ty, __Next) + sizeof(Ty*), 
  };

  int MaxId; 
  int NumCluster; 
  NUMACLHLock HeadLock;
#if !DANBI_CUBOQ_NO_POP_COMBINE
  PerCPUData<volatile Request*> ReqTails;
  PerCPUData<volatile Request*> ReqPool; 
  static volatile int IndexSeed; 
  static __thread int MyClusterID;
#endif 


  volatile Ty* Head __cacheline_aligned; 
  char FakePermanentSentinel[PermanentSentinelSize] __cacheline_aligned; 

  volatile Ty* Tail __cacheline_aligned; 

#if DANBI_CUBOQ_COLLECT_STAT
  static __thread Statistics Stat;
#endif 

  void operator=(const CuBoQueue<Ty>&); // Do not implement
  CuBoQueue(const CuBoQueue<Ty>&); // Do not implement

private:
#if !DANBI_CUBOQ_NO_POP_COMBINE
  inline int genClusterID(void) {
    Thread* Current = Thread::getCurrent(); 
    int cpuid = (Current != NULL) ? Current->getAffinity() : -1;

    if (likely(cpuid >= 0))
      return cpuid % NumCluster; 
    else
      return Machine::atomicIntInc<volatile int>(&IndexSeed) % NumCluster;
  }


  inline Request &addRequest(const int id, Ty* Node) {
    Request* NextReq = const_cast<Request*>(ReqPool[id]);
    NextReq->Status = StatusKind::Wait; 
    NextReq->Next = NULL; 
    
    Request* CurReq = const_cast<Request*>(
      Machine::fasWord<volatile Request*>(
        (volatile Request**)&ReqTails[MyClusterID], 
        (volatile Request*)NextReq) );
    CurReq->Node = Node; 
    CurReq->Ret = 0;
    Machine::cmb();
    CurReq->Next = NextReq; 
    return *CurReq;
  }

  inline void delRequest(Request &Req, const int id) {
    ReqPool[id] = &Req;
  }

  inline void wakeupNextCombiner(Request &Req, const int id) {
    assert(Req.Status != StatusKind::Done);
    Req.Next->Status = StatusKind::Combine; // Let the next become combiner.
    Req.Status = StatusKind::Done; // After accessing req->next, req itself can be released. 
  }

  inline void combinePopRequests(Request &StartReq, const int id) {
    Request *Req = &StartReq, *PrefetchReq = &StartReq; 
    Request *Req_Next, *Req_Next_Next, *LastReq, *LastNodeReq = NULL; 
    Ty *head_Next, *Node, *Node_Next, *New_Head_Next; 
    NUMACLHLock::QNode *LockNode = NULL;

    // Acquire lock
#if !DANBI_CUBOQ_NO_CLUSTER_OPT
    while(!HeadLock.tryLock(id, &LockNode)) {
      for(; PrefetchReq->Next != NULL; PrefetchReq = const_cast<Request*>(PrefetchReq->Next)) {
        Machine::prefetch(PrefetchReq);
      }
    }
#endif 
    DANBI_CUBOQ_STAT(Stat.PopRound++);
    DANBI_CUBOQ_START_TSC(Stat.PopCombineClocks);

    // Is queue empty?
    if (Head->__Next == NULL) {
      if (Head == Tail) {
        for (; true; Req = Req_Next) {
          
          Req->Ret = -EWOULDBLOCK;
          Req_Next = const_cast<Request*>(Req->Next);
          if (Req_Next->Next == NULL) {
            LastReq = Req;
            goto end_of_combine;
          }
          Machine::prefetch(Req_Next->Next);
          Req->Status = StatusKind::Done; 
        }
        assert(0); // Never be here!
      }
      else {
#if DANBI_CUBOQ_COLLECT_WAITING
        if (unlikely(Head->__Next == NULL)) {
          DANBI_CUBOQ_STAT(Stat.WaitCount++);
          DANBI_CUBOQ_START_TSC(Stat.WaitClocks);
          while(Head->__Next == NULL) 
            ; // Concurrent push is in progress.
          DANBI_CUBOQ_END_TSC(Stat.WaitClocks);
          DANBI_CUBOQ_UPDATE_MAX_TSC(Stat.WaitMaxClocks);
        }
#else 
        while(Head->__Next == NULL) 
          ; // Concurrent push is in progress.
#endif 
      }
    }
    Machine::prefetch(Head->__Next);
    head_Next = const_cast<Ty*>(Head->__Next);

    // Consume queue items if non-empty. 
    for (Node = head_Next; 
         true; 
         Req = Req_Next, Node = Node_Next) {
      Req->Node = Node; 
      Req_Next = const_cast<Request*>(Req->Next);
      Machine::cmb();
    test:
      Node_Next = const_cast<Ty*>(Node->__Next);
      Req_Next_Next = const_cast<Request*>(Req_Next->Next);
      Machine::cmb();
      if (Node_Next == NULL && Req_Next_Next != NULL) {
        if (Node != Tail) {
          Machine::mb();
          goto test;
        }
        LastNodeReq = Req; 
        for (Req = Req_Next; true; Req = Req_Next) {
          Req->Ret = -EWOULDBLOCK; 
          Req_Next = const_cast<Request*>(Req->Next);
          if (Req_Next->Next == NULL) {
            LastReq = Req;
            goto end_of_consume_loop;
          }
          Machine::prefetch(Req_Next->Next);
          Req->Status = StatusKind::Done;
        }
        assert(0); // Never be here!
      }
      // LastReq and LastNodeReq should be set Done after updating Head->__Next.
      if (Req_Next_Next == NULL) {
        LastNodeReq = Req; 
        LastReq = Req;
        goto end_of_consume_loop;
      }
      assert(Node_Next != NULL);
      Machine::prefetch(Node_Next);
      Machine::prefetch(Req_Next->Next);
      Req->Status = StatusKind::Done; 
    }
  end_of_consume_loop:
    // LastReq and LastNodeReq should be set Done after updating Head->__Next. 
    assert(LastReq->Status != StatusKind::Done); 
    assert(LastNodeReq->Status != StatusKind::Done); 

    // When all items are popped, update Head->__Next and Tail.
    if (LastNodeReq->Node->__Next == NULL) {
      DANBI_CUBOQ_STAT(Stat.CASTail++);

      Head->__Next = NULL;
      if ( Machine::casWord<volatile Ty*>(
             (volatile Ty**)&Tail, LastNodeReq->Node, Head) ) {
        // After updating Tail, anther push() can update Head->__Next to non-NULL
        // before this thread's updating it to NULL. 
        if (LastNodeReq != LastReq)
          LastNodeReq->Status = StatusKind::Done;
        goto end_of_combine;
      }
      // Tail is updated, i.e. another push() happened. 
      // Two or more items are in the queue. 
      // When concurrent push() just after LastNode is in progress, 
      // wait until it completes. 
#if DANBI_CUBOQ_COLLECT_WAITING
      if (unlikely(LastNodeReq->Node->__Next == NULL)) {
          DANBI_CUBOQ_STAT(Stat.WaitCount++);
          DANBI_CUBOQ_START_TSC(Stat.WaitClocks);
          while (LastNodeReq->Node->__Next == NULL) 
            ;
          DANBI_CUBOQ_END_TSC(Stat.WaitClocks);
          DANBI_CUBOQ_UPDATE_MAX_TSC(Stat.WaitMaxClocks);
        }
#else 
        while (LastNodeReq->Node->__Next == NULL) 
          ;
#endif 
    }

    // Update Head->__Next
    New_Head_Next = const_cast<Ty*>(LastNodeReq->Node->__Next);
    if (LastNodeReq != LastReq)
      LastNodeReq->Status = StatusKind::Done;
    Head->__Next = New_Head_Next;

  end_of_combine:
    wakeupNextCombiner(*LastReq, id);

    // Release lock
#if !DANBI_CUBOQ_NO_CLUSTER_OPT
    HeadLock.unlock(id);
#endif 
    DANBI_CUBOQ_END_TSC(Stat.PopCombineClocks);
  }
#endif // DANBI_CUBOQ_NO_POP_COMBINE

public:
  /// Constructor 
  CuBoQueue(int NumCluster_ = 4, int MaxId_ = 256) 
    : Head(reinterpret_cast<Ty*>(FakePermanentSentinel)), 
      MaxId(MaxId_), 
      NumCluster(NumCluster_), 
      HeadLock(MaxId), 
      Tail(Head) {
    Head->__Next = NULL; 

#if DANBI_CUBOQ_NO_CLUSTER_OPT
    NumCluster = 1; 
#endif 

#if !DANBI_CUBOQ_NO_POP_COMBINE
    ReqTails.initialize(NumCluster);
    for (int i = 0; i < NumCluster; ++i) {
      ReqTails[i] = (volatile Request*)::memalign(Machine::CachelineSize, sizeof(Request));
      ReqTails[i]->Status = StatusKind::Combine; 
    }

    ReqPool.initialize(MaxId);
    for (int i = 0; i < MaxId; ++i)
      ReqPool[i] = NULL;
#endif // DANBI_CUBOQ_NO_POP_COMBINE
  }

  /// Destructor 
  ~CuBoQueue() {
#if !DANBI_CUBOQ_NO_POP_COMBINE
    for (int i = 0; i < NumCluster; ++i) {
      if (ReqTails[i])
        ::free((void*)ReqTails[i]);
    }
    for (int i = 0; i < MaxId; ++i) {
      if (ReqPool[i])
        ::free((void*)ReqPool[i]);
    }
#endif 
  }

  /// Register a thread
  void registerThread(const int id) {
    HeadLock.registerThread(id);
#if !DANBI_CUBOQ_NO_POP_COMBINE
    MyClusterID = genClusterID();
    ReqPool[id] = (volatile Request*)::memalign(Machine::CachelineSize, sizeof(Request));
#endif 
  }

  /// Push a node
  inline void push(Ty* Node, const int id) {
    DANBI_CUBOQ_START_TSC(Stat.PushClocks);
    Node->__Next = NULL; 
    Machine::cmb(); 
    
    // Insert tail and publish the update by linking a new to the Next of old tail. 
    Ty* OldTail = const_cast<Ty*>( 
      Machine::fasWord<volatile Ty*>((volatile Ty**)&Tail, Node) ); 
    OldTail->__Next = Node; 

    DANBI_CUBOQ_END_TSC(Stat.PushClocks);
    DANBI_CUBOQ_STAT(Stat.PushOps++);
  }

  /// Pop a node
#if !DANBI_CUBOQ_NO_POP_COMBINE
  int pop(Ty*& Node, const int id) {
    DANBI_CUBOQ_START_TSC(Stat.PopClocks);
    Request &Req = addRequest(id, NULL); 
    DANBI_CUBOQ_STAT(Stat.PopOps++);
    while (Req.Status == StatusKind::Wait)
      ;
    if (Req.Status == StatusKind::Combine) 
      combinePopRequests(Req, id);
    Node = const_cast<Ty*>(Req.Node); 
    int Ret = Req.Ret;
    delRequest(Req, id);
    DANBI_CUBOQ_END_TSC(Stat.PopClocks);
    return Ret;
  }
#else
  int pop(Ty*& Node, const int id) {
    DANBI_CUBOQ_STAT(Stat.PopOps++);
    DANBI_CUBOQ_STAT(Stat.PopRound++);
    DANBI_CUBOQ_START_TSC(Stat.PopClocks);

    // Is queue empty?
    Machine::rmb(); 
    if (Head == Tail) {
      DANBI_CUBOQ_END_TSC(Stat.PopClocks);
      return -EWOULDBLOCK;
    }

    // Acquire headlock 
    HeadLock.lock(id);

    // Is queue empty?
    if (Head == Tail) {
      HeadLock.unlock(id);
      DANBI_CUBOQ_END_TSC(Stat.PopClocks);
      return -EWOULDBLOCK;
    }

    // If concurrent push of the first node is in progress, 
    // wait until it completes. 
    while (Head->__Next == NULL) Machine::rmb();

    Ty* head_next = const_cast<Ty*>(Head->__Next); 
    if (head_next->__Next == NULL) {
      // If only one element is in the queue
      if (Tail == head_next) {
        // If it is popping the last element, move Tail to Head. 
        DANBI_CUBOQ_STAT(Stat.CASTail++);
        Head->__Next = NULL;
        if ( Machine::casWord<volatile Ty*>((volatile Ty**)&Tail, head_next, Head) ) {
          // After updating Tail, anther push() can update Head->__Next to non-NULL
          // before this thread's updating it to NULL. 
          goto end;
        }
        // Tail is updated, i.e., another push happened. 
        // Therefore, it has at least two elements. 
      }
      // If the concurrent push of the second element is in progress, 
      // wait until it completes. 
      while (head_next->__Next == NULL) Machine::rmb(); 
    }
    Head->__Next = head_next->__Next;
  end:    
    Node = head_next; 

    // Release headlock
    HeadLock.unlock(id);
    DANBI_CUBOQ_END_TSC(Stat.PopClocks);
    return 0; 
  }
#endif 

  /// Get statistics
  inline void getStat(Statistics& Stat_) {
    DANBI_CUBOQ_STAT(Stat_ = Stat);
  }
} __cacheline_aligned; 

#if !DANBI_CUBOQ_NO_POP_COMBINE
template <typename Ty> 
  volatile int CuBoQueue<Ty>::IndexSeed;

template <typename Ty> 
  __thread int CuBoQueue<Ty>::MyClusterID; 
#endif 

#if DANBI_CUBOQ_COLLECT_STAT
template <typename Ty> 
  __thread typename CuBoQueue<Ty>::Statistics CuBoQueue<Ty>::Stat;
#endif
} // End of danbi namespace 

#endif 
