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

  Intrinsics.cpp -- implementation of danbi C intrinsics
 */
#include <cerrno>
#include <string.h>
#include "Core/IntrinsicsCPU.h"
#include "Support/BranchHint.h"
#include "Support/AbstractReserveCommitQueue.h"
#include "Support/GreenScheduler.h"
#include "Support/GreenThread.h"
#include "Core/ReadOnlyBuffer.h"
#include "Core/CPUSchedulePolicyAdapter.h"
#include "Core/SystemContext.h"
#include "Core/Kernel.h"
#include "Core/SchedulingEvent.h"
#include "Core/Runtime.h"
#include "Core/SchedulingHint.h"

using namespace danbi; 
typedef SchedulingHint< GreenScheduler<CPUSchedulePolicyAdapter> > CPUSchedulingHint; 

static inline AbstractReserveCommitQueue* resolveInputQueue(int Index) {
  return SystemContext::RunningKernel->resolveInputQueue(Index); 
}

static inline AbstractReserveCommitQueue* resolveOutputQueue(int Index) {
  return SystemContext::RunningKernel->resolveOutputQueue(Index); 
}

static inline ReadOnlyBuffer* resolveReadOnlyBuffer(int Index) {
  return SystemContext::RunningKernel->resolveReadOnlyBuffer(Index); 
}

static inline bool isInputQueueDeactivated(int Index) {
  return SystemContext::RunningKernel->isInputQueueDeactivated(Index); 
}

static inline bool isInputQueueFeedbackLoop(int Index) {
  return SystemContext::RunningKernel->isInputQueueFeedbackLoop(Index); 
}

#ifdef DANBI_ENABLE_EAGER_THREAD_DESTROY
static inline bool isDiscardable() {
  // We can discard a parallel kernel with no successful queue operation. 
  if (SystemContext::RunningKernel->getExecutionKind() == ExecutionKind::ParallelExec) {
    if (!CPUSchedulingHint::haveSuccessfulQueueOps())
      return true; 
  }

  return false; 
}
#endif 

static inline void yield() {
  static GreenScheduler<CPUSchedulePolicyAdapter>& 
    CPUs = GreenScheduler<CPUSchedulePolicyAdapter>::getInstance(); 
  CPUs.yield(*GreenScheduler<CPUSchedulePolicyAdapter>::RunningThread); 
}

static inline
int reservePeekPop(AbstractReserveCommitQueue* InQ, int InputQueueIndex, 
                   int PeekNumElm, int PopNumElm, int NumRepeat, 
                   QueueWindow (&Win)[2], ReservationInfo& RevInfo, 
                   int ServingTicket, int* IssuingTicket) {
  int TotalPeekElm = PopNumElm * (NumRepeat - 1) + PeekNumElm; 
  int TotalPopElm = PopNumElm * NumRepeat; 
  int ret; 

  assert(PeekNumElm >= PopNumElm); 
  while ( likely((ret = InQ->reservePeekPopN(TotalPeekElm, TotalPopElm, 
                                             Win, RevInfo, 
                                             ServingTicket, IssuingTicket)) < 0) ) {
    // Translate return code to event type 
    if (ret == -EBUSY) {
      SystemContext::Event = SystemContext::DbgOriginalEvent = 
        SchedulingEventKind::NotMyTurn;
    }
    else {
      if ( isInputQueueDeactivated(InputQueueIndex) ) {
        // When the input queue is deactivated, retry with repeat number is 1 
        // in order to fully consume queue elements. 
        if (NumRepeat > 1) {
          int NewRepeat = ((InQ->getNumElement() - PeekNumElm) / PopNumElm) + 1; 
          if (NewRepeat > 0) {
            NumRepeat = std::min(NumRepeat, NewRepeat);
            TotalPeekElm = PopNumElm * (NumRepeat - 1) + PeekNumElm; 
            TotalPopElm = PopNumElm * NumRepeat; 
            continue;
          }
        }

        // When the input queue is deactivated, drain them all to terminate. 
        InQ->drain();
        assert(InQ->empty());
      }

      // Set empty event 
      SystemContext::Event = SystemContext::DbgOriginalEvent = 
        SchedulingEventKind::InputQueueIsEmpty; 
      // If a kernel is in terminating, cancel this thread, 
      // because in terminating state all input queues are already empty. 
      if ( !isInputQueueFeedbackLoop(InputQueueIndex) ) {
        static CPUSchedulePolicyAdapter& 
          Policy = CPUSchedulePolicyAdapter::getInstance(); 
        Policy.checkTerminationCondition(); 
        if (SystemContext::Terminating) {
           return -ECANCELED; 
        }
      }
    }

    // Check if it is discaradble or not 
    SystemContext::QueueIndex = InQ->getIndex();
#ifdef DANBI_ENABLE_EAGER_THREAD_DESTROY
    if (isDiscardable()) {
      SystemContext::DbgOriginalEvent = SchedulingEventKind::ThreadDiscarded;
      return -ECANCELED; 
    }
#endif

    // If a kernel is in active state, yield and try later. 
    yield(); 
  }

  assert(TotalPopElm >=0);
#ifdef DANBI_ENABLE_EAGER_THREAD_DESTROY
  CPUSchedulingHint::setSuccessfulQueueOps();
#endif
  return TotalPopElm; 
}

