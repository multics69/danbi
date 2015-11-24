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

  PerCPUOrderingTicketIssuer.h -- ticket issuer with ticket number ordering policy.
 */
#ifndef DANBI_PER_CPU_ORDERING_TICKET_ISSUER_H
#define DANBI_PER_CPU_ORDERING_TICKET_ISSUER_H
#include "Support/Machine.h"
#include "Support/MachineConfig.h"
#include "Support/PerCPUData.h"

namespace danbi {
class PerCPUOrderingTicketIssuer {
private:
  volatile int IssuedTicket __cacheline_aligned; 
  PerCPUData<volatile int> LastIndexArray; 
  int Cores;

  void operator=(const PerCPUOrderingTicketIssuer&); // Do not implement
  PerCPUOrderingTicketIssuer(const PerCPUOrderingTicketIssuer&); // Do not implement
public:
  PerCPUOrderingTicketIssuer() 
    : IssuedTicket(0) {
    Cores = MachineConfig::getInstance().getNumHardwareThreadsInCPUs(); 
    LastIndexArray.initialize(); 
    LastIndexArray[0] = 0; // Ready to go 
    for (int i = 1; i < Cores; ++i)
      LastIndexArray[i] = -1; // Hold on
    Machine::wmb(); 
  }

  inline void issue(int* Ticket, int OldIndex, int NewIndex) {
    // Calculate the slots for old and new index
    int OldSlot = OldIndex % Cores; 
    int NewSlot = NewIndex % Cores; 

    // Wait until previous pop() finish to issue a ticket
    while ( !Machine::casInt<volatile int>(&LastIndexArray[OldSlot], OldIndex, OldIndex) ) ; 

    // Issue a new ticket and update last head
    *Ticket = Machine::atomicIntInc<volatile int>(&IssuedTicket) - 1; 
    LastIndexArray[NewSlot] = NewIndex; 
  }
};

}
#endif 
