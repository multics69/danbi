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

  CPUCode.h -- Native CPU code
 */
#ifndef DANBI_CPU_CODE_H
#define DANBI_CPU_CODE_H
#include "Core/AbstractCode.h"

namespace danbi {
class Kernel; 

/// prototype of kernal main function 
typedef void (*NativeKernelMain)(void);

/// Native CPU code 
class CPUCode : public AbstractCode {
private:
  NativeKernelMain KernelMain; // Function pointer to the native kernel main function 
  const char* SharedLibPath; // Valid only if loading from shared lib. 
  const char* EntryName; // Valid only if loading from shared lib. 

  void operator=(const CPUCode&); // Do not implement
  CPUCode(const CPUCode&); // Do not implement

public:
  /// Conctructor with function pointer
  CPUCode(NativeKernelMain KernelMain_)
    : KernelMain(KernelMain_), SharedLibPath(NULL), EntryName(NULL) {}

  /// Constructor with shared library and entry point name
  CPUCode(const char* SharedLibPath_, const char* EntryName_)
    :KernelMain(NULL), SharedLibPath(SharedLibPath_), EntryName(EntryName_) {}
  
  /// Does this code support execution of a particular device?
  virtual bool supportDevice(int DeviceIndex);

  /// Load to the device
  virtual int loadToDevice(int DeviceIndex);

  /// Load the compiled code into GPU
  virtual void run() {
    KernelMain(); 
  }
};

} // End of danbi namespace
#endif 
