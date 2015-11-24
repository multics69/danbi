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

  ResizableReserveCommitQueue.h -- implementation class of blocking reserve/commit based 
  circular FIFO queue. 
 */
#ifndef DANBI_RESIZABLE_RESERVE_COMMIT_QUEUE_H
#define DANBI_RESIZABLE_RESERVE_COMMIT_QUEUE_H
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include "Support/Machine.h"
#include "Support/BranchHint.h"
#include "Support/AbstractReserveCommitQueue.h"
#include "Support/Random.h"

namespace danbi {
#define DANBI_RCQ_ENABLE_HOOKUP_BUFFER
#undef DANBI_RCQ_ENABLE_DEBUG_TRACE // SHOULD be turned off at production mode

template <typename PushTicketIssuerTy, typename PushTicketServerTy, 
          typename PopTicketIssuerTy, typename PopTicketServerTy,
          typename BufferManagerTy>
class ResizableReserveCommitQueue : public AbstractReserveCommitQueue {
private:
  PushTicketIssuerTy PushTicketIssuer;
  PushTicketServerTy PushTicketServer;
  PopTicketIssuerTy  PopTicketIssuer; 
  PopTicketServerTy  PopTicketServer;  
  BufferManagerTy BuffMgr; 
  int MinNum, MaxNum;
  float GrowthRatio, ShrinkStartRatio, ShrinkRatio;
  void* Buff; 
  int ElementsPerPage; 
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
  volatile ReservationInfo* PeekPopRCLock; 

  volatile int Tail __cacheline_aligned; 
  volatile int ReservedHead; 
  volatile ReservationInfo* PushRCLock; 

  enum {
    CompactQueueProbMod = 4, // 25%
  };
  
  void operator=(const ResizableReserveCommitQueue&); // Do not implement
  ResizableReserveCommitQueue(const ResizableReserveCommitQueue&); // Do not implement

  inline void* getElementAt(int Index) {
    assert((ElmSize * (long)Index) >= 0); 
    return static_cast<char *>(Buff) + long(ElmSize) * long(Index); 
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
    float r = (float)getNumFilledElements() / (float)(NumElm - 1);
    assert(0.0F <= r && r <= 1.0F);
    return r; 
  }

  inline int getNumOccupiedElements() {
    // { Head, ReservedHead, Tail, ReservedTail }
    //   |<---------------------------------->|
    int r_tail = ReservedTail; 
    int head = Head; 
    int numelm = NumElm;
    
    if (head <= r_tail)
      return r_tail - head; 
    return numelm - (head - r_tail); 
  }

