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

  ReserveCommitQueueTest.cpp  -- unit test case for ReserveCommitQueue class
 */
#include <math.h>
#include "gtest/gtest.h"
#include "Support/ReserveCommitQueue.h"
#include "Support/DefaultBufferManager.h"
#include "Support/FCFSTicketIssuer.h"
#include "Support/OrderingTicketIssuer.h"
#include "Support/FCFSTicketServer.h"
#include "Support/OrderingTicketServer.h"

using namespace danbi; 
namespace {

void BasicTest() {
  AbstractReserveCommitQueue* Q; 
  QueueWindow Win[2]; 
  ReservationInfo RevInfo; 
  float FillRatio; 

  Q = new ReserveCommitQueue<FCFSTicketIssuer,
                             FCFSTicketServer,
                             FCFSTicketIssuer,
                             FCFSTicketServer,
                             DefaultBufferManager>(sizeof(int), 10);
  EXPECT_TRUE(Q != NULL);

  for (int i=0; i<10; i+=2) {
    EXPECT_EQ(0, Q->reservePushN(2, Win, RevInfo, 0, 0));
    EXPECT_TRUE(Win[0].Base != NULL);
    EXPECT_EQ(2, Win[0].Num); 
    EXPECT_TRUE(Win[1].Base == NULL);
    EXPECT_EQ(0, Win[1].Num);
    
    static_cast<int *>(Win[0].Base)[0] = i; 
    static_cast<int *>(Win[0].Base)[1] = i+1;
    EXPECT_EQ(0, Q->commitPush(RevInfo, &FillRatio));
    EXPECT_TRUE( fabs((0.2f * (i/2+1)) - FillRatio) < 0.00001 ); 
  }
  EXPECT_EQ(-EWOULDBLOCK, Q->reservePushN(2, Win, RevInfo, 0, 0));

  for (int i=0; i<6; i+=2) {
    EXPECT_EQ(0, Q->reservePopN(2, Win, RevInfo, 0, 0));
    EXPECT_TRUE(Win[0].Base != NULL);
    EXPECT_EQ(2, Win[0].Num); 
    EXPECT_TRUE(Win[1].Base == NULL);
    EXPECT_EQ(0, Win[1].Num);
    
    EXPECT_EQ(i, ((int *)Win[0].Base)[0]); 
    EXPECT_EQ(i+1, ((int *)Win[0].Base)[1]);
    EXPECT_EQ(0, Q->commitPop(RevInfo, &FillRatio));
    EXPECT_TRUE( fabs((1.0f - (0.2f * (i/2+1))) - FillRatio) < 0.00001 ); 
  }

  for (int i=0; i<6; i+=2) {
    EXPECT_EQ(0, Q->reservePushN(2, Win, RevInfo, 0, 0));
    EXPECT_EQ(2, Win[0].Num + Win[1].Num);
    
    static_cast<int *>(Win[0].Base)[0] = i; 
    static_cast<int *>(Win[0].Base)[1] = i+1;
    EXPECT_EQ(0, Q->commitPush(RevInfo, &FillRatio));
    EXPECT_TRUE( fabs((0.2f * (i/2+3)) - FillRatio) < 0.00001 ); 
  }
  EXPECT_EQ(-EWOULDBLOCK, Q->reservePushN(2, Win, RevInfo, 0, 0));

  for (int i=0; i<10; i+=2) {
    EXPECT_EQ(0, Q->reservePopN(2, Win, RevInfo, 0, 0));
    EXPECT_EQ(2, Win[0].Num + Win[1].Num);

    EXPECT_EQ(0, Q->commitPop(RevInfo, &FillRatio));
    EXPECT_TRUE( fabs((1.0f - (0.2f * (i/2+1))) - FillRatio) < 0.00001 ); 
  }
  EXPECT_EQ(-EWOULDBLOCK, Q->reservePopN(2, Win, RevInfo, 0, 0));
  EXPECT_TRUE(Q->empty());

  delete Q; 
}

void TicketOrderingTest() {
  AbstractReserveCommitQueue* Q1; 
  AbstractReserveCommitQueue* Q2; 
  QueueWindow Win[2]; 
  ReservationInfo RevInfo; 
  int Ticket; 

  Q1 = new ReserveCommitQueue<FCFSTicketIssuer,
                              FCFSTicketServer,
                              OrderingTicketIssuer,
                              FCFSTicketServer,
                              DefaultBufferManager>(sizeof(int), 10);
  EXPECT_TRUE(Q1 != NULL);

  Q2 = new ReserveCommitQueue<FCFSTicketIssuer,
                              OrderingTicketServer,
                              FCFSTicketIssuer,
                              FCFSTicketServer,
                              DefaultBufferManager>(sizeof(int), 10);
  EXPECT_TRUE(Q2 != NULL);

  // Fill Q1 for testing
  for (int i=0; i<10; ++i) {
    EXPECT_EQ(0, Q1->reservePushN(1, Win, RevInfo, 0, 0));
    static_cast<int*>(Win[0].Base)[0] = i; 
    EXPECT_EQ(0, Q1->commitPush(RevInfo));
  }
  EXPECT_EQ(-EWOULDBLOCK, Q1->reservePushN(1, Win, RevInfo, 0, 0));

  // Move data from Q1 to Q2
  for (int i=0; i<10; ++i) {
    EXPECT_EQ(0, Q1->reservePopN(1, Win, RevInfo, 0, &Ticket));
    int d = static_cast<int*>(Win[0].Base)[0];
    EXPECT_EQ(i, d); 
    EXPECT_EQ(0, Q1->commitPop(RevInfo));

    EXPECT_EQ(0, Q2->reservePushN(1, Win, RevInfo, Ticket, 0));
    static_cast<int*>(Win[0].Base)[0] = d; 
    EXPECT_EQ(0, Q2->commitPush(RevInfo));
  }
  EXPECT_TRUE(Q1->empty());

  delete Q1; 
  delete Q2; 
}

void MultiplePopTest() {
  AbstractReserveCommitQueue* Q1; 
  QueueWindow Win[2]; 
  ReservationInfo RevInfo; 

  Q1 = new ReserveCommitQueue<FCFSTicketIssuer,
                              FCFSTicketServer,
                              FCFSTicketIssuer,
                              FCFSTicketServer,
                              DefaultBufferManager>(sizeof(int), 100);
  EXPECT_TRUE(Q1 != NULL);

  // Fill Q1 for testing
  for (int i=0; i<64; ++i) {
    EXPECT_EQ(0, Q1->reservePushN(1, Win, RevInfo, 0, 0));
    EXPECT_EQ(0, Q1->commitPush(RevInfo));
  }

  // Pop
  for (int i=0; i<4; ++i) {
    EXPECT_EQ(0, Q1->reservePopN(16, Win, RevInfo, 0, 0));
    EXPECT_EQ(0, Q1->commitPop(RevInfo));
  }
  EXPECT_TRUE(Q1->empty());
  EXPECT_EQ(-EWOULDBLOCK, Q1->reservePopN(16, Win, RevInfo, 0, 0));

  delete Q1; 
}

TEST(ReserveCommitQueueTest, BasicFCFS) {
  BasicTest();

}

TEST(ReserveCommitQueueTest, TicketOrdering) {
  TicketOrderingTest();
}

TEST(ReserveCommitQueueTest, MultiplePop) {
  MultiplePopTest();
}
} // end of anonymous namespace

