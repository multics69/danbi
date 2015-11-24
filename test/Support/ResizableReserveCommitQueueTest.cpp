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

  ResizableReserveCommitQueueTest.cpp  -- unit test case for ResizableReserveCommitQueue class
 */
#include <math.h>
#include "gtest/gtest.h"
#include "Support/ResizableReserveCommitQueue.h"
#include "Support/ResizableBufferManager.h"
#include "Support/FCFSTicketIssuer.h"
#include "Support/FCFSTicketServer.h"

using namespace danbi; 
namespace {

void BasicTest() {
  AbstractReserveCommitQueue* Q; 
  QueueWindow Win[2]; 
  ReservationInfo RevInfo; 
  float FillRatio; 

  Q = new ResizableReserveCommitQueue<FCFSTicketIssuer,
                             FCFSTicketServer,
                             FCFSTicketIssuer,
                             FCFSTicketServer,
                                      ResizableBufferManager>(4096, 10, 100, 0.1f, 0.5f, 0.3f);
  EXPECT_TRUE(Q != NULL);
  EXPECT_EQ(10, Q->getMaxNumElement()); 

  // push 5: 5
  EXPECT_EQ(0, Q->reservePushN(5, Win, RevInfo, 0, 0));
  EXPECT_EQ(10, Q->getMaxNumElement()); 
  EXPECT_EQ(0, Q->commitPush(RevInfo, NULL));
  EXPECT_EQ(10, Q->getMaxNumElement()); 

  // push 45: 55
  EXPECT_EQ(0, Q->reservePushN(45, Win, RevInfo, 0, 0));
  EXPECT_EQ(55, Q->getMaxNumElement()); 
  EXPECT_EQ(0, Q->commitPush(RevInfo, NULL));
  EXPECT_EQ(55, Q->getMaxNumElement()); 

  // pop 30: 50
  EXPECT_EQ(0, Q->reservePopN(30, Win, RevInfo, 0, 0));
  EXPECT_EQ(50, Q->getMaxNumElement()); 
  EXPECT_EQ(0, Q->commitPop(RevInfo, NULL));
  EXPECT_EQ(50, Q->getMaxNumElement()); 

  // push 20: 50 
  EXPECT_EQ(0, Q->reservePushN(20, Win, RevInfo, 0, 0));
  EXPECT_EQ(50, Q->getMaxNumElement()); 
  EXPECT_EQ(0, Q->commitPush(RevInfo, NULL));
  EXPECT_EQ(50, Q->getMaxNumElement()); 

  // pop 25: 50
  EXPECT_EQ(0, Q->reservePopN(25, Win, RevInfo, 0, 0));
  EXPECT_EQ(50, Q->getMaxNumElement()); 
  EXPECT_EQ(0, Q->commitPop(RevInfo, NULL));
  EXPECT_EQ(50, Q->getMaxNumElement()); 

  // pop 5: 35
  EXPECT_EQ(0, Q->reservePopN(5, Win, RevInfo, 0, 0));
  EXPECT_EQ(35, Q->getMaxNumElement()); 
  EXPECT_EQ(0, Q->commitPop(RevInfo, NULL));
  EXPECT_EQ(35, Q->getMaxNumElement()); 

  delete Q; 
}

TEST(ResizableReserveCommitQueueTest, Basic) {
  BasicTest();
}
} // end of anonymous namespace

