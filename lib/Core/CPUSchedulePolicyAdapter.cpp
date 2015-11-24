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

  CPUSchedulePolicyAdapter.cpp
  -- CPU schedule policy adapter between GreenScheduler and AbstractScheduler
 */
#include <cassert>
#include <cerrno>
#include <stdio.h>
#include "Core/CPUSchedulePolicyAdapter.h"
#include "Core/AbstractScheduler.h"
#include "Core/CPUUberKernel.h"
#include "Core/SystemContext.h"
#include "Core/SchedulingHint.h"
#include "DebugInfo/EventLogger.h"
#include "Support/Thread.h"
#include "Support/Machine.h"
#include "Support/Debug.h"

using namespace danbi; 

CPUSchedulePolicyAdapter::CPUSchedulePolicyAdapter() 
  : NumAllKernel(0), Scheduler(NULL), UberKernel(NULL), ReadyQueues(NULL) {}

CPUSchedulePolicyAdapter::~CPUSchedulePolicyAdapter() {
  destruct(); 
}

void CPUSchedulePolicyAdapter::LogScheduleEvent(
  const char* Msg, int OldKernel, 
  GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* NewThread) {
#undef DBG_SCHEDULE_EVENT
#undef DBG_FILTER_OUT_NOT_MY_TURN
#ifdef DBG_SCHEDULE_EVENT
#ifdef DBG_FILTER_OUT_NOT_MY_TURN
  if (SystemContext::DbgOriginalEvent != SchedulingEventKind::NotMyTurn) 
#endif
  {
    static volatile int Count; 
    int OldCount = Machine::atomicIntInc<volatile int>(&Count); 
    if ( (OldCount%1) == 0 ) {
      CPUSchedulingHint* Hint = CPUSchedulingHint::getSchedulingHint(); 
      if (unlikely(Hint == NULL)) 
        return; 
      ::printf("[0x%x] [%3d] %s:(K %d, Q %d, E %d OE %d RETRY %d IFm %.2f OFM %.2f Ti %.2f To %.2f) -> K: %d T 0x%lx\n", 
               Thread::getID(), 
               Count, 
               Msg, 
               OldKernel, 
               SystemContext::QueueIndex,
               SystemContext::Event, 
               SystemContext::DbgOriginalEvent, 
               Hint->NotMyTurnRetry, 
               SystemContext::DbgOriginalEvent == ProbabilisticHint ? 
               Hint->InFillMin : 0.0f, 
               SystemContext::DbgOriginalEvent == ProbabilisticHint ? 
               Hint->OutFillMax : 0.0f, 
               SystemContext::DbgOriginalEvent == ProbabilisticHint ? 
               Hint->TransInProb : 0.0f, 
               SystemContext::DbgOriginalEvent == ProbabilisticHint ? 
               Hint->TransOutProb : 0.0f, 
               SystemContext::RunningKernelIndex, 
               (unsigned long)NewThread);
      Scheduler->LogNumSpawnedThreads(); 
    }
  }
#endif 
}

CPUSchedulePolicyAdapter& CPUSchedulePolicyAdapter::getInstance() {
  static CPUSchedulePolicyAdapter Instance; 
  return Instance; 
}

void CPUSchedulePolicyAdapter::destruct() {
  GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* Thread_; 

  // All threads should be terminated before calling destructor. 
  for (int i = 0; i < NumAllKernel; ++i) {
    assert( ReadyQueues[i].pop(Thread_) == -EAGAIN ); 
    ; // Do nothing except for assert()
  }

  // Since all terminated threads are in recycle bin, 
  // we can delete all the created threads easily. 
  while( !RecycleBin.pop(Thread_) ) 
    delete Thread_; 

  // Delete ready queues
  delete ReadyQueues; 

  // Clear values
  NumAllKernel = 0; 
  Scheduler = NULL; 
  UberKernel = NULL; 
  ReadyQueues = NULL; 
}

int CPUSchedulePolicyAdapter::initialize(int NumAllKernel_, 
                                         AbstractScheduler& Scheduler_, 
                                         CPUUberKernel& UberKernel_) {
  assert(NumAllKernel_ > 0 && "There shoud be one or more kernels."); 

  // If it is reused, reconstruct this object. 
  if (Scheduler != NULL)
    destruct(); 

  // Now, back to normal construction process
  NumAllKernel = NumAllKernel_;
  Scheduler = reinterpret_cast<DistributedDynamicScheduler*>(&Scheduler_); 
  UberKernel = &UberKernel_; 

  ReadyQueues = new ConcurrentQueue[NumAllKernel];
  if (!ReadyQueues) 
    return -ENOMEM; 

  return 0; 
}
  
