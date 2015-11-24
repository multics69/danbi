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

  VectorTypeTest.cpp -- unit test cases for vector type
 */
#include "gtest/gtest.h"
#include "Support/VectorType.h"

using namespace danbi; 

namespace {

TEST(VectorTypeTest, Basic) {
  float4 a, b, c; 
  
  a.x = 1.0F;   a.y = 2.0F;   a.z = 3.0F;   a.w = 4.0F; 
  b.x = 5.0F;   b.y = 6.0F;   b.z = 7.0F;   b.w = 8.0F; 
  c = a + b;

  EXPECT_EQ(6.0F, c.x); 
  EXPECT_EQ(8.0F, c.y); 
  EXPECT_EQ(10.0F, c.z); 
  EXPECT_EQ(12.0F, c.w); 

  float coeff = 0.51f;
  float4 X(0.0f); 
  float4 Y; 
  Y = coeff * X; 
  EXPECT_EQ(0.0f, Y.x); 
  EXPECT_EQ(0.0f, Y.y); 
  EXPECT_EQ(0.0f, Y.z); 
  EXPECT_EQ(0.0f, Y.w); 
}

} // end of anonymous namespace

