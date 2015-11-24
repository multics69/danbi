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

  AbstractCode.h -- Abstract code class representing kernel code
 */
#ifndef DANBI_ABSTRACT_CODE_H
#define DANBI_ABSTRACT_CODE_H
#include <cerrno>
#include "Support/Indexable.h"
#include "Core/SchedulingEvent.h"

namespace danbi {
class Kernel; 

class AbstractCode : public Indexable {
private:
public:
  /// Does this code support execution of a particular device?
  virtual bool supportDevice(int DeviceIndex) = 0; 

  /// Load to the device
  virtual int loadToDevice(int DeviceIndex) = 0; 

  /// Load the compiled code into GPU
  virtual void run() = 0; 
}; 

} // End of danbi namespace
#endif 
