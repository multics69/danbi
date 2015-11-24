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

  PerCPUOrderingTicketServer.h -- per-CPU ticket server with ticket number ordering policy.
 */
#ifndef DANBI_PER_CPU_ORDERING_TICKET_SERVER_H
#define DANBI_PER_CPU_ORDERING_TICKET_SERVER_H
#include "Support/Machine.h"
#include "Support/MachineConfig.h"
#include "Support/PerCPUData.h"

namespace danbi {
class PerCPUOrderingTicketServer {
private:
  PerCPUData<volatile int> ServingTicketArray; 
  int Cores; 

  void operator=(const PerCPUOrderingTicketServer&); // Do not implement
  PerCPUOrderingTicketServer(const PerCPUOrderingTicketServer&); // Do not implement
public:
  PerCPUOrderingTicketServer() {
    Cores = MachineConfig::getInstance().getNumHardwareThreadsInCPUs(); 
    ServingTicketArray.initialize(); 
    ServingTicketArray[0] = 0; // Ready to go 
    for (int i = 1; i < Cores; ++i)
      ServingTicketArray[i] = i - Cores; // Hold on
    Machine::wmb(); 
  }

  inline bool isMyTurn(int Ticket) {
    // Calculate the slot for serving ticket
    int Slot = Ticket % Cores; 

    // ticket number ordering policy
    return Machine::casInt<volatile int>(&ServingTicketArray[Slot], Ticket, Ticket); 
  }

  inline void consume_safe(int Ticket) {
    // Consume this ticket and start to serve the next ticket. 
    int NewSlot = (Ticket + 1) % Cores; 
    Machine::atomicIntAddReturn(&ServingTicketArray[NewSlot], Cores); 
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
