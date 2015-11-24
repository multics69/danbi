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

  AllCPUPartitioner.cpp -- implementation of all CPU partitioner
 */
#include "Core/AllCPUPartitioner.h"
#include "Core/Runtime.h"
#include "Core/Program.h"

using namespace danbi; 

int AllCPUPartitioner::partition(std::vector<Kernel*>& KernelTable) {
  // Assign all kernels to CPU
  for (int i=0, e=KernelTable.size(); i < e; ++i) {
    ComputeDeviceToKernelMap.insert( 
      std::pair<int,int>(AbstractPartitioner::CPU_DEVICE_INDEX, i)); 

    KernelToComputeDeviceMap.insert(
      std::pair<int,int>(i, AbstractPartitioner::CPU_DEVICE_INDEX)); 
  }
  return 0; 
}

