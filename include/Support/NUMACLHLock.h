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

  NUMACLHLock.h -- NUMACLH queue lock
 */
#ifndef DANBI_NUMACLH_LOCK_H
#define DANBI_NUMACLH_LOCK_H
#include <cstdlib>
#include <malloc.h>
#include "Support/Machine.h"
#include "Support/Thread.h"
#include "Support/PerCPUData.h"

namespace danbi {
class NUMACLHLock {
public:
  struct QNode {
    volatile int Locked; 
    volatile QNode* Pred;
  }  __cacheline_aligned;

private:
  struct CAQNodePtr {
    volatile QNode* Ptr; 
  } __cacheline_aligned;

  int MaxThrId; 
  CAQNodePtr* MyNode; 
  volatile QNode* Tail  __cacheline_aligned; 

  void operator=(const NUMACLHLock&); // Do not implement
  NUMACLHLock(const NUMACLHLock&); // Do not implement

public:
  inline NUMACLHLock(int MaxThrId_ = 256) 
    : MaxThrId(MaxThrId_) {
    // Init. tail 
    Tail = (QNode *)::memalign(Machine::CachelineSize, sizeof(*Tail)); 
    Tail->Locked = 0; 

    // Init. qnode arrays
    MyNode = (CAQNodePtr*)::memalign(
      Machine::CachelineSize, sizeof(*MyNode) * MaxThrId); 
    ::memset(MyNode, 0, sizeof(*MyNode) * MaxThrId); 
  }
  
  inline ~NUMACLHLock() {
    ::free((void*)Tail); 
    for(int i = 0; i < MaxThrId; ++i) {
      if (MyNode[i].Ptr)
        ::free((void*)MyNode[i].Ptr);
    }
  }

  void registerThread(const int id) {
      MyNode[id].Ptr = (volatile QNode *)::memalign(Machine::CachelineSize, sizeof(*Tail)); 
  }

  inline void lock(const int id) {
    QNode *Node = const_cast<QNode*>(MyNode[id].Ptr);
    Node->Locked = 1; 
    Machine::cmb();
    Node->Pred = const_cast<QNode*>(
      Machine::fasWord<volatile QNode*>(&Tail, const_cast<volatile QNode*>(Node)) ); 
    QNode *Pred = const_cast<QNode*>(Node->Pred);
    while (Pred->Locked) ;
  }

  inline bool tryLock(const int id, QNode **LockNode) {
    if (*LockNode == NULL) {
      QNode *Node = const_cast<QNode*>(MyNode[id].Ptr);
      Node->Locked = 1; 
      Machine::cmb();
      Node->Pred = const_cast<QNode*>(
        Machine::fasWord<volatile QNode*>(&Tail, const_cast<volatile QNode*>(Node)) ); 
      *LockNode = const_cast<QNode*>(Node->Pred);
    }
    return !(*LockNode)->Locked;
  }

  inline void unlock(const int id) {
    QNode *Node = const_cast<QNode*>(MyNode[id].Ptr);
    QNode *Pred = const_cast<QNode*>(Node->Pred);
    Node->Locked = 0;
    MyNode[id].Ptr = Pred;
  }
} __cacheline_aligned;
} // End of danbi namespace
#endif 
