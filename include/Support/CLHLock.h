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

  CLHLock.h -- CLH queue lock
 */
#ifndef DANBI_CLH_LOCK_H
#define DANBI_CLH_LOCK_H
#include <cstdlib>
#include <malloc.h>
#include "Support/Machine.h"
#include "Support/Thread.h"
#include "Support/PerCPUData.h"

namespace danbi {
class CLHLock {
private:
  struct QNode {
    volatile int Locked; 
    volatile QNode* Pred;
  }  __cacheline_aligned;

  int MaxThrId; 
  QNode* QNodePool;
  PerCPUData<volatile QNode*> MyNode; 
  volatile QNode* Tail  __cacheline_aligned; 

  void operator=(const CLHLock&); // Do not implement
  CLHLock(const CLHLock&); // Do not implement

public:
  inline CLHLock(int MaxThrId_ = 256) 
    : MaxThrId(MaxThrId_) {
    // Alloc pool
    QNodePool = (QNode *)::memalign(
      Machine::CachelineSize, sizeof(*QNodePool) * (MaxThrId + 1)); 

    // Init. tail 
    Tail = &QNodePool[0];
    Tail->Locked = 0; 

    // Init. qnode arrays
    MyNode.initialize(MaxThrId);
    for (int i = 0; i < MaxThrId; ++i)
      MyNode[i] = &QNodePool[i+1];
  }
  
  inline ~CLHLock() {
    ::free(QNodePool);
  }

  inline void lock(const int id) {
    QNode *Node = const_cast<QNode*>(MyNode[id]);
    Node->Locked = 1; 
    Machine::cmb();
    Node->Pred = const_cast<QNode*>(
      Machine::fasWord<volatile QNode*>(&Tail, const_cast<volatile QNode*>(Node)) ); 
    QNode *Pred = const_cast<QNode*>(Node->Pred);
    while (Pred->Locked) ;
  }

  inline void unlock(const int id) {
    QNode *Node = const_cast<QNode*>(MyNode[id]);
    QNode *Pred = const_cast<QNode*>(Node->Pred);
    Node->Locked = 0;
    MyNode[id] = Pred;
  }
} __cacheline_aligned;
} // End of danbi namespace
#endif 
