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

  BlockingOrderingTicketIssuer.h -- ticket issuer with ticket number ordering policy.
 */
#ifndef DANBI_BLOCKING_ORDERING_TICKET_ISSUER_H
#define DANBI_BLOCKING_ORDERING_TICKET_ISSUER_H

namespace danbi {
class BlockingOrderingTicketIssuer {
private:
  volatile int IssuedTicket __cacheline_aligned; 

  void operator=(const BlockingOrderingTicketIssuer&); // Do not implement
  BlockingOrderingTicketIssuer(const BlockingOrderingTicketIssuer&); // Do not implement
public:
  BlockingOrderingTicketIssuer() 
    : IssuedTicket(0) {}

  inline void issue(int* Ticket, int OldIndex, int NewIndex) {
    int iticket = IssuedTicket; 
    *Ticket = iticket;
    IssuedTicket = iticket + 1;
  }
};

}
#endif 
