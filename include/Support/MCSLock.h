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

  MCSLock.h -- MCS queue lock
 */
#ifndef DANBI_MCS_LOCK_H
#define DANBI_MCS_LOCK_H
#include "Support/Machine.h"

namespace danbi {

class MCSLock {
public:
  struct QNode {
    volatile int Locked; 
    volatile QNode* Next; 
  }  __cacheline_aligned;

private:
  volatile QNode* Lock  __cacheline_aligned; 
  void operator=(const MCSLock&); // Do not implement
  MCSLock(const MCSLock&); // Do not implement

public:
  inline MCSLock(): Lock(NULL) {
    Machine::wmb();
  }

  inline void lock(QNode &QN) {
    QN.Locked = 1;
    QN.Next = NULL; 
    Machine::wmb(); 
    
    QNode* Predecessor = const_cast<QNode*>(
      Machine::fasWord<volatile QNode*>(&Lock, const_cast<volatile QNode*>(&QN)) ); 
    if (Predecessor) {
      Predecessor->Next = &QN; 
      Machine::wmb();
      while (QN.Locked) Machine::rmb(); 
    }
  }

  inline void unlock(QNode &QN) {
    if (!QN.Next) {
      if ( Machine::casWord<volatile QNode*>(&Lock, const_cast<volatile QNode*>(&QN), NULL) )
        return; 
      while (!QN.Next) Machine::rmb(); 
    }
    QN.Next->Locked = 0; 
    Machine::wmb();
  }
} __cacheline_aligned;

} // End of danbi namespace
#endif 
