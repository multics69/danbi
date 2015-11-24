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

  DistributedDynamicScheduler.h -- distributed dynamic scheduler 
 */
#ifndef DANBI_DISTRIBUTED_DYNAMIC_SCHEDULER_H
#define DANBI_DISTRIBUTED_DYNAMIC_SCHEDULER_H
#include <cstdlib>
#include <cerrno>
#include "Support/Machine.h"
#include "Support/BranchHint.h"
#include "Core/AbstractScheduler.h"
#include "Core/Kernel.h"

namespace danbi {
template <typename ConnTy> class ConnectivityMatrix;

#undef DBG_RUNNING_THREAD
#define DANBI_MAX_NUM_CORES 128 // TODO: re-implement to remove this. 

class DistributedDynamicScheduler : public AbstractScheduler {
private:
  struct KernelStatus {
    Kernel* Instance; 
    ExecutionKind ExecKind; 
    int NextOnTheSameDevice; 
    int AssignedComputeDevice; 

    // Number of running thread for a kerenl. 
    // Only for debugging. 
    volatile int NumRunningThreads; 

    // Number of spawned thread for a kernel. 
    // When all spawned threads are terminated, we consider that a kernel is also terminated. 
    volatile int NumSpawnedThreads; 

    // To turn on SequentialKernelRunning, 
    // we should use CAS operation since there are contentions with other threads. 
    // On the contrary, turning off after safely acquiring it can be done 
    // with cheep normal memory operation, since there is no contention after
    // acquiring it. 
    volatile int SequentialKernelRunning; 
    
    //                     Kernel status
    //                     =============
    // [Active] -> [Terminating] -> [Terminated]
    // * Active
    //   - Schedulable 
    //   - New user-level thread creation is allowed. 
    // * Terminating
    //   - All non-feedback input queues are empty. 
    //   - Only threads that are pended because of output queue fullness
    //     are schedulabe. 
    //   - Threads that are pended because of output queue emptyness
    //     should be terminated. 
    //   - New user-level thread creation is not allowed. 
    // * Terminated
    //   - All non-feedback input queues are empty. 
    //   - Per-kernel ready queue are also empty. 
    volatile int Terminated; 
    volatile int Terminating; 
  }; 
  
  struct QueueSchedule {
    int NextKernel[NumQueueSchedulingEvent];
  };

  enum {
    NoQueueConnScheduleProbMod = 4, // 25%
  };
  
#ifdef DANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL   
  struct VisitedKernels {
    int Count; // Number of visited kernels
    int* Kernels; // List of visited kernels
  };

  static __thread VisitedKernels VisitInfo; 
  int* VisitedKernelsPool; 
  volatile int VisitedKernelPoolAllocCount; 
#endif

  KernelStatus* KernelStatusTable; 
  int NumKernel; 

  QueueSchedule* QueueScheduleTable; 
  int NumQueue; 

  int* StartKernels; 
  int NumStartKernel; 

  void operator=(const DistributedDynamicScheduler&); // Do not implement
  DistributedDynamicScheduler(const DistributedDynamicScheduler&); // Do not implement

  /// Build scheduling information from connectivity and core assignment
  virtual int buildSchedulingInfo(
    std::vector<Kernel*>& KernelTable, 
    std::vector<QueueInfo*>& QueueInfoTable, 
    ConnectivityMatrix<QueueInfo*>& ConnMatrix,
    AbstractPartitioner& Partitioner); 

  /// Get next kernel navigating via a queue 
  void getNextViaQueue(ConnectivityMatrix<QueueInfo*>& ConnMatrix, 
                       bool Forward, QueueInfo* QInfo, int& Next);

  /// Get next kernel whose assigned computing device is the same 
  bool getNext(ConnectivityMatrix<QueueInfo*>& ConnMatrix, 
               bool Forward, int This, int& Next); 

  /// Try to schedule the kerel 
  bool trySchedule(int KernelIndex);

  /// Schedule decision by queue event
  bool scheduleByQueueEvent(int KernelIndex, int QueueIndex, SchedulingEventKind Event, 
                            int& NextKernel);

  /// Schedule decision by queue connection 
  bool scheduleByQueueConnection(int KernelIndex, int& NextKernel); 

  /// Schedule by Exhausitive search
  bool scheduleByExhausitiveSearch(int& NextKernel); 

  /// Schedule from start kernels
  bool scheduleFromStartKernels(int& NextKernel); 

  /// Logging - TODO: Need to separate out a utility class
  void LogKernelEvent(const char* Msg, int KernelIndex);

  /// Acknowledge kernel termination 
  inline void acknowledgeKernelTermination(int KernelIndex) {
    // Check precondition
    assert( (0 <= KernelIndex) && (KernelIndex < NumKernel) ); 

    // Turn on terminated flag
    KernelStatusTable[KernelIndex].Terminated = 1; 
    Machine::wmb(); // 'Terminated' should be updated before decrementing queue count. 

    // Deactivate output queues after the kernel is surely terminated.
    KernelStatusTable[KernelIndex].Instance->deactivateOutputQueues(); 
    Machine::wmb();
    LogKernelEvent("Kernel-Terminated", KernelIndex); 
  }

#ifdef DANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL   
  /// Reset visited kernels list
  void resetVisitedKernels();

  /// Add a kernel to the visited list 
  void addVisitedKernel(int KernelIndex); 

