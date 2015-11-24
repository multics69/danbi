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

  ConcurrentReserveCommitQueue.h -- implementation class of concurrent reserve/commit based 
  circular FIFO queue. 
 */
#ifndef DANBI_CONCURRENT_RESERVE_COMMIT_QUEUE_H
#define DANBI_CONCURRENT_RESERVE_COMMIT_QUEUE_H
#include <cassert>
#include <cstdlib>
#include "Support/Machine.h"
#include "Support/BranchHint.h"
#include "Support/AbstractReserveCommitQueue.h"

namespace danbi {
#define DANBI_RCQ_ENABLE_HOOKUP_BUFFER
#undef DANBI_RCQ_ENABLE_DEBUG_TRACE // SHOULD be turned off at production mode

template <typename PushTicketIssuerTy, typename PushTicketServerTy, 
          typename PopTicketIssuerTy, typename PopTicketServerTy,
          typename BufferManagerTy>
class ReserveCommitQueue : public AbstractReserveCommitQueue {
private:
  PushTicketIssuerTy PushTicketIssuer;
  PushTicketServerTy PushTicketServer;
  PopTicketIssuerTy  PopTicketIssuer; 
  PopTicketServerTy  PopTicketServer;  
  BufferManagerTy BuffMgr; 
  void* Buff; 
#ifdef DANBI_RCQ_ENABLE_DEBUG_TRACE
  bool DebugTrace; 
  volatile int NumFilledElements;
  volatile int NumRvPush; 
  volatile int NumCmPush; 
  volatile int NumRvPop; 
  volatile int NumCmPop; 
#endif

  volatile int Head __cacheline_aligned; 
  volatile int ReservedTail; 
#ifndef DANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING
  volatile ReservationInfo* LastReservedPop; 
#endif

  volatile int Tail __cacheline_aligned; 
  volatile int ReservedHead; 
#ifndef DANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING
  volatile ReservationInfo* LastReservedPush; 
#endif
  
  void operator=(const ReserveCommitQueue&); // Do not implement
  ReserveCommitQueue(const ReserveCommitQueue&); // Do not implement

  inline void* getElementAt(int Index) {
    return static_cast<char *>(Buff) + ElmSize * Index; 
  }

  inline int getNumFilledElements() {
    // Calculate how full it is
    // { Head, ReservedHead, Tail, ReservedTail }
    //        |<---------------->|
    int tail = Tail; 
    int r_head = ReservedHead; 
    int numelm = NumElm;
    
    if (r_head <= tail)
      return tail - r_head; 
    return numelm - (r_head - tail); 
  }

  inline float getFillRatio() {
     int numelm = NumElm;
     return (float)getNumFilledElements() / (float)(numelm - 1);
  }

  // Check if the requested push makes the queue full
  inline bool haveEnoughRoom(int numelm, int r_tail, int head, int Num) {
    // - case 1: [..., {{head, ..., r_tail}}, ...]
    if (r_tail >= head) {
      // Wrapping around is the only way for a queue to become full. 
      if ( ((r_tail+Num) >= numelm) && (head <= (r_tail+Num-numelm)) )
        return false; 
    }
    // - case 2: [..., r_tail}}, ..., {{head, ...]
    else if (r_tail < head) {
      if (head <= (r_tail+Num))
        return false; 
    }
    return true; 
  }

  /// Check if the requested pop makes the queue empty
  inline bool haveEnoughData(int numelm, int r_head, int tail, int PeekNum) {
    // - case 1: Queue is alrady empty. 
    if (r_head == tail)
      return false;
    // - case 2: [..., {{r_head, ..., tail}}, ...]
    else if (r_head < tail) {
      if ((r_head+PeekNum) > tail) 
        return false;
    }
    // - case 3: [..., tail}}, ..., {{r_head, ...]
    else {
      // Wrapping around is the only way to become empty
      if ( ((r_head+PeekNum) >= numelm) && (tail < (r_head+PeekNum-numelm)) )
        return false;
    }
    return true; 
  }
public:
  /// Constructor
  ReserveCommitQueue(int ElmSize_, int NumElm_) 
    : AbstractReserveCommitQueue(ElmSize_, NumElm_+1), 
      BuffMgr(ElmSize, NumElm), 
      Buff(BuffMgr.getInitialBuffer()), 
      Head(0), ReservedHead(0), 
#ifndef DANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING
      LastReservedPush(NULL), 
#endif 
      Tail(0), ReservedTail(0)
#ifndef DANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING
    , LastReservedPop(NULL) 
#endif
    {
#ifdef DANBI_RCQ_ENABLE_DEBUG_TRACE
    NumFilledElements = NumRvPush = NumCmPush = NumRvPop = NumCmPop = 0; 
#endif
  }

