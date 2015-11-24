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

  DistributedDynamicScheduler.cpp -- implementation of distributed dynamic scheduler
 */
#include <queue>
#include <iterator>
#include <algorithm>
#include <cassert>
#include <stdio.h>
#include "Support/Machine.h"
#include "Support/Random.h"
#include "Support/ConnectivityMatrix.h"
#include "Core/DistributedDynamicScheduler.h"
#include "Core/QueueInfo.h"
#include "Core/Kernel.h"
#include "Core/AbstractPartitioner.h"
#include "Core/SystemContext.h"
#include "Core/SchedulingHint.h"
#include "Support/Debug.h"

using namespace danbi; 

#ifdef DANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL
namespace danbi {
__thread DistributedDynamicScheduler::VisitedKernels DistributedDynamicScheduler::VisitInfo;
} // End of danbi namespace
#endif 

void DistributedDynamicScheduler::LogKernelEvent(const char* Msg, int KernelIndex) {
#undef DBG_KERNEL_EVENT
#ifdef DBG_KERNEL_EVENT
  ::printf("%s:(K %d)\n", Msg, KernelIndex);
#endif
}

void DistributedDynamicScheduler::LogNumSpawnedThreads() {
#undef DBG_NUM_SPAWNED_THREADS
#ifdef DBG_NUM_SPAWNED_THREADS
  ::printf("NumSpawnedThreads ["); 
  for(int i = 0; i < NumKernel; ++i) {
    ::printf("%d/%d ", 
             KernelStatusTable[i].NumRunningThreads, 
             KernelStatusTable[i].NumSpawnedThreads); 
  }
  ::printf("]\n"); 
#endif
}

DistributedDynamicScheduler::~DistributedDynamicScheduler() {
  delete StartKernels; 
  delete QueueScheduleTable; 
  delete KernelStatusTable; 
#ifdef DANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL   
  delete[] VisitedKernelsPool; 
#endif
}

int DistributedDynamicScheduler::buildSchedulingInfo(
  std::vector<Kernel*>& KernelTable, 
  std::vector<QueueInfo*>& QueueInfoTable, 
  ConnectivityMatrix<QueueInfo*>& ConnMatrix,
  AbstractPartitioner& Partitioner) {
  // alloc kernel status table
  NumKernel = KernelTable.size(); 
  KernelStatusTable = new KernelStatus[NumKernel]; 
  if (!KernelStatusTable) 
    return -ENOMEM; 

  // initialize kernel information 
  for (int i = 0; i < NumKernel; ++i) {
    KernelStatusTable[i].Instance = KernelTable[i];
    KernelStatusTable[i].ExecKind = KernelTable[i]->getExecutionKind(); 
    KernelStatusTable[i].NumRunningThreads = 0;
    KernelStatusTable[i].NumSpawnedThreads = 0;
    KernelStatusTable[i].SequentialKernelRunning = 0;
    KernelStatusTable[i].Terminated = 0; 
    KernelStatusTable[i].Terminating = 0; 
    KernelStatusTable[i].NextOnTheSameDevice = -1; 
    KernelStatusTable[i].AssignedComputeDevice = Partitioner.getComputeDevice(i); 
  }

  // alloc queue schedule table
  NumQueue = QueueInfoTable.size(); 
  QueueScheduleTable = new QueueSchedule[NumQueue]; 
  if (!QueueScheduleTable)
    return -ENOMEM; 

  // fill queue schedule information
  static_assert( NumQueueSchedulingEvent == 2, 
                 "The number of queue event is larger than two.");
  for (int i = 0; i < NumQueue; ++i) {
    QueueInfo* QInfo = QueueInfoTable[i]; 
    QueueSchedule* Schedule = &QueueScheduleTable[i];
    int NextKernel; 

    // When output queue of a producer become full 
    getNextViaQueue(ConnMatrix, true, QInfo, NextKernel);
    Schedule->NextKernel[OutputQueueIsFull] = NextKernel;
    
    // When input queue of a consumer become empty
    getNextViaQueue(ConnMatrix, false, QInfo, NextKernel);
    Schedule->NextKernel[InputQueueIsEmpty] = NextKernel;
  }

  // Next schedulable kernel which is on the same device is 
  // treated as one of output queue gets full. 
  for (int i = 0; i < NumQueue; ++i) {
    QueueInfo* QInfo = QueueInfoTable[i]; 
    QueueSchedule* Schedule = &QueueScheduleTable[i];

    KernelStatusTable[QInfo->first].NextOnTheSameDevice = 
      Schedule->NextKernel[OutputQueueIsFull];
  }

  // build up start kernels
  NodeIterator i, e; 
  ConnMatrix.getStartNodes(i, e); 

  NumStartKernel = std::distance(i, e); 
  if (NumStartKernel <= 0)
    return -EINVAL; 

  StartKernels = new int[NumStartKernel]; 
  if (!StartKernels)
    return -ENOMEM; 

  for(int c = 0; i != e; ++i, ++c) 
    StartKernels[c] = *i; 

#ifdef DANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL   
  // Allocate Visited kernel pool 
  // TODO: We definitely need more sophicated memory allocator to manage visited kernel pool. 
  VisitedKernelPoolAllocCount = 0; 
  VisitedKernelsPool = new int[NumKernel * DANBI_MAX_NUM_CORES];
  if (!VisitedKernelsPool) 
    return -ENOMEM; 
#endif

  return 0; 
}

