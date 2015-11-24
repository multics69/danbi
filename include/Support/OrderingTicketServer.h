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

  OrderingTicketServer.h -- placeholder of ordering ticket server 
 */
#ifndef DANBI_ORDERING_TICKET_SERVER_H
#define DANBI_ORDERING_TICKET_SERVER_H
#include "Support/BlockingOrderingTicketServer.h"
#include "Support/SingleOrderingTicketServer.h"
#include "Support/PerCPUOrderingTicketServer.h"

namespace danbi {
#ifdef DANBI_USE_BLOCKING_RESERVE_COMMIT_QUEUE
typedef BlockingOrderingTicketServer OrderingTicketServer;
#else 
#ifdef DANBI_DISABLE_PER_CPU_ORDERING_TICKET_SERVER
typedef SingleOrderingTicketServer OrderingTicketServer;
#else
typedef PerCPUOrderingTicketServer OrderingTicketServer;
#endif
#endif 
}
#endif 