  /// Destructor
  virtual ~ReserveCommitQueue() {
    // do nothing
  }

  /// Reserve multiple queue items to push
  ///  o return 0: success
  ///  o reutrn -EBUSY: waiting for ticket
  ///  o return -EWOULDBLOCK: queue is full 
  virtual int reservePushN(int Num, QueueWindow (&Win)[2], 
                           ReservationInfo& RevInfo, 
                           int ServingTicket, int* IssuingTicket) {
    int numelm, head, r_tail, new_r_tail; 

  retry:
    // Get shared variables
    Machine::rmb();
    numelm = NumElm;
    r_tail = ReservedTail; 
    head = Head; 

    // Check if the requested reservation makes the queue full
    if (!haveEnoughRoom(numelm, r_tail, head, Num)) 
      return -EWOULDBLOCK; 

    // Lazy waiting of my turn for eager -EWOULDBLOCK return 
    if ( !PushTicketServer.isMyTurn(ServingTicket) ) 
      return -EBUSY; 
     
    // Set reserved queue windows
    Win[0].Base = getElementAt(r_tail);
    new_r_tail = r_tail + Num; 
    // - case 1: wrapped around
    if ( unlikely(new_r_tail >= numelm) ) {
      new_r_tail -= numelm; 
      Win[0].Num = numelm - r_tail; 
      Win[1].Base = getElementAt(0);
      Win[1].Num = new_r_tail; 
    }
    // - case 2: not wrapped around
    else {
      Win[0].Num = Num; 
      Win[1].Base = NULL; 
      Win[1].Num = 0; 
    }

    // Set reservation information
#ifdef DANBI_RCQ_ENABLE_DEBUG_TRACE
    if (DebugTrace)
      RevInfo.__Num = Num; 
#endif 
    RevInfo.OldIndex = r_tail; 
    RevInfo.NewIndex = new_r_tail; 
#ifndef DANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING
    RevInfo.ReadyFlag = 0; 
    RevInfo.Next = NULL; 
    Machine::wmb(); 
#endif 

    // Update ReservedTail
    if (!Machine::casInt<volatile int>(&ReservedTail, r_tail, new_r_tail))
      goto retry; 

    // Everything is alright. Update LastReservedPush
#ifndef DANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING
    ReservationInfo* old_last = const_cast<ReservationInfo*>( 
      Machine::fasWord<volatile ReservationInfo*>(&LastReservedPush, &RevInfo) ); 
    if (old_last != NULL) 
      old_last->Next = &RevInfo; 
    else 
      RevInfo.ReadyFlag = 1; 
    Machine::wmb(); 
#endif

#ifdef DANBI_RCQ_ENABLE_DEBUG_TRACE
    if (DebugTrace)
      Machine::atomicIntAddReturn<volatile int>(&NumRvPush, RevInfo.__Num); 
#endif 
    
    // Consume a push ticket only for the successful push 
    PushTicketServer.consume_safe(ServingTicket); 

    // Issue a push ticket only for the successful pop
    PushTicketIssuer.issue(IssuingTicket, r_tail, new_r_tail); 
    return 0; 
  }

  /// Commit push 
  virtual int commitPush(ReservationInfo& RevInfo, float* FillRatio = NULL) {
#ifndef DANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING
    // Check if it is my commit order 
    if ( !RevInfo.ReadyFlag ) {
      Machine::rmb(); 
      if ( !RevInfo.ReadyFlag )
        return -EBUSY; 
    }

    // If it's my turn, update tail. 
    if ( !Machine::casInt<volatile int>(&Tail, RevInfo.OldIndex, RevInfo.NewIndex) ) {
      // Otherwise, wakeup next
      Machine::rmb(); 
      if (RevInfo.Next) {
        RevInfo.Next->ReadyFlag = 1; 
        Machine::wmb(); 
      }
      return -EBUSY;
    }

    // Update LastReservedPush or wake up the next waiting committer
    if ( !Machine::casWord<volatile ReservationInfo*>(&LastReservedPush, &RevInfo, NULL) ) {
      while (!RevInfo.Next) Machine::rmb(); 
      RevInfo.Next->ReadyFlag = 1; 
      Machine::wmb(); 
    }
#else
    while ( !Machine::casInt<volatile int>(&Tail, RevInfo.OldIndex, RevInfo.NewIndex) ) ; 
#endif 

    // Calculate how much it is filled
    if ( likely(FillRatio) ) {
      *FillRatio = getFillRatio();
      assert( 0.0f <= *FillRatio && *FillRatio <= 1.0f ); 
    }

#ifdef DANBI_RCQ_ENABLE_DEBUG_TRACE
    if (DebugTrace) {
      int elm = Machine::atomicIntAddReturn<volatile int>(
        &NumFilledElements, RevInfo.__Num); 
      int melm = getNumFilledElements();
      Machine::atomicIntAddReturn<volatile int>(&NumCmPush, RevInfo.__Num); 
      if (elm == melm) {
        ::fprintf(stdout, "=RCQ_TRACE: [%p] %s: Tail: %d -> %d \t +%d \t (%d:%d) \t (%d/%d)\n", 
                  this, __func__, RevInfo.OldIndex, RevInfo.NewIndex, RevInfo.__Num, 
                  elm, melm, NumCmPush, NumRvPush); 
      }
      else {
        ::fprintf(stdout, "?RCQ_TRACE: [%p] %s: Tail: %d -> %d \t +%d \t (%d:%d) \t (%d/%d)\n", 
                  this, __func__, RevInfo.OldIndex, RevInfo.NewIndex, RevInfo.__Num, 
                  elm, melm, NumCmPush, NumRvPush); 
      }
      ::fflush(stdout); 
    }
#endif
    return 0;
  }

