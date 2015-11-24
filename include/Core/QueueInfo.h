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

  QueueInfo.h -- Queue with producer and consumer kernel indexes
 */
#ifndef DANBI_QUEUE_INFO_H
#define DANBI_QUEUE_INFO_H
#include "Support/Connection.h"
#include "Support/Indexable.h"

namespace danbi {
class AbstractReserveCommitQueue; 

struct QueueInfo : public Connection, public Indexable {
  AbstractReserveCommitQueue* Queue; 
  bool isFeedbackQueue; 
  volatile bool isDeactivated; 

  /// constructor
  QueueInfo(AbstractReserveCommitQueue* Queue_, 
            int Producer_, 
            int Consumer_, 
            bool isFeedbackQueue_); 
  
  /// destructor
  virtual ~QueueInfo(); 
};

} // End of danbi namespace
#endif 
