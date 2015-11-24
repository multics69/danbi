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

  ThreadTest.cpp -- unit test cases for Machine class
 */
#include <unistd.h>
#include <cerrno>
#include "gtest/gtest.h"
#include "Support/AbstractRunnable.h"
#include "Support/Thread.h"

using namespace danbi; 

namespace {
static volatile int HelloWorld = 0; 

class TestThread: public Thread, private AbstractRunnable {
private:
  void main() {
    ::usleep(200000); 
    ++HelloWorld;
  }

public:
  TestThread() 
    : Thread(static_cast<AbstractRunnable*>(this)[0], 16384) {}
};

TEST(ThreadTest, Basic) {
  TestThread* Worker; 

  Worker = new TestThread(); 
  EXPECT_TRUE(Worker != NULL); 
  EXPECT_EQ(0, Worker->setAffinity(1, 0)); 
  EXPECT_EQ(-EINVAL, Worker->setAffinity(1, 1000000)); 
  EXPECT_EQ(0, Worker->start()); 
  EXPECT_EQ(0, Worker->join()); 
  EXPECT_EQ(1, HelloWorld); 

  delete Worker; 
}

} // end of anonymous namespace

