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

  Kernel.cpp -- compute kernel implementation
 */
#include "Support/AbstractReserveCommitQueue.h"
#include "Core/Kernel.h"
#include "Core/AbstractCode.h"
#include "Core/QueueInfo.h"
#include "Core/Program.h"
#include "Core/ReadOnlyBuffer.h"
#include "Core/QueueInfo.h"

using namespace danbi; 

Kernel::Kernel(Program& Pgm_, ExecutionKind ExecKind_, bool Start_, 
               int RegularInCount_, int RegularOutCount_)
  : Pgm(Pgm_), Code(NULL), ExecKind(ExecKind_), Start(Start_), 
    SubQueuePopKind(Unknown), SubQueuePushKind(Unknown), 
    RegularInCount(RegularInCount_), RegularOutCount(RegularOutCount_), 
    Terminated(false), ActiveNonFeedbackInputQueues(0) {}

inline QueueInfo* Kernel::getQueueInfo(AbstractReserveCommitQueue* Queue_) {
  return Pgm.QueueInfoTable[Queue_->getIndex()];
}

int Kernel::setCode(AbstractCode* Code_) {
  int Index = Code_->getIndex(); 
  if ( !((0 <= Index) && (Index < Pgm.CodeTable.size())) )
    return -EINVAL; 
  
  Code = Pgm.CodeTable[Index]; 
  return 0; 
}

int Kernel::addInputQueue(AbstractReserveCommitQueue* Queue_) {
  int Index = Queue_->getIndex(); 
  if ( !((0 <= Index) && (Index < Pgm.QueueInfoTable.size())) )
    return -EINVAL; 
  
  InQueueTable.push_back( Pgm.QueueInfoTable[Index]->Queue ); 
  return 0; 
}

int Kernel::addOutputQueue(AbstractReserveCommitQueue* Queue_) {
  int Index = Queue_->getIndex(); 
  if ( !((0 <= Index) && (Index < Pgm.QueueInfoTable.size())) )
    return -EINVAL; 
  
  OutQueueTable.push_back( Pgm.QueueInfoTable[Index]->Queue ); 
  return 0; 
}

int Kernel::addReadOnlyBuffer(ReadOnlyBuffer* ROB_) {
  int Index = ROB_->getIndex(); 
  if ( !((0 <= Index) && (Index < Pgm.ReadOnlyBufferTable.size())) )
    return -EINVAL; 
  
  ReadOnlyBufferTable.push_back( Pgm.ReadOnlyBufferTable[Index] ); 
  return 0; 
}

bool Kernel::verify() {
  // Common verification 
  if (Code == NULL)
    return false; 

  if ( (InQueueTable.size() == 0) && (OutQueueTable.size() == 0) )
    return false; 

  if (InQueueTable.size() == 0) {
    // TODO: error if it is not the first kernel 
  }

  if (OutQueueTable.size() == 0) {
    // TODO: error if it is not the last kernel 
  }

  // Verification per execution type
  switch(ExecKind) {
  case SeqeuentialExec:
    // In case of sequential kernel, everything is sequential. 
    SubQueuePopKind = SubQueuePushKind = SequentialAccess; 
    RegularInCount = RegularOutCount = 0; 
    break; 
    
  case ParallelExec:
    SubQueuePopKind = ParallelAccess;

    // Check regular input count
    if ( (RegularInCount != 0) && (RegularInCount != 1) )
      return false; 
    RegularInCount = 1; 

    // Check output queue
    SubQueuePushKind = SequentialAccess; // sequential push by default
    if (OutQueueTable.size() == 1) {
      // Try to calculate regular output count if it is unknown
      if (RegularOutCount == 0) {
        // TODO: output count analysis
      }
      
      // If the produce/consume relationship is one-to-one, 
      // parallel push without contention in sub-queue is possible. 
      if (RegularOutCount == 1) 
        SubQueuePushKind = ParallelAccess; 
    }
    break;

  default:
    return false; 
  }

  ActiveNonFeedbackInputQueues = countNonFeedbackInputQueue();
  Machine::wmb();
  return true; 
}

int Kernel::countNonFeedbackInputQueue() {
  int Count = 0; 
  for (std::vector<AbstractReserveCommitQueue*>::iterator
         i = InQueueTable.begin(), e = InQueueTable.end(); 
       i != e; ++i) {
    QueueInfo* QInfo = getQueueInfo(*i); 
    if (!QInfo->isFeedbackQueue) 
      ++Count; 
  }
  return Count; 
}

bool Kernel::areAllNonFeedbackInputQueuesEmpty() { 
  for (std::vector<AbstractReserveCommitQueue*>::iterator
         i = InQueueTable.begin(), e = InQueueTable.end(); 
       i != e; ++i) {
    QueueInfo* QInfo = getQueueInfo(*i); 
    if (!QInfo->isFeedbackQueue && !(*i)->empty()) {
      assert(QInfo->isDeactivated && "Inconsistent queue status is detected.");
      return false; 
    }
  }
  return true; 
}

void Kernel::deactivateOutputQueues() {
  // Deactivate output queues
  for (std::vector<AbstractReserveCommitQueue*>::iterator
         i = OutQueueTable.begin(), e = OutQueueTable.end(); 
       i != e; ++i) {
    // Deactivate
    QueueInfo* QInfo = getQueueInfo(*i); 
    QInfo->isDeactivated = true; 

    // If it is not a feedback queue, decrement the active count. 
    if (!QInfo->isFeedbackQueue) {
      Kernel* Consumer = Pgm.KernelTable[QInfo->second];
      Machine::atomicIntDec<volatile int>(&Consumer->ActiveNonFeedbackInputQueues);
    }
  }

  // Publish the changed status immediately 
  Machine::wmb();
}

bool Kernel::isInputQueueDeactivated(int Index) {
  QueueInfo* QInfo = getQueueInfo( resolveInputQueue(Index) ); 
  return QInfo->isDeactivated;
}

bool Kernel::isInputQueueFeedbackLoop(int Index) {
  QueueInfo* QInfo = getQueueInfo( resolveInputQueue(Index) ); 
  return QInfo->isFeedbackQueue;
}
