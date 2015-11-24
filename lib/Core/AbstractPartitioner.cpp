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

  AbstractPartitioner.cpp -- common implementation of AbstractPartitioner
 */
#include <algorithm>
#include "Core/AbstractPartitioner.h"
#include "Core/Program.h"
#include "Core/Runtime.h"
#include "Core/Kernel.h"

using namespace danbi;

AbstractPartitioner::~AbstractPartitioner() {
  // do nothing :)
}


void AbstractPartitioner::calcMaxCoresPerDevice(std::vector<Kernel*>& KernelTable) {
  int NumKernel = KernelTable.size(); 

  for (int i = 0; i < NumKernel; ++i) {
    ExecutionKind ExecKind;
    std::map<int,int>::iterator CoreIter; 
    int d; 
    
    d = getComputeDevice(i);
    ExecKind = KernelTable[i]->getExecutionKind(); 
    if (ExecKind == ParallelExec) 
      MaxCorePerDevice[d] = INT_MAX; 
    else {
      CoreIter = MaxCorePerDevice.find(d); 
      if (CoreIter == MaxCorePerDevice.end()) 
        MaxCorePerDevice[d] = 1; 
      else if (MaxCorePerDevice[d] != INT_MAX) 
          MaxCorePerDevice[d] = MaxCorePerDevice[d] + 1; 
    }
  }

  // Upper limit of CPU core assignment
  MaxCorePerDevice[CPU_DEVICE_INDEX] = std::min(MaxCorePerDevice[CPU_DEVICE_INDEX], 
                                                MaxAssignableCPUs); 
}

int AbstractPartitioner::partition() {
  int ret =  partition(Pgm.KernelTable); 
  if (!ret)
    calcMaxCoresPerDevice(Pgm.KernelTable); 
  return ret; 
}

int AbstractPartitioner::getMaxAssignableCores(int ComputeDevice) {
  return MaxCorePerDevice[ComputeDevice]; 
}

