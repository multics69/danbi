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

  AbstractReserveCommitQueue.h -- abstract class for array based 
  circular FIFO queue with reserve and commit protocol 
 */

#ifndef DANBI_ABSTRACT_RESERVE_COMMIT_QUEUE_H
#define DANBI_ABSTRACT_RESERVE_COMMIT_QUEUE_H
#include <cstdlib>
#include "Support/Indexable.h"
#include "Support/ReserveCommitQueueAccessor.h"

namespace danbi {
class AbstractReserveCommitQueue : public Indexable {
protected:
  int ElmSize; 
  volatile int NumElm; 

public:
  /// Constructor 
  AbstractReserveCommitQueue(int ElmSize_, int NumElm_) 
    : ElmSize(ElmSize_), NumElm(NumElm_) {}

  /// Reserve multiple queue items to push
  virtual int reservePushN(int Num, QueueWindow (&Win)[2], 
                           ReservationInfo& RevInfo, 
                           int ServingTicket, int* IssuingTicket) = 0; 

  /// Commit push 
  virtual int commitPush(ReservationInfo& RevInfo, float* FillRatio = NULL) = 0;

  /// Reserve multiple queue items to pop
  virtual int reservePopN(int Num, QueueWindow (&Win)[2],
                          ReservationInfo& RevInfo, 
                          int ServingTicket, int* IssuingTicket) = 0; 

  /// Commit pop
  virtual int commitPop(ReservationInfo& RevInfo, float* FillRatio = NULL) = 0;

  /// Reserve multiple queue items to peek and pop
  virtual int reservePeekPopN(int PeekNum, int PopNum, 
                              QueueWindow (&Win)[2], ReservationInfo& RevInfo, 
                              int ServingTicket, int* IssuingTicket) = 0;

  /// Commit peek and pop
  virtual int commitPeekPop(ReservationInfo& RevInfo, float* FillRatio = NULL) = 0; 

  /// Consume the issued push ticket
  virtual void consumePushTicket(int Ticket) = 0; 

  /// Consume the issued pop ticket
  virtual void consumePopTicket(int Ticket) = 0; 

  /// Get element size
  inline int getElementSize() {
    return ElmSize; 
  }

  /// Get the maximum number of elements
  inline int getMaxNumElement() {
    return NumElm; 

  }

  /// Get the number of elements 
  virtual int getNumElement() = 0; 

  /// Drain remaining elements
  virtual void drain() = 0;

  /// Check if it is empty
  virtual bool empty() = 0; 

  /// Debug function 
  virtual void* __hookUpBuffer() = 0; 

  /// Set debug trace 
  virtual void __setDebugTrace(bool DebugTrace_) = 0; 
};

} // End of danbi namespace

#endif 
