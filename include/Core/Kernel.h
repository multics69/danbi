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

  Kernel.h -- compute kernel 
 */
#ifndef DANBI_KERNEL_H
#define DANBI_KERNEL_H
#include <cerrno>
#include <cstdlib>
#include <vector> 
#include "Support/BranchHint.h"
#include "Support/Machine.h"
#include "Support/Indexable.h"
#include "Core/SchedulingEvent.h"
#include "Core/AbstractCode.h"

namespace danbi {
class Program; 
class AbstractReserveCommitQueue; 
class ReadOnlyBuffer; 
class QueueInfo;

enum ExecutionKind {
  SeqeuentialExec, 
  ParallelExec, 
}; 

enum SubQueueAccessKind {
  Unknown, 

  SequentialAccess, 
  ParallelAccess, 
}; 

class Kernel : public Indexable {
  friend class AbstractUberKernel; 
private:
  Program& Pgm; // execution context

  AbstractCode* Code; // kernel code 
  std::vector<AbstractReserveCommitQueue*> InQueueTable;
  std::vector<AbstractReserveCommitQueue*> OutQueueTable;
  std::vector<ReadOnlyBuffer*> ReadOnlyBufferTable;

  ExecutionKind ExecKind; // parallel or sequential kernel 
  bool Start; // If it is a start kernel or not

  SubQueueAccessKind SubQueuePopKind; // sub queue pop kind
  SubQueueAccessKind SubQueuePushKind; // sub queue push kind
  int RegularInCount; // Non-zero for 1-to-1 regular parallel kernel 
  int RegularOutCount; // Non-zero for 1-to-1 regular parallel kernel 

  volatile bool Terminated; 
  volatile int ActiveNonFeedbackInputQueues; 
  
  void operator=(const Kernel&); // Do not implement
  Kernel(const Kernel&); // Do not implement

  /// getQueueInfo
  QueueInfo* getQueueInfo(AbstractReserveCommitQueue*); 
  
  /// count non-feedback input queues
  int countNonFeedbackInputQueue(); 

  /// Check if all non-feedback input queues are empty
  bool areAllNonFeedbackInputQueuesEmpty();

  /// Check if it is the right time to deactivate output queues
  inline bool canDeactivateNonFeedbackOutputQueues(bool EndOfKernel) {
    // In case of start kernel, we can check at the end of the kernel. 
    // NOTE: Key concept of terminating danbi application is 
    // propagating termiantion from the starting kernels. 
    if ( unlikely(Start) ) {
      if (EndOfKernel)
        return true; 
    }
    // For non-starting kernel, we can check in the middle of the kernel. 
    else {
      if (!EndOfKernel)
        return true; 
    }
    return false; 
  }
public:
  /// constructor 
  Kernel(Program& Pgm_, ExecutionKind ExecKind_, bool Start = false, 
         int RegularInCount_ = 0, int RegularOutCount_ = 0);

  /// Set kernel code
  int setCode(AbstractCode* Code_);

  /// Add input queue
  int addInputQueue(AbstractReserveCommitQueue* Queue_);

  /// Add output queue
  int addOutputQueue(AbstractReserveCommitQueue* Queue_);

  /// Add read-only buffer
  int addReadOnlyBuffer(ReadOnlyBuffer* ROB_);

  /// Verify if the kernel configuration is valid
  bool verify();

  /// Run the kernel 
  inline void run() {
    Code->run(); 
  }

  /// getter of execution kind 
  inline ExecutionKind getExecutionKind() {
    return ExecKind; 
  }

  /// Resolve input queue
  inline AbstractReserveCommitQueue* resolveInputQueue(int Index) {
    return InQueueTable[Index]; 
  }

  /// Resolve output queue 
  inline AbstractReserveCommitQueue* resolveOutputQueue(int Index) {
    return OutQueueTable[Index]; 
  }

  /// resolve read-only buffer
  inline ReadOnlyBuffer* resolveReadOnlyBuffer(int Index) {
    return ReadOnlyBufferTable[Index]; 
  }

  /// Check if a thread for this kernel can be safely terminated
  inline bool canTerminate(bool EndOfKernel) {
    // Start kernel cannot be terminated in the middle
    if (Start && !EndOfKernel)
      return false; 

    // If there are active inputs except for feedback input, 
    // we should consume them all. 
    if (ActiveNonFeedbackInputQueues > 0)
      return false; 
    Machine::cmb(); 

    // If terminination condition is already checked, 
    // and the decision is made to be terminated. 
    if (Terminated)
      return true; 

    // Check if it is the right time to deactivate output queues. 
    if (canDeactivateNonFeedbackOutputQueues(EndOfKernel)) {
      // If all non-feedback input queues are non-active, 
      // let's further check is all non-feedback input queues are empty. 
      if (areAllNonFeedbackInputQueuesEmpty()) {
        // Now, I am sure that I can terminate. 
        // However, deactivate non-feedback output queues 
        // only if I get the chance to turn on 'Terminated'.
        Terminated = true; 
        return true; 
      }
    }
    return false;  
  }

  /// Deactivate all output  queues
  void deactivateOutputQueues(); 

  /// Check if input queue is deactivated or not
  bool isInputQueueDeactivated(int Index); 

  /// Check if input queue is a feedback loop or not
  bool isInputQueueFeedbackLoop(int Index); 

  /// Is it a start kernel?
  inline bool isStart() {
    return Start; 
  }
};

} // End of danbi namespace
#endif 
