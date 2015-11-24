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

  SingleOrderingTicketServer.h -- single ticket server with ticket number ordering policy.
 */
#ifndef DANBI_SINGLE_ORDERING_TICKET_SERVER_H
#define DANBI_SINGLE_ORDERING_TICKET_SERVER_H
#include "Support/Machine.h"

namespace danbi {
class SingleOrderingTicketServer {
private:
  volatile int ServingTicket __cacheline_aligned; 

  void operator=(const SingleOrderingTicketServer&); // Do not implement
  SingleOrderingTicketServer(const SingleOrderingTicketServer&); // Do not implement
public:
  SingleOrderingTicketServer() 
    : ServingTicket(0) {}

  inline bool isMyTurn(int Ticket) {
    // ticket number ordering policy
    return Machine::casInt<volatile int>(&ServingTicket, Ticket, Ticket); 
  }

  inline void consume_safe(int Ticket) {
    // Consume this ticket and start to serve the next ticket. 
    assert( Ticket == ServingTicket ); 
    Machine::atomicIntInc<volatile int>(&ServingTicket); 
  }

  inline void consume(int Ticket) {
    // Wait until predecessor tickets are consumed. 
    while ( !isMyTurn(Ticket)) ; 
    // Consume this ticket
    consume_safe(Ticket); 
  }
};

}
#endif 
