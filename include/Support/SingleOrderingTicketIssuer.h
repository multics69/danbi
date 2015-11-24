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

  SingleOrderingTicketIssuer.h -- ticket issuer with ticket number ordering policy.
 */
#ifndef DANBI_SINGLE_ORDERING_TICKET_ISSUER_H
#define DANBI_SINGLE_ORDERING_TICKET_ISSUER_H
#include "Support/Machine.h"

namespace danbi {
class SingleOrderingTicketIssuer {
private:
  volatile int IssuedTicket __cacheline_aligned; 
  volatile int LastIndex; 

  void operator=(const SingleOrderingTicketIssuer&); // Do not implement
  SingleOrderingTicketIssuer(const SingleOrderingTicketIssuer&); // Do not implement
public:
  SingleOrderingTicketIssuer() 
    : IssuedTicket(0), LastIndex(0) {}

  inline void issue(int* Ticket, int OldIndex, int NewIndex) {
    // Wait until previous pop() finish to issue a ticket
    while ( !Machine::casInt<volatile int>(&LastIndex, OldIndex, OldIndex) ) ; 

    // Issue a new ticket and update last head
    *Ticket = Machine::atomicIntInc<volatile int>(&IssuedTicket) - 1; 
    LastIndex = NewIndex; 
  }
};

}
#endif 
