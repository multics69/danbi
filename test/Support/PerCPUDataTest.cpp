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

  PerCPUDataTest.cpp -- unit test cases for per-CPU data type
 */
#include "gtest/gtest.h"
#include "Support/MachineConfig.h"
#include "Support/PerCPUData.h"

using namespace danbi; 

namespace {

TEST(PerCPUDataTest, Basic) {
  PerCPUData<int> Token; 

  EXPECT_EQ(0, Token.initialize());
 
  int Cores = MachineConfig::getInstance().getNumHardwareThreadsInCPUs(); 
  for (int i = 0; i < Cores; ++i) {
    Token[i] = 1234 + i; 
    EXPECT_EQ(1234 + i, Token[i]); 
  }

  for (int i = 0; i < Cores; ++i) {
    EXPECT_EQ(1234 + i, Token[i]); 
  }
}

} // end of anonymous namespace