void DistributedDynamicScheduler::getNextViaQueue(
  ConnectivityMatrix<QueueInfo*>& ConnMatrix, 
  bool Forward, QueueInfo* QInfo, int& Next) {
  // If two kernels linked via the queue are assigned on the same device
  if ( KernelStatusTable[QInfo->first].AssignedComputeDevice == 
       KernelStatusTable[QInfo->second].AssignedComputeDevice ) {
    Next = Forward ? QInfo->second : QInfo->first; 
    return; 
  }

  // Otherwise, we should across devices 
  // that are assigned to the different devices. 
  int Start = Forward ? QInfo->first : QInfo->second;
  if ( !getNext(ConnMatrix, Forward, Start, Next) )
    Next = Start; 
}

bool DistributedDynamicScheduler::getNext(ConnectivityMatrix<QueueInfo*>& ConnMatrix, 
                                          bool Forward, int This, int& Next) {
  std::queue<int> BFS; 

  // Get the current compute device
  int ComputeDevice = KernelStatusTable[This].AssignedComputeDevice; 

  // Find one kernel whose assigned compute device is the same in BFS order
  do {
    ConnectivityIterator i, e; 
    if (Forward) {
      if (!ConnMatrix.forwardNoFeedbackNext(This, i, e))
        return false; 
    }
    else {
      if (!ConnMatrix.backwardNoFeedbackNext(This, i, e))
        return false; 
    }

    for(; i != e; ++i) {
      int Successor = i->second; 
      assert( (0 <= Successor) && (Successor < NumKernel) ); 
      // Check if a successor is assigned to the same compute device
      if (KernelStatusTable[Successor].AssignedComputeDevice == ComputeDevice) {
        Next = Successor; 
        return true; 
      }
      // Otherwise, push the successor to the queue for searching further
     BFS.push(Successor); 
    }

    This = BFS.front(); 
    BFS.pop(); 
  } while( !BFS.empty() ); 

  return false; 
}

inline bool DistributedDynamicScheduler::trySchedule(int KernelIndex) {
  assert( (0 <= KernelIndex) && (KernelIndex < NumKernel) ); 

  // If a kernel is already terminated, never reschedule. 
  KernelStatus* KStatus = &KernelStatusTable[KernelIndex]; 
  if ( unlikely(KStatus->Terminated) )
    return false; 

  // In case of parallel kernel 
  if ( likely(KStatus->ExecKind == ParallelExec) )
    return true; 
  // In case of sequential kernel 
  else {
    assert(KStatus->ExecKind == SeqeuentialExec);
    // Check if it is not already running, acquire chance by using CAS operation. 
    if (KStatus->SequentialKernelRunning == 0)
      if (Machine::casInt<volatile int>(&KStatus->SequentialKernelRunning, 0, 1))
        return true;  
  }

  return false; 
}

inline bool DistributedDynamicScheduler::scheduleByQueueEvent(int KernelIndex, 
                                                              int QueueIndex, 
                                                              SchedulingEventKind Event, 
                                                              int& NextKernel) {
  // When waiting for ticket order or commit order, try another instance of the same kernel 
  if (Event == NotMyTurn) {
    NextKernel = KernelIndex; 
    bool rc = trySchedule(NextKernel); 
    if ( likely(rc) ) 
      CPUSchedulingHint::increaseNotMyTurnRetry();
    return rc; 
  }
  
  // When all thread of a kernel is terminated, give up to schedule
  if (Event == KernelTerminated) 
    return false; 

  // In case of real queue event 
  assert( (0 <= QueueIndex) && (QueueIndex < NumQueue) );
  QueueSchedule* Schedule = &QueueScheduleTable[QueueIndex]; 
  NextKernel = Schedule->NextKernel[Event]; 
    
  assert(KernelStatusTable[NextKernel].AssignedComputeDevice == 
         KernelStatusTable[KernelIndex].AssignedComputeDevice &&
         "Invalid compute device is scheduled.");
  assert(NextKernel >= 0 && "Invalid kernel is scheduled"); 
  return trySchedule(NextKernel); 
}

