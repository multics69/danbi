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

  AbstractUberKernel.h -- abstract uber kernel
 */
#ifndef DANBI_ABSTRACT_UBER_KERNEL_H
#define DANBI_ABSTRACT_UBER_KERNEL_H
#include <set>
#include "Support/AbstractRunnable.h"
#include "Core/AbstractScheduler.h"
#include "Core/SchedulingEvent.h"
#include "Core/Kernel.h"

namespace danbi {
class Kernel; 
class AbstractCode;
class ReadOnlyBuffer; 

class AbstractUberKernel : virtual public AbstractRunnable {
protected:
  AbstractScheduler* Scheduler; 
  std::set<Kernel*> Kernels; 
  std::set<AbstractCode*> Codes; 
  std::set<ReadOnlyBuffer*> ReadOnlyBuffers; 

public:
  /// Constructor 
  AbstractUberKernel(AbstractScheduler* Scheduler_)
    : Scheduler(Scheduler_) {}

  /// Destructor
  virtual ~AbstractUberKernel() {}

  /// Add a kernel that is able to be executed on native CPU
  virtual void addKernel(Kernel& Knl) {
    Kernels.insert(&Knl); 
    Codes.insert(Knl.Code); 
    for (std::vector<ReadOnlyBuffer*>::iterator
           i = Knl.ReadOnlyBufferTable.begin(), 
           e = Knl.ReadOnlyBufferTable.end(); 
         i != e; ++i) {
      ReadOnlyBuffers.insert(*i);
    }
  }

  /// Uberkernelize
  virtual int uberkernelize() = 0; 

  /// Load to the device
  virtual int loadToDevice(int DeviceIndex) = 0; 
};

} // End of danbi namespace

#endif 
