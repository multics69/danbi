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

  AllCPUPartitioner.h -- kernel partitioner which assigns all kernels to CPU
 */
#ifndef DANBI_ALL_CPU_PARTITIONER_H
#define DANBI_ALL_CPU_PARTITIONER_H
#include "Core/AbstractPartitioner.h"

namespace danbi {

class AllCPUPartitioner : public AbstractPartitioner {
private:
  void operator=(const AllCPUPartitioner&); // Do not implement
  AllCPUPartitioner(const AllCPUPartitioner&); // Do not implement

  virtual int partition(std::vector<Kernel*>& KernelTable); 

public:
  /// Constructor
  AllCPUPartitioner(Runtime& Rtm_, Program& Pgm_, int MaxAssignableCPUs_ = INT_MAX) 
    : AbstractPartitioner(Rtm_, Pgm_, MaxAssignableCPUs_) {}
}; 

} // End of danbi namespace

#endif 
