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

  QueueFactory.cpp -- implementation of object factory 
  for AbstractReserveCommitQueue class
 */
#include <cassert>
#include "Support/ReserveCommitQueue.h"
#include "Support/FCFSTicketIssuer.h"
#include "Support/FCFSTicketServer.h"
#include "Support/OrderingTicketIssuer.h"
#include "Support/OrderingTicketServer.h"
#include "Support/DefaultBufferManager.h"
#include "Support/ResizableReserveCommitQueue.h"
#include "Support/ResizableBufferManager.h"
#include "Core/QueueFactory.h"
using namespace danbi; 

#define checkOrderingAndBuffering(PuTI, PuTS, PoTI, PoTS, FG, TG) (     \
    ((PuTI) == PushTicketIssuer) &&                                     \
    ((PuTS) == PushTicketServer) &&                                     \
    ((PoTI) == PopTicketIssuer) &&                                      \
    ((PoTS) == PopTicketServer) &&                                      \
    ((FG) == PopFromGPU) &&                                             \
    ((TG) == PushToGPU)                                                 \
)

AbstractReserveCommitQueue* QueueFactory::newQueue(int ElmSize, 
                                                   int NumElm, 
                                                   bool PushTicketIssuer, 
                                                   bool PushTicketServer, 
                                                   bool PopTicketIssuer, 
                                                   bool PopTicketServer, 
                                                   bool PopFromGPU, 
                                                   bool PushToGPU) {
#ifdef DANBI_DO_NOT_TICKET
  PushTicketIssuer = false; 
  PushTicketServer = false; 
  PopTicketIssuer = false; 
  PopTicketServer = false; 
#endif 

#ifndef DANBI_USE_RESIZABLE_QUEUE
  // Fixed size reserve commit queue 
  if (NumElm > 0) {
    // 0000
    if (checkOrderingAndBuffering(false, false, false, false, false, false))
      return new ReserveCommitQueue<FCFSTicketIssuer, 
                                    FCFSTicketServer,
                                    FCFSTicketIssuer,
                                    FCFSTicketServer,
                                    DefaultBufferManager>(ElmSize, NumElm);

    // 0001
    if (checkOrderingAndBuffering(false, false, false, true, false, false))
      return new ReserveCommitQueue<FCFSTicketIssuer, 
                                    FCFSTicketServer,
                                    FCFSTicketIssuer,
                                    OrderingTicketServer,
                                    DefaultBufferManager>(ElmSize, NumElm);

    // 0010
    if (checkOrderingAndBuffering(false, false, true, false, false, false))
      return new ReserveCommitQueue<FCFSTicketIssuer, 
                                    FCFSTicketServer,
                                    OrderingTicketIssuer,
                                    FCFSTicketServer,
                                    DefaultBufferManager>(ElmSize, NumElm);

    // 0100
    if (checkOrderingAndBuffering(false, true, false, false, false, false))
      return new ReserveCommitQueue<FCFSTicketIssuer, 
                                    OrderingTicketServer,
                                    FCFSTicketIssuer,
                                    FCFSTicketServer,
                                    DefaultBufferManager>(ElmSize, NumElm);

    // 0101
    if (checkOrderingAndBuffering(false, true, false, true, false, false))
      return new ReserveCommitQueue<FCFSTicketIssuer, 
                                    OrderingTicketServer,
                                    FCFSTicketIssuer,
                                    OrderingTicketServer,
                                    DefaultBufferManager>(ElmSize, NumElm);

    // 0110 
    if (checkOrderingAndBuffering(false, true, true, false, false, false))
      return new ReserveCommitQueue<FCFSTicketIssuer, 
                                    OrderingTicketServer,
                                    OrderingTicketIssuer,
                                    FCFSTicketServer,
                                    DefaultBufferManager>(ElmSize, NumElm);

    // 1000
    if (checkOrderingAndBuffering(true, false, false, false, false, false))
      return new ReserveCommitQueue<FCFSTicketIssuer, 
                                    FCFSTicketServer,
                                    FCFSTicketIssuer,
                                    FCFSTicketServer,
                                    DefaultBufferManager>(ElmSize, NumElm);

    // 1001 
    if (checkOrderingAndBuffering(true, false, false, true, false, false))
      return new ReserveCommitQueue<OrderingTicketIssuer, 
                                    FCFSTicketServer,
                                    FCFSTicketIssuer,
                                    OrderingTicketServer,
                                    DefaultBufferManager>(ElmSize, NumElm);

    // 1010
    if (checkOrderingAndBuffering(true, false, false, true, false, false))
      return new ReserveCommitQueue<OrderingTicketIssuer, 
                                    FCFSTicketServer,
                                    FCFSTicketIssuer,
                                    OrderingTicketServer,
                                    DefaultBufferManager>(ElmSize, NumElm);
    assert(0 && "Unsupported queue configuration"); 
  }
  // Resizable reserve commit queue
  else 
#else // end of DANBI_USE_STATIC_QUEUE_SIZE
#undef DANBI_DEBUG_DONT_CHANGE_QUEUE_SIZE // Never turn on in production mode
  {
    // 0000
    if (checkOrderingAndBuffering(false, false, false, false, false, false))
      return new ResizableReserveCommitQueue<FCFSTicketIssuer, 
                                    FCFSTicketServer,
                                    FCFSTicketIssuer,
                                    FCFSTicketServer,
                                    ResizableBufferManager>(ElmSize
#ifdef DANBI_DEBUG_DONT_CHANGE_QUEUE_SIZE
                                                            , 
                                                            NumElm + 1, 
                                                            NumElm + 1
#endif 
                                      );

    // 0001
    if (checkOrderingAndBuffering(false, false, false, true, false, false))
      return new ResizableReserveCommitQueue<FCFSTicketIssuer, 
                                    FCFSTicketServer,
                                    FCFSTicketIssuer,
                                    OrderingTicketServer,
                                    ResizableBufferManager>(ElmSize
#ifdef DANBI_DEBUG_DONT_CHANGE_QUEUE_SIZE
                                                            , 
                                                            NumElm + 1, 
                                                            NumElm + 1
#endif 
                                      );

    // 0010
    if (checkOrderingAndBuffering(false, false, true, false, false, false))
      return new ResizableReserveCommitQueue<FCFSTicketIssuer, 
                                    FCFSTicketServer,
                                    OrderingTicketIssuer,
                                    FCFSTicketServer,
                                    ResizableBufferManager>(ElmSize
#ifdef DANBI_DEBUG_DONT_CHANGE_QUEUE_SIZE
                                                            , 
                                                            NumElm + 1, 
                                                            NumElm + 1
#endif 
                                      );

    // 0100
    if (checkOrderingAndBuffering(false, true, false, false, false, false))
      return new ResizableReserveCommitQueue<FCFSTicketIssuer, 
                                    OrderingTicketServer,
                                    FCFSTicketIssuer,
                                    FCFSTicketServer,
                                    ResizableBufferManager>(ElmSize
#ifdef DANBI_DEBUG_DONT_CHANGE_QUEUE_SIZE
                                                            , 
                                                            NumElm + 1, 
                                                            NumElm + 1
#endif 
                                      );

    // 0101
    if (checkOrderingAndBuffering(false, true, false, true, false, false))
      return new ResizableReserveCommitQueue<FCFSTicketIssuer, 
                                    OrderingTicketServer,
                                    FCFSTicketIssuer,
                                    OrderingTicketServer,
                                    ResizableBufferManager>(ElmSize
#ifdef DANBI_DEBUG_DONT_CHANGE_QUEUE_SIZE
                                                            , 
                                                            NumElm + 1, 
                                                            NumElm + 1
#endif 
                                      );

    // 0110 
    if (checkOrderingAndBuffering(false, true, true, false, false, false))
      return new ResizableReserveCommitQueue<FCFSTicketIssuer, 
                                    OrderingTicketServer,
                                    OrderingTicketIssuer,
                                    FCFSTicketServer,
                                    ResizableBufferManager>(ElmSize
#ifdef DANBI_DEBUG_DONT_CHANGE_QUEUE_SIZE
                                                            , 
                                                            NumElm + 1, 
                                                            NumElm + 1
#endif 
                                      );

    // 1000
    if (checkOrderingAndBuffering(true, false, false, false, false, false))
      return new ResizableReserveCommitQueue<FCFSTicketIssuer, 
                                    FCFSTicketServer,
                                    FCFSTicketIssuer,
                                    FCFSTicketServer,
                                    ResizableBufferManager>(ElmSize
#ifdef DANBI_DEBUG_DONT_CHANGE_QUEUE_SIZE
                                                            , 
                                                            NumElm + 1, 
                                                            NumElm + 1
#endif 
                                      );

    // 1001 
    if (checkOrderingAndBuffering(true, false, false, true, false, false))
      return new ResizableReserveCommitQueue<OrderingTicketIssuer, 
                                    FCFSTicketServer,
                                    FCFSTicketIssuer,
                                    OrderingTicketServer,
                                    ResizableBufferManager>(ElmSize
#ifdef DANBI_DEBUG_DONT_CHANGE_QUEUE_SIZE
                                                            , 
                                                            NumElm + 1, 
                                                            NumElm + 1
#endif 
                                      );

    // 1010
    if (checkOrderingAndBuffering(true, false, false, true, false, false))
      return new ResizableReserveCommitQueue<OrderingTicketIssuer, 
                                    FCFSTicketServer,
                                    FCFSTicketIssuer,
                                    OrderingTicketServer,
                                    ResizableBufferManager>(ElmSize
#ifdef DANBI_DEBUG_DONT_CHANGE_QUEUE_SIZE
                                                            , 
                                                            NumElm + 1, 
                                                            NumElm + 1
#endif 
                                      );
    assert(0 && "Unsupported queue configuration"); 
  }
#endif
  return NULL; 
}

 