inline bool DistributedDynamicScheduler::scheduleByQueueConnection(int KernelIndex, 
                                                                   int& NextKernel) {
  // First, check if we will try scheduleByQueueConnection() or not
  if ( unlikely(!(Random::randomInt() % NoQueueConnScheduleProbMod)) )
    return false; 

  // Ok, let's try scheduleByQueueConnection()
  int Start = KernelIndex; 
  NextKernel = KernelIndex; 
  do {
    // Select a next kernel assigned to the same device via forward link. 
    NextKernel = KernelStatusTable[NextKernel].NextOnTheSameDevice;
    
    // If there is no kerel on the same compute device, give up. 
    if ( (NextKernel < 0) || (NextKernel == Start) )
      break; 

    // If we found a kernel on the same compute device, try schedule. 
    if (trySchedule(NextKernel))
      return true; 
  } while(true); 

  return false; 
}

inline bool DistributedDynamicScheduler::scheduleByExhausitiveSearch(int& NextKernel) {
  int Seed = Random::randomInt() % NumKernel; 

  // Exhaustive search from random start seed except for visited kernels
  for (int i = 0; i < NumKernel; ++i) {
    NextKernel = (Seed + i) % NumKernel; 

#ifdef DANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL   
    // Test if it is already visited or not
    if ( unlikely(testVisitedKernel(NextKernel)) )
      continue; 
#endif

    // Since it is running on CPU, schedule only kernels on CPU device. 
    KernelStatus* KStatus = &KernelStatusTable[NextKernel]; 
    if ( unlikely(KStatus->AssignedComputeDevice != AbstractPartitioner::CPU_DEVICE_INDEX) )
      continue; 

    // Try schedule 
    if (trySchedule(NextKernel))
      return true; 
  }

  // Finally, exhaustive search from random start seed 
  for (int i = 0; i < NumKernel; ++i) {
    NextKernel = (Seed + i) % NumKernel; 

    // Since it is running on CPU, schedule only kernels on CPU device. 
    KernelStatus* KStatus = &KernelStatusTable[NextKernel]; 
    if ( unlikely(KStatus->AssignedComputeDevice != AbstractPartitioner::CPU_DEVICE_INDEX) )
      continue; 

    // Try schedule 
    if (trySchedule(NextKernel))
      return true; 
  }
 return false; 
}

inline bool DistributedDynamicScheduler::scheduleFromStartKernels(int& NextKernel) {
  int Seed = Random::randomInt() % NumStartKernel; 

  // At first, try to allocate among start kernels
  for (int i = 0; i < NumStartKernel; ++i) {
    // Pick a start kernel index
    NextKernel = StartKernels[ (Seed + i) % NumStartKernel ];

    // Since it is running on CPU, schedule only kernels on CPU device. 
    KernelStatus* KStatus = &KernelStatusTable[NextKernel]; 
    if (KStatus->AssignedComputeDevice != AbstractPartitioner::CPU_DEVICE_INDEX)
      continue; 

    // Try schedule 
    if (trySchedule(NextKernel))
      return true; 
  }
  return false; 
}

int DistributedDynamicScheduler::schedule(int KernelIndex, 
                                          int QueueIndex, 
                                          SchedulingEventKind Event) {
  // Check precondition
  assert( (0 <= KernelIndex) && (KernelIndex < NumKernel) ); 
  int NextKernel; 

  // For random scheduling
  if ( unlikely(Event == RandomJump) )
    goto random_jump; 

  // At first, try to make scheduling decision based on connectivity matrix. 
  if ( scheduleByQueueEvent(KernelIndex, QueueIndex, Event, NextKernel) )
    goto success_end; 

  // In case of failing to get next scheduled kernel for queue event
  if ( scheduleByQueueConnection(KernelIndex, NextKernel) )
    goto success_end; 

  // Finally try exhausitive search, 
  // it does makes sense because NextOnTheSameDevice picks just on path of many. 
 random_jump:
  if ( scheduleByExhausitiveSearch(NextKernel) )
    goto success_end; 

  // Now, there is no schedulable kernel at this time. 
  // Negative kernel index means no schedulable kernel. 
 fail_end:
  NextKernel = -1; 
  return NextKernel; 

 success_end:
#ifdef DANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL   
  // Check new kernel is already visited
  if ( unlikely(testVisitedKernel(NextKernel)) ) {
    acknowledgeThreadScheduleOut(NextKernel);
    resetVisitedKernels(); 
    goto random_jump; 
  }
  addVisitedKernel(NextKernel);
#endif

  // Update thread local RunningKernel to the new scheduled kernel 
  SystemContext::RunningKernelIndex = NextKernel; 
  SystemContext::RunningKernel = KernelStatusTable[NextKernel].Instance; 
  return NextKernel; 
}

