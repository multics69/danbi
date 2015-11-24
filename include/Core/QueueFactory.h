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

  QueueFactory.h -- header of object factory for AbstractReserveCommitQueue class
 */
#ifndef DANBI_QUEUE_FACTORY_H
#define DANBI_QUEUE_FACTORY_H
#include "Support/AbstractReserveCommitQueue.h"

namespace danbi {
/// Object factory for AbstractReserveCommitQueue class
class QueueFactory {
public:
  static AbstractReserveCommitQueue* newQueue(int ElmSize, 
                                              int NumElm, 
                                              bool PushTicketIssuer, 
                                              bool PushTicketServer, 
                                              bool PopTicketIssuer, 
                                              bool PopTicketServer, 
                                              bool PopFromGPU, 
                                              bool PushToGPU); 
};

} // End of danbi namespace

#endif 
