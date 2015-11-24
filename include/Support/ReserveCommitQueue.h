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

  ReserveCommitQueue.h -- placeholder of concret reserve/commit queue
 */
#ifndef DANBI_RESERVE_COMMIT_QUEUE_H
#define DANBI_RESERVE_COMMIT_QUEUE_H
#ifdef DANBI_USE_BLOCKING_RESERVE_COMMIT_QUEUE
#include "Support/BlockingReserveCommitQueue.h"
#else
#include "Support/ConcurrentReserveCommitQueue.h"
#endif
#endif
