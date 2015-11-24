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

  MachineConfigTest.cpp -- unit test cases for MachineConfig class
 */
#include <unistd.h>
#include "gtest/gtest.h"
#include "Support/MachineConfig.h"

using namespace danbi; 

namespace {

TEST(MachineConfigTest, CPU) {
  MachineConfig MC = MachineConfig::getInstance(); 
  int NumCore = MC.getNumCoresInCPUs();
  int NumHWThr = MC.getNumHardwareThreadsInCPUs(); 
  int SysConfNumHWThr = ::sysconf(_SC_NPROCESSORS_CONF);

  EXPECT_TRUE( 0 < NumCore && NumCore <= 40 ); 
  EXPECT_EQ(SysConfNumHWThr, NumHWThr); 
  EXPECT_TRUE( (NumCore == NumHWThr) || (NumCore*2 == NumHWThr) );
}

TEST(MachineConfigTest, GPU) {
  MachineConfig MC = MachineConfig::getInstance(); 
  int NumGPU = MC.getNumGPUs(); 

  EXPECT_TRUE(NumGPU >= 0); 
  int PrevCU = 0; 
  for (int i=0; i<NumGPU; ++i) {
    GPUInfo GPU = MC.getGPUInfo(i); 
    EXPECT_LE(PrevCU, GPU.NumComputeUnit);
    EXPECT_TRUE(GPU.NativeSIMTWidth == 32 || GPU.NativeSIMTWidth == 64);
    EXPECT_LT(0, GPU.LocalMemSize);
    PrevCU = GPU.NumComputeUnit; 
  }
}

} // end of anonymous namespace