void CPUSchedulePolicyAdapter::registerThread(
  GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >& Thread_) {
  assert(Scheduler != NULL && "Scheduler should be already registered."); 
  // Acknowledge thread spawn
  Scheduler->acknowledgeThreadSpawn(SystemContext::RunningKernelIndex); 
}

void CPUSchedulePolicyAdapter::deregisterThread(
  GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >& Thread_) {
  assert(Scheduler != NULL && "Scheduler should be already registered."); 
  // Acknowledge thread termination
  Scheduler->acknowledgeThreadTermination(
    SystemContext::RunningKernelIndex, 
    SystemContext::Event == SchedulingEventKind::KernelTerminated); 
  Scheduler->decreaseRunningThreads(SystemContext::RunningKernelIndex); 
  
  // Push to recycle bin
  RecycleBin.push(&Thread_); 

  // Logging
  EventLogger::appendThreadLifeEvent(SystemContext::RunningKernelIndex, false); 
  EventLogger::appendQueueEvent(SystemContext::RunningKernelIndex, 
                                SystemContext::QueueIndex, 
                                SystemContext::Event, 
                                SystemContext::DbgOriginalEvent); 
}

void CPUSchedulePolicyAdapter::scheduleOutThread(
  GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* Thread_) {
  // Push the old kernel to a per-kernel ready queue
  ReadyQueues[SystemContext::OldKernelIndex].push(Thread_);

  // Acknowledge thread schedule out
  Scheduler->acknowledgeThreadScheduleOut(SystemContext::OldKernelIndex); 
  Scheduler->decreaseRunningThreads(SystemContext::OldKernelIndex); 

  // Reset NoMyTurnRetry count
  CPUSchedulingHint::resetNotMyTurnRetry();
}

void CPUSchedulePolicyAdapter::checkTerminationCondition() {
  // Check termination condition 
  if (SystemContext::RunningKernel->canTerminate(false)) {
    Scheduler->acknowledgeKernelCanTerminate(SystemContext::RunningKernelIndex);
    SystemContext::Terminating = true; 
  }
}

GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* 
CPUSchedulePolicyAdapter::selectThreadForKernel() {
  GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* NewThread = NULL; 

  // Try to get a pending thread for the new kernel in a per-kernel ready queue
  if (ReadyQueues[SystemContext::RunningKernelIndex].pop(NewThread)) {
    // - If a kernel is active and an event is a queue event, 
    //   create a new user level thread. 
    // - In case of waiting for ticket order or commit order , NotMyTurn, 
    //   reschedule other thread instance of the same kernel. 
    // - If a kernel is terminating and all pending threads are already processed, 
    //   now it is time to be in terminated status. 
     if ( (SystemContext::Event != NotMyTurn) && 
         !Scheduler->isKernelTerminating(SystemContext::RunningKernelIndex) ) {
      NewThread = newGreenThread(); 
      LogScheduleEvent("Reschedule-Spawn", 
                       SystemContext::OldKernelIndex, NewThread);
    }
  }
  // If there is a kernel on a per-kernel ready queue
  else {
    assert(NewThread != NULL); 
    LogScheduleEvent("Reschedule", 
                     SystemContext::OldKernelIndex, NewThread);
  }

  return NewThread; 
}

inline
bool CPUSchedulePolicyAdapter::keepGoingWithThisThread(SchedulingEventKind Event, 
                                                       int KernelIndex) {
  // For NotMyTurn evnet
  if (Event == SchedulingEventKind::NotMyTurn) {
#ifdef DANBI_ENABLE_RANDOM_JUMP_IN_THE_MIDDLE
    // Test random scheduling probability
    if ( unlikely(CPUSchedulingHint::decideRandomScheduling()) ) {
      CPUSchedulingHint* Hint = CPUSchedulingHint::getSchedulingHint(); 
      SystemContext::Event = Hint->NewEvent;
      return false;
    }
#endif 

    // If there is no idle thread instance for this kernel, 
    // keep going with this instance.
    if ( ReadyQueues[KernelIndex].empty() ) {
      // Increase retry count
      CPUSchedulingHint::increaseNotMyTurnRetry();
      return true; 
    }
  }
  return false; 
}