  /// Reserve multiple queue items to pop
  ///  o return 0: success
  ///  o reutrn -EBUSY: waiting for ticket
  ///  o return -EWOULDBLOCK: queue is empty
  virtual int reservePopN(int Num, QueueWindow (&Win)[2],
                          ReservationInfo& RevInfo, 
                          int ServingTicket, int* IssuingTicket) {
    return reservePeekPopN(Num, Num, Win, RevInfo, ServingTicket, IssuingTicket); 
  }

  /// Commit pop
  virtual int commitPop(ReservationInfo& RevInfo, float* FillRatio = NULL) {
    return commitPeekPop(RevInfo, FillRatio);
  }

  /// Reserve multiple queue items to peek and pop
  ///  o return 0: success
  ///  o reutrn -EINVAL: invalid argument
  ///  o reutrn -EBUSY: waiting for ticket
  ///  o return -EWOULDBLOCK: queue is empty
  virtual int reservePeekPopN(int PeekNum, int PopNum, 
                              QueueWindow (&Win)[2], ReservationInfo& RevInfo, 
                              int ServingTicket, int* IssuingTicket) {
    int numelm, tail, r_head, new_r_head; 
    assert(PeekNum >= PopNum); 
  retry:
    // Get shared variables
    Machine::rmb();
    numelm = NumElm;
    r_head = ReservedHead; 
    tail = Tail; 

    // Check if the requested reservation makes the queue empty
    if (!haveEnoughData(numelm, r_head, tail, PeekNum))
      return -EWOULDBLOCK; 

    // Lazy waiting of my turn for eager -EWOULDBLOCK return 
    if ( !PopTicketServer.isMyTurn(ServingTicket) ) 
      return -EBUSY; 

    // Set new r_head
    new_r_head = r_head + PopNum; 
    if ( unlikely(new_r_head >= numelm) )
      new_r_head -= numelm; 
     
    // Set reserved queue windows
    Win[0].Base = getElementAt(r_head);
    int new_peek_head = r_head + PeekNum; 
    // - case 1: wrapped around
    if ( unlikely(new_peek_head >= numelm) ) {
      new_peek_head -= numelm; 
      Win[0].Num = numelm - r_head; 
      Win[1].Base = getElementAt(0);
      Win[1].Num = new_peek_head; 
    }
    // - case 2: not wrapped around
    else {
      Win[0].Num = PeekNum; 
      Win[1].Base = NULL; 
      Win[1].Num = 0; 
    }

    // Set reservation information
#ifdef DANBI_RCQ_ENABLE_DEBUG_TRACE
    if (DebugTrace)
      RevInfo.__Num = PopNum; 
#endif 
    RevInfo.OldIndex = r_head; 
    RevInfo.NewIndex = new_r_head; 
#ifndef DANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING
    RevInfo.ReadyFlag = 0; 
    RevInfo.Next = NULL; 
    Machine::wmb(); 
#endif 

    // Update ReservedHead
    if (!Machine::casInt<volatile int>(&ReservedHead, r_head, new_r_head))
      goto retry; 

#ifndef DANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING
    // Everything is alright. Update LastReservedPop
    ReservationInfo* old_last = const_cast<ReservationInfo*>( 
      Machine::fasWord<volatile ReservationInfo*>(&LastReservedPop, &RevInfo) ); 
    if (old_last != NULL) 
      old_last->Next = &RevInfo; 
    else 
      RevInfo.ReadyFlag = 1; 
    Machine::wmb(); 
#endif 

    // Consume a pop ticket only for the successful pop
    PopTicketServer.consume_safe(ServingTicket); 

    // Issue a pop ticket only for the successful pop
    PopTicketIssuer.issue(IssuingTicket, r_head, new_r_head); 

#ifdef DANBI_RCQ_ENABLE_DEBUG_TRACE
    if (DebugTrace) {
      int elm = Machine::atomicIntAddReturn<volatile int>(
        &NumFilledElements, -PopNum); 
      int melm = getNumFilledElements();
      Machine::atomicIntAddReturn<volatile int>(&NumRvPop, PopNum); 
      if (elm == melm) {
        ::fprintf(stdout, "=RCQ_TRACE: [%p] %s: RHead: %d -> %d \t +%d \t (%d:%d) \t (%d/%d)\n", 
                  this, __func__, r_head, new_r_head, 
                  PopNum, 
                  elm, melm,
                  NumCmPop, NumRvPop); 
      }
      else {
        ::fprintf(stdout, "?RCQ_TRACE: [%p] %s: RHead: %d -> %d \t +%d \t (%d:%d) \t (%d/%d)\n", 
                  this, __func__, r_head, new_r_head, 
                  PopNum, 
                  elm, melm,
                  NumCmPop, NumRvPop); 
      }
      ::fflush(stdout); 
    }
#endif
    return 0; 
  }

