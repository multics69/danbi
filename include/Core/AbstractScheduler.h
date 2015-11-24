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

  AbstractScheduler.h -- AbstractScheduler class
 */
#ifndef DANBI_ABSTRACT_SCHEDULER_H
#define DANBI_ABSTRACT_SCHEDULER_H
#include <vector>
#include <map>
#include "Support/ConnectivityMatrix.h"
#include "Core/Runtime.h"
#include "Core/Program.h"
#include "Core/SchedulingEvent.h"

namespace danbi {
class QueueInfo; 
class AbstractPartitioner; 
class Kernel; 

class AbstractScheduler {
private:
  void operator=(const AbstractScheduler&); // Do not implement
  AbstractScheduler(const AbstractScheduler&); // Do not implement

protected:
  Runtime& Rtm; 
  Program& Pgm; 

  /// Build scheduling information from connectivity and core assignment
  virtual int buildSchedulingInfo(
    std::vector<Kernel*>& KernelTable, 
    std::vector<QueueInfo*>& QueueInfoTable, 
    ConnectivityMatrix<QueueInfo*>& ConnMatrix,
    AbstractPartitioner& Partitioner) = 0; 

public:
  /// Constructor
  AbstractScheduler(Runtime& Rtm_, Program& Pgm_)
    : Rtm(Rtm_), Pgm(Pgm_) {}

  /// Destructor
  virtual ~AbstractScheduler() {}

  /// Build up scheduling information 
  int buildSchedulingInfo() {
    // build scheduling information 
    return buildSchedulingInfo(Pgm.KernelTable, Pgm.QueueInfoTable,
                               Pgm.ConnMatrix, *Rtm.Partitioner);
  }

  /// Decide next schedulable kernel
  virtual int schedule(int KernelIndex, int QueueIndex, SchedulingEventKind Event) = 0; 

  /// Decide start kernel 
  virtual int start() = 0; 

  /// Initialize kernel execution 
  virtual void initKernelExecution() = 0; 

  /// Check if a kernel keeps running
  virtual bool keepContinue() = 0; 
};

} // End of danbi 

#endif 
