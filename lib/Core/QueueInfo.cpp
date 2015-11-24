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

  QueueInfo.cpp -- implementation of QueueInfo 
 */
#include "Core/QueueInfo.h"
#include "Support/AbstractReserveCommitQueue.h"
using namespace danbi; 

QueueInfo::QueueInfo(AbstractReserveCommitQueue* Queue_, 
                     int Producer_, 
                     int Consumer_, 
                     bool isFeedbackQueue_)
  : Connection(Producer_, Consumer_), Queue(Queue_), 
    isFeedbackQueue(isFeedbackQueue_), isDeactivated(false) {
    // do nothing
}

QueueInfo::~QueueInfo() {
  delete Queue; 
}



