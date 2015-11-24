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

  CPUCodeTest.cpp  -- unit test case for CPUCode class
 */
#include <cstdlib>
#include <cerrno>
#include "gtest/gtest.h"
#include "Core/CPUCode.h"
#include "Core/SchedulingEvent.h"
#include "Core/AbstractPartitioner.h"


using namespace danbi; 
namespace {
static int HelloWorld = 0; 

void KernelMain(void) {
  ++HelloWorld; 
}

TEST(CPUCodeTest, BasicFunction) {
  CPUCode* Code; 
  int QueueIndex; 

  Code = new CPUCode(KernelMain); 
  EXPECT_TRUE(Code); 
  EXPECT_TRUE(Code->supportDevice(AbstractPartitioner::CPU_DEVICE_INDEX)); 
  EXPECT_FALSE(Code->supportDevice(1));
  EXPECT_EQ(-ENOTSUP, Code->loadToDevice(1));
  EXPECT_EQ(0, Code->loadToDevice(AbstractPartitioner::CPU_DEVICE_INDEX));
  Code->run(); 
  EXPECT_EQ(1, HelloWorld); 
  Code->run(); 
  EXPECT_EQ(2, HelloWorld); 
  delete Code; 
}

TEST(CPUCodeTest, BasicSharedLib) {
  CPUCode* Code; 

  Code = new CPUCode("libm.so", "sqrt"); 
  EXPECT_TRUE(Code); 
  EXPECT_TRUE(Code->supportDevice(AbstractPartitioner::CPU_DEVICE_INDEX)); 
  EXPECT_FALSE(Code->supportDevice(1));
  EXPECT_EQ(-ENOTSUP, Code->loadToDevice(1));
  EXPECT_EQ(0, Code->loadToDevice(AbstractPartitioner::CPU_DEVICE_INDEX));
  delete Code; 

  Code = new CPUCode("xXxXlibm.so", "sqrt"); 
  EXPECT_TRUE(Code); 
  EXPECT_EQ(-ENOENT, Code->loadToDevice(AbstractPartitioner::CPU_DEVICE_INDEX));
  delete Code; 

  Code = new CPUCode("libm.so", "xXxXsqrt"); 
  EXPECT_TRUE(Code); 
  EXPECT_EQ(-EINVAL, Code->loadToDevice(AbstractPartitioner::CPU_DEVICE_INDEX));
  delete Code; 
}

} // end of anonymous namespace

