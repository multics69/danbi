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

  Runtime.h -- danbi runtime class
 */
#ifndef DANBI_RUNTIME_H
#define DANBI_RUNTIME_H
#include <cerrno>
#include <map>
#include <string>

namespace danbi {
class Program; 
class AbstractPartitioner; 
class AbstractScheduler; 
class AbstractUberKernel; 
class AbstractComputeDevice; 

class Runtime {
  friend class AbstractPartitioner; 
  friend class AbstractScheduler; 

private:
  Program* Pgm;
  AbstractPartitioner* Partitioner;
  AbstractScheduler* Scheduler; 
  std::map<int,AbstractUberKernel*> Uberkernels; // Device ID to uberkernel map
  std::map<int,AbstractComputeDevice*> ComputeDevices; // Device ID to device map
  
  void operator=(const Runtime&); // Do not implement
  Runtime(const Runtime&); // Do not implement

  /// Build per-device uber kernel
  int buildPerDeviceUberKernel(); 

  /// Create compute devices
  int createComputeDevices(); 
public:
  /// Constructor 
  Runtime();

  /// Destructor
  virtual ~Runtime(); 

  /// Load Program: one runtime can execute only one program
  int loadProgram(Program* Pgm_, 
                  AbstractPartitioner* Partitioner_, 
                  AbstractScheduler* Scheduler_);

  /// Prepare to run a program 
  int prepare(); 

  /// Run a program with given partitioning and scheduling policy 
  int start(); 

  /// Join all workers
  int join(); 

  /// Generate log
  int generateLog(std::string SimpleGraphPath, std::string FullGraphPath); 
};

} // End of danbi namespace

#endif 
