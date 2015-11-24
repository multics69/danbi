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
#include "gtest/gtest.h"
#include "Support/Machine.h"

using namespace danbi; 

namespace {
struct LongLong {
  long a, b; 
  bool operator==(const LongLong& rhs) const {
    return a == rhs.a && b == rhs.b; 
  }
};


TEST(MachineTest, Atomic) {
  int i = 0; 
  long l = 0;
  LongLong ll = {0, 0}, ll2;
  LongLong llZero = {0, 0}, llOne = {1, 1}, llTwo = {2, 2}; 

  EXPECT_EQ(10, Machine::atomicIntAddReturn<int>(&i, 10));
  EXPECT_EQ(10, i); 
  EXPECT_EQ(11, Machine::atomicIntInc<int>(&i));
  EXPECT_EQ(11, i); 
  EXPECT_EQ(10, Machine::atomicIntDec<int>(&i));
  EXPECT_EQ(10, i); 

  EXPECT_EQ(10, Machine::atomicWordAddReturn<long>(&l, 10));
  EXPECT_EQ(10, l); 
  EXPECT_EQ(11, Machine::atomicWordInc<long>(&l));
  EXPECT_EQ(11, l); 
  EXPECT_EQ(10, Machine::atomicWordDec<long>(&l));
  EXPECT_EQ(10, l); 

  EXPECT_EQ(10, Machine::fasInt<int>(&i, 0));
  EXPECT_EQ(0, i);
  EXPECT_TRUE(Machine::casInt<int>(&i, 0, 1));
  EXPECT_EQ(1, i);
  EXPECT_FALSE(Machine::casInt<int>(&i, 0, 1));
  EXPECT_EQ(1, i);

  EXPECT_EQ(10, Machine::fasWord<long>(&l, 0));
  EXPECT_EQ(0, l);
  EXPECT_TRUE(Machine::casWord<long>(&l, 0, 1));
  EXPECT_EQ(1, l);
  EXPECT_FALSE(Machine::casWord<long>(&l, 0, 1));
  EXPECT_EQ(1, l);

  EXPECT_TRUE(Machine::cas2Word<LongLong>(&ll, llZero, llOne));
  EXPECT_EQ(llOne, ll);
  EXPECT_FALSE(Machine::cas2Word<LongLong>(&ll, llZero, llOne));
  EXPECT_EQ(llOne, ll);
  Machine::atomicWrite2Word<LongLong>(&ll, llTwo); 
  EXPECT_EQ(llTwo, ll);
  Machine::atomicRead2Word<LongLong>(&ll, &ll2); 
  EXPECT_EQ(ll2, ll);
}

TEST(MachineTest, TSC) {
  long long time1, time2; 

  time1 = Machine::rdtsc(); 
  time2 = Machine::rdtsc(); 
  EXPECT_TRUE(time1 < time2); 
}

} // end of anonymous namespace

