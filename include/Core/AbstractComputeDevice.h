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

  AbstractComputeDevice.h -- compute device such as CPUs and GPU
 */
#ifndef DANBI_ABSTRACT_COMPUTE_DEVICE_H
#define DANBI_ABSTRACT_COMPUTE_DEVICE_H

namespace danbi {
class AbstractUberKernel; 
class AbstractScheduler; 

class AbstractComputeDevice {
private:
  void operator=(const AbstractComputeDevice&); // Do not implement
  AbstractComputeDevice(const AbstractComputeDevice&); // Do not implement
  
protected:
  int MaxAssignableCores; 
  int NumAllKernel; 
  AbstractScheduler& Scheduler; 
  AbstractUberKernel& UberKernel; 

public:
  /// Constructor 
  AbstractComputeDevice(int MaxAssignableCores_, 
                        int NumAllKernel_, 
                        AbstractScheduler& Scheduler_, 
                        AbstractUberKernel& UberKernel_) 
    : MaxAssignableCores(MaxAssignableCores_), NumAllKernel(NumAllKernel_), 
      Scheduler(Scheduler_), UberKernel(UberKernel_) {}

  /// Destructor 
  virtual ~AbstractComputeDevice() {}

  /// Prepare execution 
  virtual int prepare() = 0; 

  /// Start execution 
  virtual int start() = 0; 

  /// Join 
  virtual int join() = 0; 
};

} // End of danbi namespace

#endif 
