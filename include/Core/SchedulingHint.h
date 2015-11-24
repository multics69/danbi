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

  SchedulingHint.h -- Scheduling Hint
 */
#ifndef DANBI_SCHEDULING_HINT_H
#define DANBI_SCHEDULING_HINT_H
#include <cassert>
#include "Support/BranchHint.h"
#include "Support/Random.h"
#include "Support/GreenScheduler.h"
#include "Core/CPUSchedulePolicyAdapter.h"
#include "Core/SchedulingEvent.h"

namespace danbi {
#define SCHEDULING_HINT_INIT_MIN  9.0f
#define SCHEDULING_HINT_INIT_MAX -9.0f

template <typename GreenSchedulerTy>
struct SchedulingHint {
  bool ReadyQueueMayNotEmpty; 
  int NotMyTurnRetry; 
  float InFillMin, InFillMax, OutFillMin, OutFillMax; 
  int InQIndex, OutQIndex;
  float TransInProb, TransOutProb; 
  int NewQIndex; 
  SchedulingEventKind NewEvent;
#ifdef DANBI_ENABLE_EAGER_THREAD_DESTROY
  bool HaveSuccessfulQueueOps; 
#endif 

  static inline SchedulingHint* getSchedulingHint() {
    if ( unlikely(GreenSchedulerTy::RunningThread == NULL) ) 
      return NULL; 
    return reinterpret_cast<SchedulingHint*>(GreenSchedulerTy::RunningThread->TLS); 
  }

  static inline void initialize() {
    SchedulingHint* Hint = getSchedulingHint(); 
    Hint->ReadyQueueMayNotEmpty = false; 
    Hint->NotMyTurnRetry = 0;
    Hint->InFillMin = Hint->OutFillMin = SCHEDULING_HINT_INIT_MIN; 
    Hint->InFillMax = Hint->OutFillMax = SCHEDULING_HINT_INIT_MAX;
#ifdef DANBI_ENABLE_EAGER_THREAD_DESTROY
    Hint->HaveSuccessfulQueueOps = false; 
#endif 
  }

  static inline void setReadyQueueMayNotEmpty() {
    SchedulingHint* Hint = getSchedulingHint(); 
    if ( likely(Hint) )
      Hint->ReadyQueueMayNotEmpty = true; 
  }

  static inline bool getReadyQueueMayNotEmpty() {
    return getSchedulingHint()->ReadyQueueMayNotEmpty; 
  }

  static inline void increaseNotMyTurnRetry() {
    SchedulingHint* Hint = getSchedulingHint(); 
    ++Hint->NotMyTurnRetry; 
  }

  static inline void resetNotMyTurnRetry() {
    SchedulingHint* Hint = getSchedulingHint(); 
    Hint->NotMyTurnRetry = 0;
  }

#ifdef DANBI_ENABLE_EAGER_THREAD_DESTROY
  static inline void setSuccessfulQueueOps() {
    getSchedulingHint()->HaveSuccessfulQueueOps = true; 
  }

  static inline bool haveSuccessfulQueueOps() {
    return getSchedulingHint()->HaveSuccessfulQueueOps;
  }
#endif

  static inline void updateInputQueueHint(int QIndex, float FillRatio) {
    assert( 0.0f <= FillRatio && FillRatio <= 1.0f ); 
    SchedulingHint* Hint = getSchedulingHint(); 

    if ( likely(Hint->InFillMin > FillRatio) ) {
      Hint->InFillMin = FillRatio; 
      Hint->InQIndex = QIndex; 
    }
    if ( likely(Hint->InFillMax < FillRatio) )
      Hint->InFillMax = FillRatio; 
  }

  static inline void updateOutputQueueHint(int QIndex, float FillRatio) {
    assert( 0.0f <= FillRatio && FillRatio <= 1.0f ); 
    SchedulingHint* Hint = getSchedulingHint(); 

    if ( likely(Hint->OutFillMin > FillRatio) )
      Hint->OutFillMin = FillRatio; 
    if ( likely(Hint->OutFillMax < FillRatio) ) {
      Hint->OutFillMax = FillRatio; 
      Hint->OutQIndex = QIndex; 
    }
  }

  static inline bool decideSpeculativeScheduling() {
#ifndef DANBI_DISABLE_SPECULATIVE_SCHEDULING
    SchedulingHint* Hint = getSchedulingHint(); 
    float Prob = Random::randomFloat(); 

    // Calculate transation probability
    // - A kernel has no input queue.
    if ( unlikely(Hint->InFillMin > 1.0f) ) {
      Hint->TransOutProb = (Hint->OutFillMax + Hint->OutFillMin - 1.0f) / 2.0f; 
      Hint->TransInProb = 0.0f; 
    }
    // - A kernel has no output queue.
    else if ( unlikely(Hint->OutFillMin > 1.0f) ) {
      Hint->TransOutProb = 0.0f; 
      Hint->TransInProb = (1.0f - Hint->InFillMin - Hint->InFillMax) / 2.0f; 
    }
    // - A kernel has both input and output queue. 
    else {
      Hint->TransOutProb = 
        (2.0f*Hint->OutFillMax + Hint->OutFillMin - Hint->InFillMax - 1.0f) / (2.0f * 2.0f); 
      Hint->TransInProb = 
        (1.0f - 2.0f*Hint->InFillMin - Hint->InFillMax + Hint->OutFillMin) / (2.0f * 2.0f); 
    }

    // Test transition probability
    // - forward transition
    if ( unlikely(Prob <= Hint->TransOutProb) ) {
      Hint->NewQIndex = Hint->OutQIndex; 
      Hint->NewEvent = SchedulingEventKind::OutputQueueIsFull; 
      return true; 
    }
    // - backward transition
    if ( unlikely(Prob <= (Hint->TransOutProb + Hint->TransInProb)) ) {
      Hint->NewQIndex = Hint->InQIndex; 
      Hint->NewEvent = SchedulingEventKind::InputQueueIsEmpty; 
      return true; 
    }
#endif

    // Keep staying here
    return false; 
  }

  static inline bool decideRandomScheduling() {
#ifndef DANBI_DISABLE_RANDOM_SCHEDULING
    SchedulingHint* Hint = getSchedulingHint(); 
    float Prob = Random::randomFloat(); 

    // Calculate random jump probability
    float RandomJumpProb = std::min((float)Hint->NotMyTurnRetry / 10000.0f, 1.0f); 
    if ( unlikely(Prob <= RandomJumpProb) ) {
      Hint->NewEvent = SchedulingEventKind::RandomJump; 
      return true; 
    }
#endif
    // Keep staying here
    return false; 
  }
}; 

typedef SchedulingHint< GreenScheduler<CPUSchedulePolicyAdapter> > CPUSchedulingHint; 

} // End of danbi namespace 

#endif 

