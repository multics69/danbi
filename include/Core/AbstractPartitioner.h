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

  AbstractPartitioner.h -- abstract kernel partitioner class for core assignment
 */
#ifndef DANBI_ABSTRACT_PARTITIONER_H
#define DANBI_ABSTRACT_PARTITIONER_H
#include <limits.h>
#include <map>
#include <vector>

namespace danbi {
class Runtime; 
class Program; 
class Kernel; 

class AbstractPartitioner {
  friend class AbstractScheduler; 

public:
  enum {
   CPU_DEVICE_INDEX = -1
  }; 

private:  
  void operator=(const AbstractPartitioner&); // Do not implement
  AbstractPartitioner(const AbstractPartitioner&); // Do not implement

  /// Calculate maximum number of assigable cores per compute device
  void calcMaxCoresPerDevice(std::vector<Kernel*>& KernelTable);
protected:
  Runtime& Rtm; 
  Program& Pgm;
  std::multimap<int,int> ComputeDeviceToKernelMap; 
  std::map<int,int> KernelToComputeDeviceMap;
  std::map<int,int> MaxCorePerDevice; 
  int MaxAssignableCPUs; 

  /// Inherited concrete class should implement this. 
  virtual int partition(std::vector<Kernel*>& KernelTable) = 0; 

public:
  /// Constructor
  AbstractPartitioner(Runtime& Rtm_, Program& Pgm_, int MaxAssignableCPUs_ = INT_MAX)
    : Rtm(Rtm_), Pgm(Pgm_), MaxAssignableCPUs(MaxAssignableCPUs_) {}

  /// Destructor
  virtual ~AbstractPartitioner(); 

  /// Kernel partitioning and assigning to cores
  virtual int partition(); 

  /// Get assigned compute device
  int getComputeDevice(int KernelIndex) {
    return KernelToComputeDeviceMap[KernelIndex];
  }

  /// Get maximum assignable cores per device
  int getMaxAssignableCores(int ComputeDevice); 
};

} /// End of danbi namespace
#endif 