bool CPUSchedulePolicyAdapter::selectKernel(
  GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* Thread_, 
  int RetryCount) {
  // When it is the first time to be scheduled, try to get a starting kernel. 
  if ( unlikely(SystemContext::Event == SchedulingEventKind::NumSchedulingEvent &&
                Thread_ == NULL && 
                RetryCount == 0) ) {
    if ( Scheduler->start() < 0 )
      return false; 
  }
  // When it is already started, make schedule decision based on event. 
  else {
    // If it is for ticketing with no thread instance in the ready queue, 
    // keep going with this thread instance. 
    if ( likely(
           keepGoingWithThisThread(SystemContext::Event, SystemContext::RunningKernelIndex)) )
      return false; 

    // Otherwise select new kernel instance. 
    if ( Scheduler->schedule(SystemContext::RunningKernelIndex, 
                             SystemContext::QueueIndex, 
                             (RetryCount == 0) ? 
                             SystemContext::Event : SchedulingEventKind::RandomJump) < 0 )
      return false; 
  }
  return true; 
}

GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* 
CPUSchedulePolicyAdapter::selectNext(
  GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* Thread_) {
  Kernel* OldKernel = SystemContext::RunningKernel; 
  SystemContext::OldKernelIndex = SystemContext::RunningKernelIndex;
  GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* NewThread;
  int RetryCount = 0; 

  // Check if there is replacing thread
  if ( unlikely(SystemContext::ReplacingThread) ) {
    NewThread = SystemContext::ReplacingThread;
    SystemContext::ReplacingThread = NULL; 
    LogScheduleEvent("Reschedule-Replacing", SystemContext::RunningKernelIndex, NULL);
    goto success_end;
  }

 retry:
  // Select a kernel 
  if ( !selectKernel(Thread_, RetryCount) ) {
    // If it fails to select a kernel, give up. 
    return NULL; 
  }

  // Select a thread for a scheduled kernel
  NewThread = selectThreadForKernel();
  if ( unlikely(NewThread == NULL) ) {
    // If the same kernel is selected, keep continue. 
    if ( unlikely((SystemContext::Event != NotMyTurn) && 
                  (SystemContext::RunningKernelIndex == SystemContext::OldKernelIndex)) )
      return NULL; 

    // Acknowledge thread schedule out
    Scheduler->acknowledgeThreadScheduleOut(SystemContext::RunningKernelIndex); 

    // If it fails to select a thread, recover the status and retry. 
    SystemContext::RunningKernel = OldKernel;
    SystemContext::RunningKernelIndex = SystemContext::OldKernelIndex; 
    ++RetryCount; 
    goto retry;
  }
  assert( NewThread != NULL && NewThread->isStackHealthy() ); 

  // Get if a new kernel is terminating or not
  SystemContext::Terminating = Scheduler->isKernelTerminating(
    SystemContext::RunningKernelIndex);

 success_end:
  CPUSchedulingHint::setReadyQueueMayNotEmpty();
  Scheduler->increaseRunningThreads(SystemContext::RunningKernelIndex);

  // Logging
  EventLogger::appendQueueEvent(SystemContext::RunningKernelIndex, 
                                SystemContext::QueueIndex, 
                                SystemContext::Event, 
                                SystemContext::DbgOriginalEvent); 
  return NewThread; 
}

GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* 
CPUSchedulePolicyAdapter::newGreenThread() {
  GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* New; 
  // At first, try to recycle. If there is no recyclable, allocate one. 
  if ( RecycleBin.pop(New) )
    New = new GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >(*UberKernel); 
  if (!New) return NULL; 

  // Initialize stack and register it to the scheduler
  if ( New->initialize() )
    return NULL; 

  // Logging
  EventLogger::appendThreadLifeEvent(SystemContext::RunningKernelIndex, true); 
  return New; 
}

bool CPUSchedulePolicyAdapter::keepContinue() {
  // If there are one or more thread instances in the ready queue, 
  // terminate this instance and run the one in the ready queue
  // to fit the number of native thread and green thread 
  // for minimizing ticketing overhead. 
  if ( unlikely(CPUSchedulingHint::getReadyQueueMayNotEmpty()) ) {
    if ( !ReadyQueues[SystemContext::RunningKernelIndex].pop(
           SystemContext::ReplacingThread) ) 
      return false; 
  }
 
  // Now, check if it keeps continue or not at the point of scheduler. 
  return Scheduler->keepContinue();
}

void CPUSchedulePolicyAdapter::kickoff() {
  UberKernel->kickoff();
}