  /// Test if a kernel is already visited or not
  bool testVisitedKernel(int KernelIndex); 
#endif

public:
  /// Constructor 
  DistributedDynamicScheduler(Runtime& Rtm_, Program& Pgm_)
    : AbstractScheduler(Rtm_, Pgm_), 
      KernelStatusTable(NULL), NumKernel(0),
      QueueScheduleTable(NULL), NumQueue(0), 
      StartKernels(NULL), NumStartKernel(0)
#ifdef DANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL   
    , VisitedKernelsPool(NULL), VisitedKernelPoolAllocCount(0) 
#endif 
    {
  }

  /// Destructor
  virtual ~DistributedDynamicScheduler(); 

  /// Decide next schedulable kernel
  virtual int schedule(int KernelIndex, int QueueIndex, SchedulingEventKind Event); 

  /// Decide start kernel 
  virtual int start(); 

  /// Initialize kernel execution 
  virtual void initKernelExecution();  

  /// Check if a kernel keeps running
  virtual bool keepContinue(); 

  /// Acknowledge thread spawn 
  inline void acknowledgeThreadSpawn(int KernelIndex) {
    // Check precondition
    assert( (0 <= KernelIndex) && (KernelIndex < NumKernel) ); 

    // increment thread count 
    KernelStatus* KStatus = &KernelStatusTable[KernelIndex];
    Machine::atomicIntInc<volatile int>(&KStatus->NumSpawnedThreads);
    LogKernelEvent("Thread-Spawn", KernelIndex); 
  }

  /// Acknowledge thread termination 
  inline void acknowledgeThreadTermination(int KernelIndex, bool KernelTermination) {
    // Check precondition
    assert( (0 <= KernelIndex) && (KernelIndex < NumKernel) ); 
    KernelStatus* KStatus = &KernelStatusTable[KernelIndex];

    // Thread termination for kernel termination 
    if ( unlikely(KernelTermination) ) {
      // Decrement thread count
      // If all threads for a kernel is terminated, then the kernel is terminated also. 
      if ( Machine::atomicIntDec<volatile int>(&KStatus->NumSpawnedThreads) == 0 ) 
        acknowledgeKernelTermination(KernelIndex); 
      assert( KStatus->NumSpawnedThreads >= 0 ); 

      // If it is a sequential kernel, turn on terminated also. 
      if (KStatus->ExecKind == SeqeuentialExec)
        KStatus->SequentialKernelRunning = 0;
    } 
    // Thread termination for scheduling 
    else {
      // Decrement thread count
      Machine::atomicIntDec<volatile int>(&KStatus->NumSpawnedThreads); 
      assert(KStatus->ExecKind == ParallelExec); 
    }
    LogKernelEvent("Thread-Terminating", KernelIndex); 
  }

  /// Acknowledge a thread is scheduled out
  inline void acknowledgeThreadScheduleOut(int KernelIndex) {
    // Check precondition
    assert( (0 <= KernelIndex) && (KernelIndex < NumKernel) ); 

    // Deschedule the currently running kernel. 
    KernelStatus* KStatus = &KernelStatusTable[KernelIndex];
    if (KStatus->ExecKind == SeqeuentialExec) 
      KStatus->SequentialKernelRunning = 0;
  }

  /// Increase the number of running thread
  inline void increaseRunningThreads(int KernelIndex) {
#ifdef DBG_RUNNING_THREAD
    // Check precondition
    assert( (0 <= KernelIndex) && (KernelIndex < NumKernel) ); 

    KernelStatus* KStatus = &KernelStatusTable[KernelIndex];
    Machine::atomicIntInc<volatile int>(&KStatus->NumRunningThreads);
#endif 
  }

  /// Decrease the number of running thread
  inline void decreaseRunningThreads(int KernelIndex) {
#ifdef DBG_RUNNING_THREAD
    // Check precondition
    assert( (0 <= KernelIndex) && (KernelIndex < NumKernel) ); 

    KernelStatus* KStatus = &KernelStatusTable[KernelIndex];
    Machine::atomicIntDec<volatile int>(&KStatus->NumRunningThreads);
#endif 
  }

  /// Acknowledge a thread can terminate
  inline void acknowledgeKernelCanTerminate(int KernelIndex) {
    // Check precondition
    assert( (0 <= KernelIndex) && (KernelIndex < NumKernel) ); 

    // Turn on terminating flag
    KernelStatusTable[KernelIndex].Terminating = 1; 
    LogKernelEvent("Kernel-Can-Terminate", KernelIndex); 
  }

  /// Check if a kernel is terminating
  inline bool isKernelTerminating(int KernelIndex) {
    // Check precondition
    assert( (0 <= KernelIndex) && (KernelIndex < NumKernel) ); 
    return KernelStatusTable[KernelIndex].Terminating; 
  }

  /// Check if a queue is consumable
  inline bool isQueueConsumable(int QueueIndex) {
    // Check precondition
    assert( (0 <= QueueIndex) && (QueueIndex < NumQueue) ); 

    // If a consumer of this queue is not terminated,
    // it is not consumable. 
    int Consumer = QueueScheduleTable[QueueIndex].NextKernel[OutputQueueIsFull]; 
    if (Consumer >= 0)
      return !KernelStatusTable[Consumer].Terminated; 
    return false; 
  }

  // Logging
  void LogNumSpawnedThreads(); 
};

} // End of danbi namespace 

#endif 
