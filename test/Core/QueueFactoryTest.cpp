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

  QueueFactoryTest.cpp  -- unit test case for QueueFactory class
 */
#include <cstdlib>
#include "gtest/gtest.h"
#include "Core/QueueFactory.h"


using namespace danbi; 
namespace {

TEST(QueueFactoryTest, Creation) {
  AbstractReserveCommitQueue* Q; 

  // false, false, false, false, false, false
  Q = QueueFactory::newQueue(sizeof(int), 10, 
                             false, false, false, false, false, false); 
  EXPECT_TRUE(Q != NULL); 
  delete Q; 

  // true, false, false, false, false, false
  Q = QueueFactory::newQueue(sizeof(int), 10, 
                             true, false, false, false, false, false); 
  EXPECT_TRUE(Q != NULL); 
  delete Q; 

  // false, true, false, false, false, false
  Q = QueueFactory::newQueue(sizeof(int), 10, 
                             false, true, false, false, false, false); 
  EXPECT_TRUE(Q != NULL); 
  delete Q; 
}

} // end of anonymous namespace