  inline float getOccupiedRatio() {
    int numelm = NumElm;
    return (float)getNumOccupiedElements() / (float)(numelm - 1);
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

  /// Try to acquire reserve lock 
  inline bool tryAcquireReserveLock(volatile ReservationInfo** RCLock,
                                 ReservationInfo& RevInfo) {
    RevInfo.ReserveLock = RevInfo.CommitLock = RCLockKind::Wait;
    RevInfo.Next = NULL; 
    Machine::wmb(); 
    
    if (Machine::casWord<volatile ReservationInfo*>(RCLock, NULL, &RevInfo)) {
      RevInfo.CommitLock = RCLockKind::Go;
      return true; 
    }
    return false; 
  }

  /// Acquire reserve lock 
  inline void acquireReserveLock(volatile ReservationInfo** RCLock,
                                 ReservationInfo& RevInfo) {
    RevInfo.ReserveLock = RevInfo.CommitLock = RCLockKind::Wait;
    RevInfo.Next = NULL; 
    Machine::wmb(); 
    
    ReservationInfo* old = const_cast<ReservationInfo*>( 
      Machine::fasWord<volatile ReservationInfo*>(RCLock , &RevInfo) ); 
    if (old != NULL) {
      old->Next = &RevInfo;
      while (old->ReserveLock != RCLockKind::Go) Machine::rmb();
      old->ReserveLock = RCLockKind::Consumed; 
    }
    else 
      RevInfo.CommitLock = RCLockKind::Go;
  }

  /// Release reserve lock 
  inline void releaseReserveLock(volatile ReservationInfo** RCLock,
                                 ReservationInfo& RevInfo) {
    RevInfo.ReserveLock = RCLockKind::Go; 
  }

  /// Acquire commit lock 
  inline void acquireCommitLock(volatile ReservationInfo** RCLock,
                                ReservationInfo& RevInfo) {
    while (RevInfo.CommitLock != RCLockKind::Go) Machine::rmb(); 
  }

  /// Release commit lock 
  inline void releaseCommitLock(volatile ReservationInfo** RCLock,
                                ReservationInfo& RevInfo) {
    if (!RevInfo.Next) {
      if (Machine::casWord<volatile ReservationInfo*>(RCLock, &RevInfo, NULL))
        return;
      while (!RevInfo.Next) Machine::rmb(); 
    }
    RevInfo.Next->CommitLock = RCLockKind::Go; 
    while (RevInfo.ReserveLock != RCLockKind::Consumed) Machine::rmb(); 
  }

  /// Cancel reserve lock 
  inline void cancelReserveLock(volatile ReservationInfo** RCLock,
                                ReservationInfo& RevInfo) {
    releaseReserveLock(RCLock, RevInfo);
    acquireCommitLock(RCLock, RevInfo);
    releaseCommitLock(RCLock, RevInfo);
  }

  inline bool isResizable() {
    // When a queue is wrapped around, it is not resizable. 
    // [[... data ...,       Tail, ReservedTail},
    //                                     ... empty ...,
    //   {Head, ReservedHead, ... data ...]]
    return Head <= ReservedTail;
  }

  inline int calcGrownQSize(int numelm, int num) {
    return (numelm + num + 1) * (1.0f + GrowthRatio); 
  }

  /// Compact queue 
  inline int compactQueue(int Num) {
    ReservationInfo PeekPopRevInfo; 
    if ((Tail == ReservedTail) && 
        tryAcquireReserveLock(&PeekPopRCLock, PeekPopRevInfo)) {
      // NOTE: Since popper can independently make pregress before acquiring PeekPopRCLock, 
      // we need double check here. 
      Machine::rmb();
      int numelm = NumElm, new_numelm; 
      // CASE A: If the queue status is not changed during acquiring the lock. 
      if (likely(!isResizable())) {
        assert(Head == ReservedHead && Head < NumElm && ReservedHead < NumElm); 
        assert(Tail == ReservedTail && Tail < Head && ReservedTail < Head); 

        // Ok, there is no popper until canceling the PeekPopRCLock. 
        // And, of course, it is the only pusher. 
        int head = Head, r_tail = ReservedTail; 
        int move_num; 

        // CASE 1: append the front tail block to the end of the head block
        if (r_tail <= (numelm - head)) {
          move_num = r_tail; 
          new_numelm = BuffMgr.resize( calcGrownQSize(numelm, Num + move_num) );
          if (unlikely(new_numelm < (numelm + move_num)))
            goto shift_head;
          ::memcpy((char*)Buff + (long(ElmSize) * long(numelm)), 
                   (char*)Buff, 
                   long(ElmSize) * long(move_num)); 
          Tail = ReservedTail = numelm + move_num; 
        }
        // CASE 2: move the head block to the end of grown queue 
        else {
          move_num = numelm - head;
          new_numelm = BuffMgr.resize( calcGrownQSize(numelm, Num + move_num) );
          if (unlikely(new_numelm < (numelm + move_num)))
            goto shift_head;
          ::memcpy((char*)Buff + (long(ElmSize) * long(new_numelm - numelm + head)), 
                   (char*)Buff + (long(ElmSize) * long(head)), 
                   long(ElmSize) * long(move_num)); 
          Head = ReservedHead = new_numelm - numelm + head; 
        }
        // CASE 3: if it fails to grow large enough, shift the head block. 
        while(false) {
          shift_head:
          if (likely(new_numelm > numelm)) {
            move_num = numelm - head;
            ::memmove((char*)Buff + (long(ElmSize) * long(new_numelm - numelm + head)), 
                      (char*)Buff + (long(ElmSize) * long(head)), 
                      long(ElmSize) * long(move_num)); 
            Head = ReservedHead = new_numelm - numelm + head; 
          }
        }
      }
      // CASE B: If the queue status is changed during acquiring the lock. 
      else {
        new_numelm = BuffMgr.resize( calcGrownQSize(numelm, Num) );
      }
      // NOTE: Since there is no other concurrent pushers or poppers other than me, 
      // update ordering between Tail/ReservedTail and NumElm does not matter. 
      cancelReserveLock(&PeekPopRCLock, PeekPopRevInfo);
      return new_numelm; 
    }
    return NumElm;
  }

  /// Grow queue size
  inline int growQueue(int Num) {
    // NOTE: assuming PUSH lock is acquired. 

    // If growing is possible, simply grow the size of the queue. 
    Machine::rmb();
    if (isResizable()) {
      // NOTE: Since it is the only one pusher, 
      //       the queue cannot be changed to be non-resizable. 
      // NOTE: Acquiring POP lock is NOT required, 
      //       since head variables cannot proceed tails when it is resiable. 
      int numelm = NumElm;
      NumElm = BuffMgr.resize( calcGrownQSize(numelm, Num) );
      Machine::wmb();

      assert(Head <= ReservedHead && ReservedHead <= Tail && Tail <=ReservedTail);
      assert(Head < NumElm && ReservedHead < NumElm);
      assert(Tail < NumElm && ReservedTail < NumElm);
    }
    // Otherwise, compact and grow the queue, 
    // only when there is no concurrent popper and concurrent pusher. 
    else if (Tail == ReservedTail) {
      if (likely(Random::randomInt() % CompactQueueProbMod)) {
        NumElm = compactQueue(Num); 
        Machine::wmb();
      }
    }

    return NumElm;
  }

  /// Check if shrink is needed
  inline bool requireShrinkQueue() {
    if (!isResizable())
      return false;

    int numelm = NumElm;
    if ((numelm == MinNum) || ((1.0f - getOccupiedRatio()) < ShrinkStartRatio))
      return false;

    int shrinksize = std::min(numelm - ReservedTail - 1, int(numelm * ShrinkRatio));
    if (shrinksize <= ElementsPerPage) 
      return false; 

    return true; 
  }

  /// Shink queue size if needed
  inline int shrinkQueueIfNeeded() {
    // NOTE: assuming POP lock is acquired. 
    // Check if shrink is needed. 
    if (!requireShrinkQueue()) 
      return NumElm;

    // Acquire PUSH lock. 
    ReservationInfo RevInfo;
    acquireReserveLock(&PushRCLock, RevInfo); 
    do { 
      // Double check if shrink is needed.
      Machine::rmb();
      if (!requireShrinkQueue()) 
        break;

      // Resize queue
      // [[... empty ..., 
      //                 {Head, ReservedHead, ... data ..., Tail, ReservedTail}, 
      //   ... empty ...]]
      int numelm = NumElm;
      NumElm = BuffMgr.resize(std::max(
                                ReservedTail + 1, 
                                numelm - int(numelm * ShrinkRatio)));
      Machine::wmb();

      assert(Head <= ReservedHead && ReservedHead <= Tail && Tail <=ReservedTail);
      assert(Head < NumElm && ReservedHead < NumElm);
      assert(Tail < NumElm && ReservedTail < NumElm);
    } while(false);
    cancelReserveLock(&PushRCLock, RevInfo);
    return NumElm;
  }

public:
  /// Constructor
  ResizableReserveCommitQueue(int ElmSize_, 
                              int MinNum_ = 1024, 
                              int MaxNum_ = (256 * 1024 * 1024), 
                              float GrowthRatio_ = 1.1f,
                              float ShrinkStartRatio_ = 0.9f,
                              float ShrinkRatio_ = 0.4f)
    : AbstractReserveCommitQueue(ElmSize_, MinNum_), 
      BuffMgr(ElmSize, NumElm, MaxNum_), 
      MinNum(MinNum_), MaxNum(MaxNum_),
      GrowthRatio(GrowthRatio_), ShrinkStartRatio(ShrinkStartRatio_), ShrinkRatio(ShrinkRatio_),
      Buff(BuffMgr.getInitialBuffer()), 
      Head(0), ReservedHead(0), PushRCLock(NULL), 
      Tail(0), ReservedTail(0), PeekPopRCLock(NULL) {
    // Reinit. queue size because ResizableBufferManager can allocate elements in page size.
    NumElm = BuffMgr.size();
    ElementsPerPage = BufferManagerTy::PageSize / ElmSize;
#ifdef DANBI_RCQ_ENABLE_DEBUG_TRACE
    NumFilledElements = NumRvPush = NumCmPush = NumRvPop = NumCmPop = 0; 
#endif
  }

  /// Destructor
  virtual ~ResizableReserveCommitQueue() {
    // do nothing
  }

  /// Reserve multiple queue items to push
  ///  o return 0: success
  ///  o reutrn -EBUSY: waiting for ticket
  ///  o return -EWOULDBLOCK: queue is full 
  virtual int reservePushN(int Num, QueueWindow (&Win)[2], 
                           ReservationInfo& RevInfo, 
                           int ServingTicket, int* IssuingTicket) {
    // Waiting of my turn for eager -EWOULDBLOCK return 
    if ( !PushTicketServer.isMyTurn(ServingTicket) ) 
      return -EBUSY; 

    // Queue operation
    // - Acquire reserve lock 
    acquireReserveLock(&PushRCLock, RevInfo); 
    do {
      int numelm, head, r_tail, new_r_tail; 

      // Get shared variables
      Machine::rmb();
      numelm = NumElm;
      r_tail = ReservedTail; 
      head = Head; 
 
      // Check if the requested reservation makes the queue full
      if (!haveEnoughRoom(numelm, r_tail, head, Num)) {
        // Grow queue size, and then check again if there is room. 
        int new_numelm = growQueue(Num);
        if (!haveEnoughRoom(new_numelm, r_tail, head, Num)) {
          cancelReserveLock(&PushRCLock, RevInfo);
          return -EWOULDBLOCK; 
        }
        numelm = new_numelm;
      }

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
      
      // Update reserved tail and set reservation information
      ReservedTail = new_r_tail;
      RevInfo.NewIndex = new_r_tail; 
      Machine::wmb();
#ifdef DANBI_RCQ_ENABLE_DEBUG_TRACE
      if (DebugTrace) {
        RevInfo.__Num = Num; 
        NumRvPush += RevInfo.__Num;
      }
#endif 
      
      // Consume a push ticket only for the successful push 
      PushTicketServer.consume_safe(ServingTicket); 

      // Issue a push ticket only for the successful pop
      PushTicketIssuer.issue(IssuingTicket, r_tail, new_r_tail); 
    } while(false); 

    // Release reserve lock 
    releaseReserveLock(&PushRCLock, RevInfo); 
    return 0; 
  }

  /// Commit push 
  virtual int commitPush(ReservationInfo& RevInfo, float* FillRatio = NULL) {
    // Acquire commit lock 
    acquireCommitLock(&PushRCLock, RevInfo); 

    // Update tail
    Tail = RevInfo.NewIndex;
    Machine::wmb();

    // Calculate how much it is filled
    if ( likely(FillRatio) )
      *FillRatio = getFillRatio();

#ifdef DANBI_RCQ_ENABLE_DEBUG_TRACE
    if (DebugTrace) {
      int elm = Machine::atomicIntAddReturn<volatile int>(
        &NumFilledElements, RevInfo.__Num); 
      int melm = getNumFilledElements();
      Machine::atomicIntAddReturn<volatile int>(&NumCmPush, RevInfo.__Num); 
      if (elm == melm) {
        ::fprintf(stdout, "=RCQ_TRACE: [%p] %s: Tail: %d \t +%d \t (%d:%d) \t (%d/%d)\n", 
                  this, __func__, RevInfo.NewIndex, RevInfo.__Num, 
                  elm, melm, NumCmPush, NumRvPush); 
      }
      else {
        ::fprintf(stdout, "?RCQ_TRACE: [%p] %s: Tail: %d \t +%d \t (%d:%d) \t (%d/%d)\n", 
                  this, __func__, RevInfo.NewIndex, RevInfo.__Num, 
                  elm, melm, NumCmPush, NumRvPush); 
      }
      ::fflush(stdout); 
    }
#endif

    // Release commit lock 
    releaseCommitLock(&PushRCLock, RevInfo); 
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
    int ret = 0; 

    assert(PeekNum >= PopNum); 

    // Get shared variables
    numelm = NumElm;
    r_head = ReservedHead; 
    tail = Tail; 

    // Check if the requested reservation makes the queue empty
    if (!haveEnoughData(numelm, r_head, tail, PeekNum))
      return -EWOULDBLOCK; 

    // Lazy waiting of my turn for eager -EWOULDBLOCK return 
    if ( !PopTicketServer.isMyTurn(ServingTicket) ) 
      return -EBUSY; 

    // Acquire reserve lock 
    acquireReserveLock(&PeekPopRCLock, RevInfo); 

    // Queue operation
    do {
      // Get shared variables
      Machine::rmb();
      numelm = NumElm;
      r_head = ReservedHead; 
      tail = Tail; 
      
      // Check if the requested reservation makes the queue empty
      if (!haveEnoughData(numelm, r_head, tail, PeekNum)) {
        cancelReserveLock(&PeekPopRCLock, RevInfo);
        return -EWOULDBLOCK; 
      }

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
      
      // Update reserved head and set reservation information
      ReservedHead = new_r_head;
      RevInfo.NewIndex = new_r_head; 
      Machine::wmb();

      // Consume a pop ticket only for the successful pop
      PopTicketServer.consume_safe(ServingTicket); 
      
      // Issue a pop ticket only for the successful pop
      PopTicketIssuer.issue(IssuingTicket, r_head, new_r_head); 

      // Shrink queue size if needed.
      shrinkQueueIfNeeded();

#ifdef DANBI_RCQ_ENABLE_DEBUG_TRACE
      if (DebugTrace) {
        RevInfo.__Num = PopNum;  
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
    } while (false); 

    // Release reserve lock 
    releaseReserveLock(&PeekPopRCLock, RevInfo); 
    return 0; 
  }

  /// Commit peek and pop
  virtual int commitPeekPop(ReservationInfo& RevInfo, float* FillRatio = NULL) {
    // Acquire commit lock 
    acquireCommitLock(&PeekPopRCLock, RevInfo); 

    // Update head
    Head = RevInfo.NewIndex;
    Machine::wmb();

    // Calculate how much it is filled
    if ( likely(FillRatio) )
      *FillRatio = getFillRatio();

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

    // Release commit lock 
    releaseCommitLock(&PeekPopRCLock, RevInfo); 
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
