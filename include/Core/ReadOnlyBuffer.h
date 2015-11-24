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

  ReadOnlyBuffer.h -- read-only buffer 
 */
#ifndef DANBI_READ_ONLY_BUFFER_H
#define DANBI_READ_ONLY_BUFFER_H
#include <cassert>
#include <string.h>
#include "Support/Indexable.h"
#include "Core/AbstractPartitioner.h"

namespace danbi {
class ReadOnlyBuffer : public Indexable {
private:
  const void* Buff; 
  int ElmSize; 
  int DimX, DimY, DimZ; 

  void operator=(const ReadOnlyBuffer&); // Do not implement
  ReadOnlyBuffer(const ReadOnlyBuffer&); // Do not implement

public:
  /// constructor
  ReadOnlyBuffer(const void* Buff_, int ElmSize_, 
                 int DimX_, int DimY_ = 1, int DimZ_ = 1) 
    : Buff(Buff_), ElmSize(ElmSize_), DimX(DimX_), DimY(DimY_), DimZ(DimZ_) {}

  /// destructor
  ~ReadOnlyBuffer() {}

  /// load constant values
  int loadToDevice(int DeviceIndex) {
    if (DeviceIndex == AbstractPartitioner::CPU_DEVICE_INDEX)
      return 0; 
    return -ENOTSUP; 
  }

  /// getElementAt
  inline const void* getElementAt(int X, int Y = 0, int Z = 0) {
    assert( ((X < DimX) && (Y < DimY) && (Z < DimZ)) &&
            "Out of bound error"); 
    return static_cast<const char *>(Buff) + 
      (X + (Y*DimX) + (Z*DimX*DimY)) * ElmSize; 
  }
};

} // End of danbi namespace
#endif
