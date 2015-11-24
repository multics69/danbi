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

  OrderingTicketIssuer.h -- placeholder of ordering ticket issuer
 */
#ifndef DANBI_ORDERING_TICKET_ISSUER_H
#define DANBI_ORDERING_TICKET_ISSUER_H
#include "Support/BlockingOrderingTicketIssuer.h"
#include "Support/SingleOrderingTicketIssuer.h"
#include "Support/PerCPUOrderingTicketIssuer.h"

namespace danbi {
#ifdef DANBI_USE_BLOCKING_RESERVE_COMMIT_QUEUE
typedef BlockingOrderingTicketIssuer OrderingTicketIssuer;
#else
typedef SingleOrderingTicketIssuer OrderingTicketIssuer; 
#endif 

typedef PerCPUOrderingTicketIssuer OrderingTicketIssuer_;  // DO NOT USE: it degrades performance. 
}
#endif 
