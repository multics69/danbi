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

  PerCPUData.h -- Per-CPU data structure
 */
#ifndef DANBI_PER_CPU_DATA_H
#define DANBI_PER_CPU_DATA_H
#include <cstdlib>
#include <cerrno>
#include <malloc.h>
#include "Support/Machine.h"
#include "Support/MachineConfig.h"

namespace danbi {

template <typename T> 
class PerCPUData {
private:
  struct TA {
    T V __cacheline_aligned; 
  };
  static_assert( sizeof(TA)%Machine::CachelineSize == 0, 
                 "per-CPU data should be cacheline aligned." ); 

  TA* Data; /// memory accessed from user

public:
  /// Constructor 
  PerCPUData(): Data(NULL) {}

  /// Initialize
  int initialize(int NumHWThr = -1) {
    // Get the number of H/W threads
    if (NumHWThr == -1)
      NumHWThr = MachineConfig::getInstance().getNumHardwareThreadsInCPUs();

    // Allocate per-CPU data array
    Data = (TA *)::memalign(Machine::CachelineSize, sizeof(TA) * NumHWThr); 
    if (!Data) return -ENOMEM; 

    return 0; 
  }

  /// Destructor
  ~PerCPUData() {
    if (Data) {
      ::free((void*)Data); 
      Data = NULL; 
    }
  }

  /// Array access
  inline T& operator[](int Index) {
    return Data[Index].V;
  }
}; 

} // End of danbi namespace 

#endif 
