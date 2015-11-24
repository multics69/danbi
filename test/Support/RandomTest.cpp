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

  RandomTest.cpp -- unit test cases for random number generator type
 */
#include "gtest/gtest.h"
#include "Support/Random.h"

using namespace danbi; 

namespace {

TEST(RandomTest, Basic) {
  int Prev  = Random::randomInt(); 
  for (int i = 0; i < 10; ++i) {
    int Cur = Random::randomInt(); 
    EXPECT_TRUE(Prev != Cur); 
  }

  for (int i = 0; i < 10; ++i) {
    float Cur = Random::randomFloat(); 
    EXPECT_TRUE( 0.0f<= Cur || Cur <= 1.0f ); 
  }

  const int Count = 10000000; 
  int Bucket[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
  for (int i = 0; i < Count; ++i)
    Bucket[ Random::randomInt() % 10 ] ++; 

  const int MinCount = Count/10 - Count/100; 
  const int MaxCount = Count/10 + Count/100; 
  for (int i = 0; i < 10; ++i)
    EXPECT_TRUE( (MinCount < Bucket[i]) && (Bucket[i] < MaxCount) ); 
}

} // end of anonymous namespace

