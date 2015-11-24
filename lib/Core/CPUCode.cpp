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

  CPUCode.cpp -- native cpu code implementation
 */
#include <cstdlib>
#include <cerrno>
#include <cassert>
#include <dlfcn.h>
#include "Core/CPUCode.h"
#include "Core/AbstractPartitioner.h"

using namespace danbi;

bool CPUCode::supportDevice(int DeviceIndex) {
  return AbstractPartitioner::CPU_DEVICE_INDEX == DeviceIndex;
}

int CPUCode::loadToDevice(int DeviceIndex) {
  // Check if the device is proper
  if (AbstractPartitioner::CPU_DEVICE_INDEX != DeviceIndex) 
    return -ENOTSUP; 

  /// If it is constructed from function pointer to the kernel main 
  if (KernelMain) 
    return 0; 

  /// If it is constructed from shared lib. 
  void* Handle = ::dlopen(SharedLibPath, RTLD_LAZY); 
  if (!Handle)
    return -ENOENT; 

  KernelMain = reinterpret_cast<NativeKernelMain>(::dlsym(Handle, EntryName));
  if (!KernelMain)
    return -EINVAL; 

  // Never ::dlclose()
  return 0; 
}


