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

  SchedulingEvent.h -- enumeration of scheduling event 
 */
#ifndef DANBI_SCHEDULING_EVENT_H
#define DANBI_SCHEDULING_EVENT_H
namespace danbi {

enum SchedulingEventKind {
  InputQueueIsEmpty, 
  OutputQueueIsFull,

  NotMyTurn, 
  KernelTerminated, 
  RandomJump, 
  ProbabilisticHint, 
#ifdef DANBI_ENABLE_EAGER_THREAD_DESTROY
  ThreadDiscarded, 
#endif

  // Number of scheduling event
  NumSchedulingEvent, 

  // Number of queue scheduling event 
  NumQueueSchedulingEvent = 2, 
};

} // End of danbi namespace
#endif 
