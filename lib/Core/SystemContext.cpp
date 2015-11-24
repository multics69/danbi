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

  SystemContext.cpp -- System context
 */
#include "Core/SystemContext.h"

namespace danbi {
DistributedDynamicScheduler* SystemContext::DDScheduler = NULL; 
__thread Kernel* SystemContext::RunningKernel = NULL;
__thread int SystemContext::RunningKernelIndex = -1; 
__thread int SystemContext::OldKernelIndex = -1; 
__thread int SystemContext::QueueIndex = -1; 
__thread bool SystemContext::Terminating = 0; 
__thread SchedulingEventKind SystemContext::Event = NumSchedulingEvent; 
__thread SchedulingEventKind SystemContext::DbgOriginalEvent = NumSchedulingEvent; 
__thread GreenThread< GreenScheduler<CPUSchedulePolicyAdapter> >* 
SystemContext::ReplacingThread = NULL;
} // End of danbi namespace 
