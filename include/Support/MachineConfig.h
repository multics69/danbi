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

  MachineConfig.h -- singleton class to get machine configuration 
  such as CPUs and GPUs information
 */

#ifndef DANBI_MACHINE_CONFIG_H
#define DANBI_MACHINE_CONFIG_H
#ifdef DANBI_OPENCL_ENABLED
#include <CL/opencl.h>
#endif 

#include <vector>
#include <cassert>

namespace danbi {
struct GPUInfo {
#ifdef DANBI_OPENCL_ENABLED
  cl_platform_id PlatfomID;
  cl_device_id GPUID;
#endif 
  int NativeSIMTWidth; 
  int NumComputeUnit; 
  int LocalMemSize; 
};

class MachineConfig {
private:
  int NumCoresInCPUs; 
  int NumHardwareThreadsInCPUs; 
  std::vector<GPUInfo> GPUInfos; 

  MachineConfig(); // getInstance() is the only way to create this class
public:
  /// get singleton instance 
  static MachineConfig& getInstance(); 

  /// get the number of cores in CPUs
  int getNumCoresInCPUs() const {
    return NumCoresInCPUs; 
  }

  /// get the number of hardware threads in CPU
  int getNumHardwareThreadsInCPUs() const {
    return NumHardwareThreadsInCPUs; 
  }

  /// get the number of GPUs
  int getNumGPUs() const {
    return GPUInfos.size();
  }

  /// get the GPU info.
  const GPUInfo& getGPUInfo(int GPUID) const {
    return GPUInfos[GPUID];
  }
};

} // End of danbi namespace

#endif 