  /// Commit peek and pop
  virtual int commitPeekPop(ReservationInfo& RevInfo, float* FillRatio = NULL) {
#ifndef DANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING
    // Check if it is my commit order 
    if ( !RevInfo.ReadyFlag ) {
      Machine::rmb(); 
      if ( !RevInfo.ReadyFlag )
        return -EBUSY; 
    }

    // If it's my turn, update head.
    if ( !Machine::casInt<volatile int>(&Head, RevInfo.OldIndex, RevInfo.NewIndex) ) {
      // Otherwise, wakeup next
      Machine::rmb(); 
      if (RevInfo.Next) {
        RevInfo.Next->ReadyFlag = 1; 
        Machine::wmb(); 
      }
      return -EBUSY;
    }

    // Update LastReservedPush or wake up the next waiting committer
    if ( !Machine::casWord<volatile ReservationInfo*>(&LastReservedPop, &RevInfo, NULL) ) {
      while (!RevInfo.Next) Machine::rmb(); 
      RevInfo.Next->ReadyFlag = 1; 
      Machine::wmb(); 
    }
#else
    while ( !Machine::casInt<volatile int>(&Head, RevInfo.OldIndex, RevInfo.NewIndex) ) ; 
#endif

    // Calculate how much it is filled
    if ( likely(FillRatio) ) {
      *FillRatio = getFillRatio();
      assert( 0.0f <= *FillRatio && *FillRatio <= 1.0f ); 
    }

#ifdef DANBI_RCQ_ENABLE_DEBUG_TRACE
    if (DebugTrace) {
      int elm = NumFilledElements;
      int melm = getNumFilledElements();
      Machine::atomicIntAddReturn<volatile int>(&NumCmPop, RevInfo.__Num); 
      ::fprintf(stdout, "=RCQ_TRACE: [%p] %s: Head: %d -> %d \t +%d \t (%d:%d) \t (%d/%d)\n", 
                this, __func__, RevInfo.OldIndex, RevInfo.NewIndex, RevInfo.__Num, 
                elm, melm, NumCmPop, NumRvPop); 
      ::fflush(stdout); 
    }
#endif
    return 0;
  }

  /// Consume the issued push ticket
  virtual void consumePushTicket(int Ticket) {
    PushTicketServer.consume(Ticket); 
  }

  /// Consume the issued pop ticket
  virtual void consumePopTicket(int Ticket) {
    PopTicketServer.consume(Ticket); 
  }

  /// Get the number of elements 
  virtual int getNumElement() {
    Machine::rmb();
    return getNumFilledElements();
  }

  /// Drain remaining elements
  virtual void drain() {
    int r_head;
    Machine::rmb();
    r_head = ReservedHead; 
    ReservedTail = r_head; 
    Tail = r_head;
    Machine::wmb();
  }
 
  /// Check if it is empty
  virtual bool empty() {
    int tail, r_head;
    Machine::rmb();
    r_head = ReservedHead; 
    tail = Tail; 
    return r_head == tail;
  }

  /// Debug function 
  virtual void* __hookUpBuffer() {
#ifdef DANBI_RCQ_ENABLE_HOOKUP_BUFFER
    return Buff; 
#else
    return NULL; 
#endif 
  }

  /// Set debug trace 
  virtual void __setDebugTrace(bool DebugTrace_) {
#ifdef DANBI_RCQ_ENABLE_DEBUG_TRACE
    DebugTrace = DebugTrace_;
#endif
  }
};

} // End of danbi namespace

#endif 
