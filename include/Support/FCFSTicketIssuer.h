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

  FCFSTicketIssuer.h -- ticket issuer with first-come-first-serve policy. 
  It does not take care ticket number. 
 */
#ifndef DANBI_FCFS_TICKET_ISSUER_H
#define DANBI_FCFS_TICKET_ISSUER_H
namespace danbi {

class FCFSTicketIssuer {
private:
  void operator=(const FCFSTicketIssuer&); // Do not implement
  FCFSTicketIssuer(const FCFSTicketIssuer&); // Do not implement
public:
  FCFSTicketIssuer() { 
    // do nothing
  }

  virtual ~FCFSTicketIssuer() {
    // do nothing
  }

  inline void issue(int* Ticket) {
    // do nothing
  }

  inline void issue(int* Ticket, int OldHead, int NewHead) {
    // do nothing
  }
};

}
#endif 