int DistributedDynamicScheduler::start() {
  int NextKernel; 

  // At first, try to allocate among start kernels
  if ( scheduleFromStartKernels(NextKernel) )
    goto success_end;

  // Finally, do exhaustive search  
  if ( scheduleByExhausitiveSearch(NextKernel) )
    goto success_end;

  // No start kernel. 
  // One possible reason is that all kernels are already terminated. 
  NextKernel = -1; 
  goto fail_end;

 success_end:
#ifdef DANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL   
  // Add this to the visited kernels
  addVisitedKernel(NextKernel); 
#endif

  // Update thread local RunningKernel to the new scheduled kernel 
  SystemContext::RunningKernelIndex = NextKernel; 
  SystemContext::RunningKernel = KernelStatusTable[NextKernel].Instance; 
 fail_end:
  return NextKernel;
}

void DistributedDynamicScheduler::initKernelExecution() {
  CPUSchedulingHint::initialize(); 
#ifdef DANBI_ENABLE_EAGER_THREAD_DESTROY
  SystemContext::DbgOriginalEvent = NumSchedulingEvent; 
#endif 
#ifdef DANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL   
  resetVisitedKernels();
#endif 
}

bool DistributedDynamicScheduler::keepContinue() {
  // Is a kernel terminated?
  if ( unlikely(SystemContext::RunningKernel->canTerminate(true)) ) {
    // Turn on terminating flag
    KernelStatusTable[SystemContext::RunningKernelIndex].Terminating = 1; 
    Machine::wmb(); // 'Terminating' should be updated before updating 'Terminated'.

    // Set KernelTerminated event
    SystemContext::Event = SchedulingEventKind::KernelTerminated;
    return false; 
  }

#ifdef DANBI_ENABLE_EAGER_THREAD_DESTROY
  // In cast that the kernel is discarded
  if ( unlikely(SystemContext::DbgOriginalEvent == SchedulingEventKind::ThreadDiscarded) )
    return false; 
#endif 

  // In case of parallel kernel 
  KernelStatus* KStatus = &KernelStatusTable[SystemContext::RunningKernelIndex]; 
  if ( likely(KStatus->ExecKind == ParallelExec) ) {
    // Do we need to jump somethere?
    if ( unlikely(CPUSchedulingHint::decideSpeculativeScheduling() ||
                  CPUSchedulingHint::decideRandomScheduling()) ) {
      CPUSchedulingHint* Hint = CPUSchedulingHint::getSchedulingHint(); 
      SystemContext::QueueIndex = Hint->NewQIndex;
      SystemContext::Event = Hint->NewEvent;
      SystemContext::DbgOriginalEvent = SchedulingEventKind::ProbabilisticHint;
      return false; 
    }
  }

  // Keep continue 
  return true; 
}

#ifdef DANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL
void DistributedDynamicScheduler::resetVisitedKernels() {
  VisitInfo.Count = 0; 
}

void DistributedDynamicScheduler::addVisitedKernel(int KernelIndex) {
  // Alloc. visited list if not
  if ( unlikely(!VisitInfo.Kernels) ) {
    int n = Machine::atomicIntInc<volatile int>(&VisitedKernelPoolAllocCount); 
    assert(n < DANBI_MAX_NUM_CORES && "Too many native threads");

    resetVisitedKernels(); 
    VisitInfo.Kernels = &VisitedKernelsPool[NumKernel * n];
  }

  // Add a new kernel index
  VisitInfo.Kernels[VisitInfo.Count] = KernelIndex; 
  ++VisitInfo.Count;
}

bool DistributedDynamicScheduler::testVisitedKernel(int KernelIndex) {
  // Test if a kernel is already visited
  for (int i = 0; i < VisitInfo.Count; ++i) {
    if ( unlikely(VisitInfo.Kernels[i] == KernelIndex) )
      return true; 
  }
  return false; 
}
#endif

