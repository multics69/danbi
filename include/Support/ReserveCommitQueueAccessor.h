/*                                                                 --*- C++ -*-
  Copyright (C) 2012, 2013 Changwoo Min. All Rights Reserved.

  This file is part of DANBI project. 

  NOTICE: All information contained herein is, and remains the property 
  of Changwoo Min. The intellectual and technical concepts contained 
  herein are proprietary to Changwoo Min and may be covered  by patents 
  or patents in process, and are protected by trade secret or copyright law. 
  Dissemination of this information or reproduction of this material is 
  strictly forbidden unless prior written permission is obtained 
  from Changwoo Min(multics69@gmail.com). 

  ReserveCommitQueueAccessor.h -- Accessor to abstract reserve/commit queue
 */
#ifndef DANBI_RESERVE_COMMIT_QUEUE_ACCESSOR_H
#define DANBI_RESERVE_COMMIT_QUEUE_ACCESSOR_H
#include <string.h>
#include <algorithm>
#include <cassert>
#include "Support/Machine.h"
#include "Support/BranchHint.h"

namespace danbi {
struct QueueWindow {
  void* Base; // base address
  int Num; // number of element
};

enum RCLockKind {
  Wait, // No lock is held. 
  Go, // Lock is held. 
  Consumed, // ReserveLock is consumed by the next successor. 
};

struct ReservationInfo {
  int OldIndex; // old index (i.e. head or tail)
  int NewIndex; // new index (i.e. head or tail)
  volatile int ReserveLock; 
  volatile int CommitLock;  
  volatile int ReadyFlag; // Am I ready to commit?
  volatile ReservationInfo* Next; // Next waiting commiter

  int __Num; // Number of requested elments for debugging
};

struct ReserveCommitQueueAccessor {
  QueueWindow Win[2]; 
  ReservationInfo RevInfo; 
  
  inline 
  void* getElementAt(int ElmSize, int Idx) {
    if ( likely(Win[0].Num > Idx) )
      return (char *)(Win[0].Base) + (ElmSize * Idx); 
    else 
      return (char *)(Win[1].Base) + (ElmSize * (Idx - Win[0].Num)); 
  }

  inline
  void copyFromMem(int ElmSize, int Idx, int Num, void* Src) {
    if ( unlikely(Num <=0) )
      return; 

    // Destination buffer forms one chunk. 
    if ( likely(Win[1].Num == 0) )
      ::memcpy((char *)(Win[0].Base) + (ElmSize * Idx), Src, ElmSize * Num); 
    // Destination buffer forms two chunks. 
    else {
      // Copy to the first chunk
      if (Idx < Win[0].Num) {
        int CopyNum = std::min(Win[0].Num - Idx, Num); 
        ::memcpy((char *)(Win[0].Base) + (ElmSize * Idx), Src, ElmSize * CopyNum); 
        Src = (char*)Src + (ElmSize * CopyNum); 
        Num -= CopyNum; 
        Idx = 0; 
      }
      else
        Idx -= Win[0].Num; 

      // Copy to the second chunk 
      if (Num > 0) 
        ::memcpy((char *)(Win[1].Base) + (ElmSize * Idx), Src, ElmSize * Num); 
    }
  }

  inline
  void copyToMem(int ElmSize, int Idx, int Num, void* Dst) {
    if ( unlikely(Num <=0) )
      return; 

    // Destination buffer forms one chunk. 
    if ( likely(Win[1].Num == 0) )
      ::memcpy(Dst, (char *)(Win[0].Base) + (ElmSize * Idx), ElmSize * Num); 
    // Destination buffer forms two chunks. 
    else {
      // Copy from the first chunk
      if (Idx < Win[0].Num) {
        int CopyNum = std::min(Win[0].Num - Idx, Num); 
        ::memcpy(Dst, (char *)(Win[0].Base) + (ElmSize * Idx), ElmSize * CopyNum); 
        Dst = (char*)Dst + (ElmSize * CopyNum); 
        Num -= CopyNum; 
        Idx = 0; 
      }
      else
        Idx -= Win[0].Num; 

      // Copy from the second chunk 
      if (Num > 0) 
        ::memcpy(Dst, (char *)(Win[1].Base) + (ElmSize * Idx), ElmSize * Num); 
    }
  }

  static
  void copy(void *Dst, int ElmSize, int Num, ReserveCommitQueueAccessor& Src, int SrcIdx) {
    // Source buffer forms one chunk. 
    if ( likely(Src.Win[1].Num == 0) )
      ::memcpy(Dst, (char *)(Src.Win[0].Base) + (ElmSize * SrcIdx), ElmSize * Num); 
    // Source buffer forms two chunk. 
    else {
      // Copy to the first chunk
      if (SrcIdx < Src.Win[0].Num) {
        int CopyNum = std::min(Src.Win[0].Num - SrcIdx, Num); 
        ::memcpy(Dst, (char *)(Src.Win[0].Base) + (ElmSize * SrcIdx), ElmSize * CopyNum); 
        Num -= CopyNum; 
        Dst = (char*)Dst + (ElmSize * CopyNum); 
        SrcIdx = 0; 
      }
      else
        SrcIdx -= Src.Win[0].Num; 
        
      // Copy to the second chunk 
      if (Num > 0) 
        ::memcpy(Dst, (char *)(Src.Win[1].Base) + (ElmSize * SrcIdx), ElmSize * Num); 
    }
  }

  inline
  void copy(int ElmSize, int DstIdx, int Num, ReserveCommitQueueAccessor& Src, int SrcIdx) {
    if ( unlikely(Num <=0) )
      return; 

    // Destination buffer forms one chunk. 
    if ( likely(Win[1].Num == 0) )
      copy((char *)(Win[0].Base) + (ElmSize * DstIdx), ElmSize, Num, Src, SrcIdx); 
    // Destination buffer forms two chunks. 
    else {
      // Copy to the first chunk
      if (DstIdx < Win[0].Num) {
        int CopyNum = std::min(Win[0].Num - DstIdx, Num); 
        copy((char *)(Win[0].Base) + (ElmSize * DstIdx), ElmSize, CopyNum, Src, SrcIdx); 
        Num -= CopyNum; 
        SrcIdx += CopyNum; 
        DstIdx = 0;
      }
      else 
        DstIdx -= Win[0].Num;

      // Copy to the second chunk
      if (Num > 0)
        copy((char *)(Win[1].Base) + (ElmSize * DstIdx), ElmSize, Num, Src, SrcIdx); 
    }
  }
};

} // End of danbi namespace

#endif 
