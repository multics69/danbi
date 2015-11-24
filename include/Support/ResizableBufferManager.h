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

  ResizableBufferManager.h -- Resizable buffer manager for circular quque. 
 */

#ifndef DANBI_RESIZABLE_BUFFER_MANAGER_H
#define DANBI_RESIZABLE_BUFFER_MANAGER_H
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <sys/mman.h>
#include <math.h>

namespace danbi {
class ResizableBufferManager {
public:
  enum {
    PageSize = 4096, 
    PageSizeBitMask = 0xFFF,
  }; 

private:
  long ElmSize; 
  char* Buff; 
  long MinNum, MaxNum, CurNum; 
  
  void operator=(const ResizableBufferManager&); // Do not implement
  ResizableBufferManager(const ResizableBufferManager&); // Do not implement 

  long ceilPageSize(long Size) {
    return (Size + PageSize - 1) & ~PageSizeBitMask; 
  }

public:
  /// constructor 
  ResizableBufferManager(long ElmSize_, long MinNum_, long MaxNum_ = 0)
    : ElmSize(ElmSize_), Buff(NULL) {
    // Calculate MinNum and MaxNum
    MinNum = std::max(MinNum_, PageSize/ElmSize);
    MaxNum = std::max(MaxNum_, MinNum); 

    // Allcate virtual address of the requested buffer
    Buff = (char *)::mmap(NULL, ElmSize * MaxNum, PROT_READ | PROT_WRITE, 
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); 
    assert(Buff != MAP_FAILED);

    // Shink to the minimum number
    if (MaxNum > MinNum) {
      int ret = ::madvise(
        Buff + (ElmSize * MinNum), ElmSize * (MaxNum - MinNum), MADV_DONTNEED); 
      assert(ret == 0); 
    }
    CurNum = MinNum; 
  }

  /// destructor
  virtual ~ResizableBufferManager() {
    if (Buff) {
      int ret = ::munmap(Buff, ElmSize * MaxNum); 
      assert(ret == 0);
      Buff = NULL; 
    }
  }

  /// get the initial buffer
  inline void* getInitialBuffer() {
    return Buff; 
  }

  /// Resize buffer
  inline long resize(long NumElm) {
    long OrgNumElm = NumElm; 

    // Calculate proper NumElm
    assert(NumElm > 0);
    NumElm = std::min( std::max(NumElm, MinNum), MaxNum);
    long NumPages = ::ceil(double(NumElm * ElmSize) / double(PageSize));
    // Check if there is floating point precision error
   calc_numelm:
    NumElm = std::min((NumPages * PageSize) / ElmSize, MaxNum); 
    if (unlikely((NumElm < OrgNumElm) && (NumElm < MaxNum))) {
      NumPages++; 
      goto calc_numelm;
    }
    assert(NumElm >= MinNum && NumElm <= MaxNum);
    
    // Shrink
    if (NumElm < CurNum) {
      long PageAlignedSize = ceilPageSize(ElmSize * NumElm);
      long Delta = (CurNum * ElmSize) - PageAlignedSize + ElmSize;
      if (likely(Delta > 0)) {
        int ret = ::madvise(Buff+PageAlignedSize, Delta, MADV_DONTNEED); 
        assert(ret == 0); 
      }
    }

    // Update CurNum 
    CurNum = NumElm; 
    assert(CurNum > 0 && (int)CurNum > 0);
    return CurNum;
  }

  /// Get the size of buffer
  inline long size() {
    return CurNum;
  }
};

} // End of danbi namespace

#endif 
