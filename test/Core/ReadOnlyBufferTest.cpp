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

  ReadOnlyBufferTest.cpp  -- unit test case for ReadOnlyBuffer class
 */
#include <cstdlib>
#include "gtest/gtest.h"
#include "Core/ReadOnlyBuffer.h"


using namespace danbi; 
namespace {

TEST(ReadOnlyBufferTest, Basic) {
  const int GoldenIntArray[2][3] = { {1, 2, 3}, 
                                     {4, 5, 6 } };
  ReadOnlyBuffer* ROB; 

  ROB = new ReadOnlyBuffer(GoldenIntArray, sizeof(int), 3, 2); 
  EXPECT_TRUE(ROB != NULL); 
  EXPECT_EQ(0, ROB->loadToDevice(AbstractPartitioner::CPU_DEVICE_INDEX));

  int g = 1; 
  int v; 
  for (int y = 0; y < 2; ++y) {
    for (int x = 0; x < 3; ++x, ++g) {
      v = static_cast<const int *>(ROB->getElementAt(x, y))[0];
      EXPECT_EQ(g, v); 
    }
  }

  delete ROB; 
}

} // end of anonymous namespace

