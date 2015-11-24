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

  MachineConfig.cpp -- This file implements the MachineConfig class.
 */
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <string.h>
#include "Support/MachineConfig.h"
#include "Support/Machine.h"

using namespace danbi; 

#define CHECK_OPENCL_ERROR_AND_ABORT(_error) do {                       \
  if ((_error) != CL_SUCCESS) {                                         \
    std::cerr << "OpenCL error at line "                                \
              << __LINE__ << " in " << __func__                         \
              << ": error = " << _error << std::endl;                   \
      ::abort();                                                        \
    }                                                                   \
} while(0)

MachineConfig& MachineConfig::getInstance() {
  static MachineConfig Instance; 
  return Instance; 
}

static inline int virtual2PhysicalCoreRatio(void) {
  /// TODO: Although this code is intended to get nubmer of 
  /// physical and virtual cores by using cpuid instruction. 
  /// However, it does not work properly for some CPUs. 
  /// Need to re-implement from scratch. 
  unsigned int regs[4];

  /// Get vendor
  char vendor[12];
  __asm__ __volatile__(
    "cpuid" 
    : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
    : "a" (0), "c" (0)
    );

  ((unsigned int *)vendor)[0] = regs[1]; // EBX
  ((unsigned int *)vendor)[1] = regs[3]; // EDX
  ((unsigned int *)vendor)[2] = regs[2]; // ECX
  std::string cpuVendor = std::string(vendor, 12);

  /// Logical core count per CPU
  __asm__ __volatile__(
    "cpuid" 
    : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
    : "a" (1), "c" (0)
    );
  unsigned int logical = (regs[1] >> 16) & 0xff; // EBX[23:16]
  unsigned int cores = logical;
  unsigned int cpuFeatures = regs[3]; // EDX

  if (cpuVendor == "GenuineIntel") {
    __asm__ __volatile__(
      "cpuid" 
      : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
      : "a" (4), "c" (0)
    );
    cores = ((regs[0] >> 26) & 0x3f) + 1; // EAX[31:26] + 1
  } else if (cpuVendor == "AuthenticAMD") {
    __asm__ __volatile__(
      "cpuid" 
      : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
      : "a" (0x80000008), "c" (0)
    );
    cores = ((unsigned)(regs[2] & 0xff)) + 1; // ECX[7:0] + 1
  }

  /// check hyperthread is supported and enabled. 
  return (cpuFeatures & (1 << 28) && cores < logical) ? 2 : 1;
}

MachineConfig::MachineConfig() {
  /// Get CPU info. 
  NumHardwareThreadsInCPUs = ::sysconf(_SC_NPROCESSORS_CONF);
  NumCoresInCPUs = NumHardwareThreadsInCPUs / virtual2PhysicalCoreRatio();

#ifdef DANBI_OPENCL_ENABLED
  /// Get GPU info. 
  cl_int Error;
  cl_platform_id* PlatformIDs;
  cl_uint NumPlatforms; 
  cl_device_id* GPUIDs;
  cl_uint NumGPUsForPlatform; 
  GPUInfo TempGPUInfo; 
  char TempPlatformName[1024]; 

  // Get platform IDs
  Error = ::clGetPlatformIDs(0, NULL, &NumPlatforms);
  CHECK_OPENCL_ERROR_AND_ABORT(Error); 
  PlatformIDs = new cl_platform_id[NumPlatforms];
  Error = ::clGetPlatformIDs(NumPlatforms, PlatformIDs, NULL);
  CHECK_OPENCL_ERROR_AND_ABORT(Error); 
  
  // For each platform 
  for (int p = 0; p < NumPlatforms; ++p) {
    // Set platform ID
    TempGPUInfo.PlatfomID = PlatformIDs[p]; 

    // Get native SIMT width accroding to the platform name
    Error = ::clGetPlatformInfo(PlatformIDs[p], CL_PLATFORM_NAME, 
                                1024, TempPlatformName, NULL); 
    CHECK_OPENCL_ERROR_AND_ABORT(Error); 
    if (::strstr(TempPlatformName, "NVIDIA") != NULL)
      TempGPUInfo.NativeSIMTWidth = 32; 
    else if (::strstr(TempPlatformName, "AMD") != NULL)
      TempGPUInfo.NativeSIMTWidth = 64;
    else {
      std::cerr << "Unsupported OpenCL platform: " 
                << TempPlatformName << std::endl;
      ::abort(); 
    }
      
    // Retrieve information for each GPU
    Error = ::clGetDeviceIDs(PlatformIDs[p], CL_DEVICE_TYPE_GPU, 
                             0, NULL, &NumGPUsForPlatform);
    CHECK_OPENCL_ERROR_AND_ABORT(Error); 
    GPUIDs = new cl_device_id[NumGPUsForPlatform];
    Error = ::clGetDeviceIDs(PlatformIDs[p], CL_DEVICE_TYPE_GPU, 
                             NumGPUsForPlatform, GPUIDs, NULL);
    CHECK_OPENCL_ERROR_AND_ABORT(Error); 
    for (int g = 0; g < NumGPUsForPlatform; ++g) {
      // Set GPU ID
      TempGPUInfo.GPUID = GPUIDs[g];

      // Set compute unit
      Error = ::clGetDeviceInfo(TempGPUInfo.GPUID, CL_DEVICE_MAX_COMPUTE_UNITS,
                                sizeof(TempGPUInfo.NumComputeUnit), 
                                &TempGPUInfo.NumComputeUnit, NULL); 
      CHECK_OPENCL_ERROR_AND_ABORT(Error); 

      // Check local memory type
      cl_device_local_mem_type LocalMemType; 
      Error = ::clGetDeviceInfo(TempGPUInfo.GPUID, CL_DEVICE_LOCAL_MEM_TYPE,
                                sizeof(LocalMemType), &LocalMemType, NULL); 
      CHECK_OPENCL_ERROR_AND_ABORT(Error); 
      if (LocalMemType != 1) {
        std::cerr 
          << "GPU device without dedicated local memory is not spported." 
          << std::endl;
      ::abort(); 
      }

      // Set local memory size
      cl_ulong MemSize; 
      Error = ::clGetDeviceInfo(TempGPUInfo.GPUID, CL_DEVICE_LOCAL_MEM_SIZE,
                                sizeof(MemSize), 
                                &MemSize, NULL); 
      CHECK_OPENCL_ERROR_AND_ABORT(Error); 
      TempGPUInfo.LocalMemSize = (int)MemSize; 

      // Add to the vector
      GPUInfos.push_back(TempGPUInfo);
    }
    delete [] GPUIDs; 
  }
  delete [] PlatformIDs; 
  
  // sort by te number of compute unit
  std::sort(GPUInfos.begin(), GPUInfos.end(), 
            [](const GPUInfo& lhs, const GPUInfo& rhs) {
              return lhs.NumComputeUnit > rhs.NumComputeUnit;
            }
    );
#endif 
}

