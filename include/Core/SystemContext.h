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

  SystemContext.h -- System context
 */
#ifndef DANBI_SYSTEM_CONTEXT_H
#define DANBI_SYSTEM_CONTEXT_H
#include "Support/Machine.h"
#include "Support/GreenThread.h"
#include "Support/GreenScheduler.h"
#include "Core/CPUSchedulePolicyAdapter.h"
#include "Core/SchedulingEvent.h"

namespace danbi {
class DistributedDynamicScheduler; 
class Kernel; 

struct SystemContext {
  static DistributedDynamicScheduler* DDScheduler;
  static __thread Kernel* RunningKernel __cacheline_aligned;
  static __thread int RunningKernelIndex;
  static __thread int OldKernelIndex;
  static __thread int QueueIndex;
  static __thread bool Terminating;
  static __thread SchedulingEventKind Event;
  static __thread SchedulingEventKind DbgOriginalEvent; 
  static __thread GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* ReplacingThread; 
};

} // End of danbi namespace 

#endif 
