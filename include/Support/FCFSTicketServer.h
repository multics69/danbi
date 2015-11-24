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

  FCFSTicketServer.h -- ticket server with first-come-first-serve policy. 
  It does not take care ticket number. 
 */
#ifndef DANBI_FCFS_TICKET_SERVER_H
#define DANBI_FCFS_TICKET_SERVER_H
namespace danbi {

class FCFSTicketServer {
private:
  void operator=(const FCFSTicketServer&); // Do not implement
  FCFSTicketServer(const FCFSTicketServer&); // Do not implement
public:
  FCFSTicketServer() { 
    // do nothing
  }

  virtual ~FCFSTicketServer() {
    // do nothing
  }

  inline bool isMyTurn(int Ticket) {
    // first-come-first-serve policy
    return true; 
  }

  inline void consume(int Ticket) {
    // do nothing
  }

  inline void consume_safe(int Ticket) {
    // do nothing
  }
};

}
#endif 
