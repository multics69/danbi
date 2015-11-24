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

  MachineTest.cpp -- unit test cases for Machine class
 */
#include <vector>
#include <algorithm>
#include <iterator>
#include "gtest/gtest.h"
#include "Support/Connection.h"
#include "Support/ConnectivityMatrix.h"

using namespace danbi; 

namespace {

TEST(ConnectivityMatrixTest, Basic1) {
  std::vector<Connection*> ConnectionTable; 
  std::vector<Connection*> FeedbackConnections; 
  Connection* C; 

  // build connection table 
  C = new Connection(1, 2); 
  EXPECT_TRUE(C != NULL); 
  ConnectionTable.push_back(C); 

  C = new Connection(2, 3); 
  EXPECT_TRUE(C != NULL); 
  ConnectionTable.push_back(C); 
  
  C = new Connection(2, 4); 
  EXPECT_TRUE(C != NULL); 
  ConnectionTable.push_back(C); 
  
  C = new Connection(3, 5); 
  EXPECT_TRUE(C != NULL); 
  ConnectionTable.push_back(C); 
  
  C = new Connection(4, 5); 
  EXPECT_TRUE(C != NULL); 
  ConnectionTable.push_back(C); 
  
  C = new Connection(4, 1); 
  EXPECT_TRUE(C != NULL); 
  ConnectionTable.push_back(C); 
  FeedbackConnections.push_back(C); 

  C = new Connection(3, 4); 
  EXPECT_TRUE(C != NULL); 
  ConnectionTable.push_back(C); 
  
  C = new Connection(4, 3); 
  EXPECT_TRUE(C != NULL); 
  ConnectionTable.push_back(C); 

  // build up start node table
  std::vector<int> StartNodes; 
  StartNodes.push_back(1); 
  
  // connection matrix
  ConnectivityMatrix<Connection*>* CM = new ConnectivityMatrix<Connection*>(
    StartNodes, ConnectionTable, FeedbackConnections); 
  EXPECT_TRUE(CM != NULL); 
  CM->build(); 

  // test all nodes
  NodeIterator SA, EA; 
  CM->getAllNodes(SA, EA); 
  EXPECT_EQ(5, std::distance(SA, EA)); 

  // test start nodes
  CM->getStartNodes(SA, EA); 
  EXPECT_EQ(1, std::distance(SA, EA)); 

  // test forward link  
  ConnectivityIterator i, e; 
  EXPECT_TRUE( CM->forwardNext(1, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 2); 
  
  EXPECT_TRUE( CM->forwardNext(2, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 3 || i->second == 4); 

  EXPECT_TRUE( CM->forwardNext(3, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 5 || i->second == 4); 

  EXPECT_TRUE( CM->forwardNext(4, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 5 || i->second == 1 || i->second == 3); 

  // test backward link  
  EXPECT_TRUE( CM->backwardNext(5, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 3 || i->second == 4); 

  EXPECT_TRUE( CM->backwardNext(3, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 2 || i->second == 4); 

  EXPECT_TRUE( CM->backwardNext(4, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 2 || i->second == 3); 

  EXPECT_TRUE( CM->backwardNext(2, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 1); 
  
  // test forward link without feedback
  EXPECT_TRUE( CM->forwardNoFeedbackNext(1, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 2); 
  
  EXPECT_TRUE( CM->forwardNoFeedbackNext(2, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 3 || i->second == 4); 

  EXPECT_TRUE( CM->forwardNoFeedbackNext(3, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 5 || i->second == 4); 

  EXPECT_TRUE( CM->forwardNoFeedbackNext(4, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 5 || i->second == 3); 

  // test backward link  without feedback
  EXPECT_TRUE( CM->backwardNoFeedbackNext(5, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 3 || i->second == 4); 

  EXPECT_TRUE( CM->backwardNoFeedbackNext(3, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 2 || i->second == 4); 

  EXPECT_TRUE( CM->backwardNoFeedbackNext(4, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 2 || i->second == 3); 

  EXPECT_TRUE( CM->backwardNoFeedbackNext(2, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 1); 
  delete CM; 
}

TEST(ConnectivityMatrixTest, Basic2) {
  std::vector<Connection*> ConnectionTable; 
  std::vector<Connection*> FeedbackConnections; 
  Connection* C; 

  // build connection table 
  C = new Connection(0, 1); 
  EXPECT_TRUE(C != NULL); 
  ConnectionTable.push_back(C); 

  C = new Connection(1, 2); 
  EXPECT_TRUE(C != NULL); 
  ConnectionTable.push_back(C); 

  C = new Connection(2, 5); 
  EXPECT_TRUE(C != NULL); 
  ConnectionTable.push_back(C); 

  C = new Connection(0, 3); 
  EXPECT_TRUE(C != NULL); 
  ConnectionTable.push_back(C); 

  C = new Connection(3, 4); 
  EXPECT_TRUE(C != NULL); 
  ConnectionTable.push_back(C); 

  C = new Connection(4, 5); 
  EXPECT_TRUE(C != NULL); 
  ConnectionTable.push_back(C); 

  // build up start node table
  std::vector<int> StartNodes; 
  StartNodes.push_back(0); 
  
  // connection matrix
  ConnectivityMatrix<Connection*>* CM = new ConnectivityMatrix<Connection*>(
    StartNodes, ConnectionTable, FeedbackConnections); 
  EXPECT_TRUE(CM != NULL); 
  CM->build(); 

  // test all nodes
  NodeIterator SA, EA; 
  CM->getAllNodes(SA, EA); 
  EXPECT_EQ(6, std::distance(SA, EA)); 

  // test start nodes
  CM->getStartNodes(SA, EA); 
  EXPECT_EQ(1, std::distance(SA, EA)); 

  // test forward link  
  ConnectivityIterator i, e; 
  EXPECT_TRUE( CM->forwardNext(0, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 1 || i->second == 3); 
  
  EXPECT_TRUE( CM->forwardNext(1, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 2); 
  
  EXPECT_TRUE( CM->forwardNext(2, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 5); 
  
  EXPECT_TRUE( CM->forwardNext(3, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 4); 
  
  EXPECT_TRUE( CM->forwardNext(4, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 5); 
  
  // test forward link without feedback link 
  EXPECT_TRUE( CM->forwardNoFeedbackNext(0, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 1 || i->second == 3); 
  
  EXPECT_TRUE( CM->forwardNoFeedbackNext(1, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 2); 
  
  EXPECT_TRUE( CM->forwardNoFeedbackNext(2, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 5); 
  
  EXPECT_TRUE( CM->forwardNoFeedbackNext(3, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 4); 
  
  EXPECT_TRUE( CM->forwardNoFeedbackNext(4, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 5); 
  
  // test backward link  
  EXPECT_TRUE( CM->backwardNext(5, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 2 || i->second == 4); 
  
  EXPECT_TRUE( CM->backwardNext(2, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 1);

  EXPECT_TRUE( CM->backwardNext(1, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 0);

  EXPECT_TRUE( CM->backwardNext(4, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 3);

  EXPECT_TRUE( CM->backwardNext(3, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 0);

  // test backward link without feedback
  EXPECT_TRUE( CM->backwardNoFeedbackNext(5, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 2 || i->second == 4); 
  
  EXPECT_TRUE( CM->backwardNoFeedbackNext(2, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 1);

  EXPECT_TRUE( CM->backwardNoFeedbackNext(1, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 0);

  EXPECT_TRUE( CM->backwardNoFeedbackNext(4, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 3);

  EXPECT_TRUE( CM->backwardNoFeedbackNext(3, i, e) ); 
  for(; i != e; ++i)
    EXPECT_TRUE(i->second == 0);
  delete CM; 
}

} // end of anonymous namespace