static inline
int reservePush(AbstractReserveCommitQueue* OutQ, int NumElm, 
                QueueWindow (&Win)[2], ReservationInfo& RevInfo, 
                int ServingTicket, int* IssuingTicket) {
  // Reserve to push elements
  int ret; 
  while ( likely((ret = OutQ->reservePushN(NumElm, Win, RevInfo, 
                                           ServingTicket, IssuingTicket)) < 0) ) {
    // When queue is full
    SystemContext::Event = SystemContext::DbgOriginalEvent = 
      likely(ret == -EBUSY) ? 
      SchedulingEventKind::NotMyTurn : SchedulingEventKind::OutputQueueIsFull;
    SystemContext::QueueIndex = OutQ->getIndex();

#ifdef DANBI_ENABLE_EAGER_THREAD_DESTROY
    // Check if it is discaradble or not 
    if (isDiscardable()) {
      SystemContext::DbgOriginalEvent = SchedulingEventKind::ThreadDiscarded;
      return -ECANCELED; 
    }
#endif 
    
    // Yield and try later
    yield(); 

    // If a queue is not consumable, silently give up pushing an item.  
    if ( !SystemContext::DDScheduler->isQueueConsumable(OutQ->getIndex()) )
      return 0; 
  }

#ifdef DANBI_ENABLE_EAGER_THREAD_DESTROY
  CPUSchedulingHint::setSuccessfulQueueOps();
#endif
  return 0;
}

int __reserve_pop(int InputQueueIndex, ReserveCommitQueueAccessor* Accessor, 
                  int NumElm, int NumRepeat, int ServingTicket, int* IssuingTicket) {
  AbstractReserveCommitQueue* InQ = resolveInputQueue(InputQueueIndex); 
  return reservePeekPop(InQ, InputQueueIndex, 
                        NumElm, NumElm, NumRepeat, Accessor->Win, Accessor->RevInfo, 
                        ServingTicket, IssuingTicket);
}

int __reserve_peek_pop(int InputQueueIndex, ReserveCommitQueueAccessor* Accessor, 
                       int PeekNumElm, int PopNumElm, int NumRepeat, 
                       int ServingTicket, int* IssuingTicket) {
  AbstractReserveCommitQueue* InQ = resolveInputQueue(InputQueueIndex); 
  return reservePeekPop(InQ, InputQueueIndex, 
                        PeekNumElm, PopNumElm, NumRepeat, Accessor->Win, Accessor->RevInfo, 
                        ServingTicket, IssuingTicket);
}

void __commit_pop(int InputQueueIndex, ReserveCommitQueueAccessor* Accessor) {
  AbstractReserveCommitQueue* InQ = resolveInputQueue(InputQueueIndex);
  float FillRatio; 

  int ret; 
  while ( likely((ret = InQ->commitPop(Accessor->RevInfo, &FillRatio)) < 0) ) {
    SystemContext::Event = SchedulingEventKind::NotMyTurn; 
    SystemContext::QueueIndex = InQ->getIndex();
    SystemContext::DbgOriginalEvent = SystemContext::Event; 
    yield(); 
  }

  if ( !isInputQueueDeactivated(InputQueueIndex) )
    CPUSchedulingHint::updateInputQueueHint(InQ->getIndex(), FillRatio); 
}

void __commit_peek_pop(int InputQueueIndex, ReserveCommitQueueAccessor* Accessor) {
  __commit_pop(InputQueueIndex, Accessor);
}

int __reserve_push(int OutputQueueIndex, ReserveCommitQueueAccessor* Accessor, 
                    int NumElm, int ServingTicket, int* IssuingTicket) {
  AbstractReserveCommitQueue* OutQ = resolveOutputQueue(OutputQueueIndex); 
  return reservePush(OutQ, NumElm, Accessor->Win, Accessor->RevInfo, 
                     ServingTicket, IssuingTicket);
}

void __commit_push(int OutputQueueIndex, ReserveCommitQueueAccessor* Accessor) {
  AbstractReserveCommitQueue* OutQ = resolveOutputQueue(OutputQueueIndex);
  float FillRatio; 

  int ret; 
  while ( likely((ret = OutQ->commitPush(Accessor->RevInfo, &FillRatio)) < 0) ) {
    SystemContext::Event = SchedulingEventKind::NotMyTurn; 
    SystemContext::QueueIndex = OutQ->getIndex();
    SystemContext::DbgOriginalEvent = SystemContext::Event; 
    yield(); 
  }

  CPUSchedulingHint::updateOutputQueueHint(OutQ->getIndex(), FillRatio); 
}

void __consume_push_ticket(int OutputQueueIndex, int Ticket) {
  AbstractReserveCommitQueue* OutQ = resolveOutputQueue(OutputQueueIndex); 
  OutQ->consumePushTicket(Ticket); 
}

void __consume_pop_ticket(int InputQueueIndex, int Ticket) {
  AbstractReserveCommitQueue* InQ = resolveInputQueue(InputQueueIndex); 
  InQ->consumePopTicket(Ticket); 
}

const void* __robx(int ROBIndex, int X) {
  ReadOnlyBuffer* ROB = resolveReadOnlyBuffer(ROBIndex);
  return ROB->getElementAt(X);
}

const void* __robxy(int ROBIndex, int X, int Y) {
  ReadOnlyBuffer* ROB = resolveReadOnlyBuffer(ROBIndex);
  return ROB->getElementAt(X, Y);
}

const void* __robxyz(int ROBIndex, int X, int Y, int Z) {
  ReadOnlyBuffer* ROB = resolveReadOnlyBuffer(ROBIndex);
  return ROB->getElementAt(X, Y, Z); 
}
