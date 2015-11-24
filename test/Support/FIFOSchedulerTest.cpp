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

  FIFOSchedulerTest.cpp -- unit test cases for FIFO scheduler
 */
#include "gtest/gtest.h"
#include "Support/AbstractGreenRunnable.h"
#include "Support/GreenThread.h"
#include "Support/GreenScheduler.h"
#include "Support/FIFOSchedulePolicy.h"

using namespace danbi; 

#define SCHEDULE_PER_THREAD 4096
static int ScheduleHistory[SCHEDULE_PER_THREAD*2]; 
static int HistoryIndex; 

template<typename GreenSchedulerTy>
class TestGreenRunnable : public AbstractGreenRunnable<GreenSchedulerTy> {
  DANBI_GREEN_RUNNABLE_YIELDABLE(GreenSchedulerTy);
private:
  int Start; 
public:
  TestGreenRunnable(int Start_) 
    : Start(Start_) {}

  virtual void main() {
    int i; 
    for (i = Start; i < (Start+SCHEDULE_PER_THREAD); ++i) {
      ScheduleHistory[HistoryIndex] = i; 
      ++HistoryIndex; 
      yield(); 
    }
    yield();
  }
};

TEST(FIFOSchedulerTest, Basic) {
  HistoryIndex = 0;
  unsigned int SV01 = 0xabcddc01UL; 
  TestGreenRunnable<GreenScheduler<FIFOSchedulePolicy>>* GR1;
  unsigned int SV02 = 0xabcddc02UL;
  TestGreenRunnable<GreenScheduler<FIFOSchedulePolicy>>* GR2;
  unsigned int SV03 = 0xabcddc03UL;
  GreenThread<GreenScheduler<FIFOSchedulePolicy>>* GT1;
  unsigned int SV04 = 0xabcddc04UL;
  GreenThread<GreenScheduler<FIFOSchedulePolicy>>* GT2;

  // New 
  GR1 = new TestGreenRunnable<GreenScheduler<FIFOSchedulePolicy>>(1); 
  EXPECT_TRUE(GR1 != NULL); 
  GR2 = new TestGreenRunnable<GreenScheduler<FIFOSchedulePolicy>>(1000); 
  EXPECT_TRUE(GR2 != NULL); 
  GT1 = new GreenThread<GreenScheduler<FIFOSchedulePolicy>>(*GR1); 
  EXPECT_TRUE(GT1 != NULL); 
  GT2 = new GreenThread<GreenScheduler<FIFOSchedulePolicy>>(*GR2); 
  EXPECT_TRUE(GT2 != NULL); 
  EXPECT_EQ(0, GT1->initialize()); 
  EXPECT_EQ(0, GT2->initialize()); 
    
  // Check if there is no stack corruption.
  EXPECT_EQ(0xabcddc01UL, SV01); 
  EXPECT_EQ(0xabcddc02UL, SV02); 
  EXPECT_EQ(0xabcddc03UL, SV03); 
  EXPECT_EQ(0xabcddc04UL, SV04); 
    
  // Create native thread to run green threads
  EXPECT_EQ(0, 
            GreenScheduler<FIFOSchedulePolicy>::getInstance().createNativeThreads(1, true));

  // Run scheduler
  EXPECT_EQ(0, GreenScheduler<FIFOSchedulePolicy>::getInstance().start());

  // Join until the end
  EXPECT_EQ(0, GreenScheduler<FIFOSchedulePolicy>::getInstance().join());

  // Check all terminated
  EXPECT_TRUE(GT1->isTerminated()); 
  EXPECT_TRUE(GT2->isTerminated()); 

  // Check if scheduling is correct. 
  EXPECT_EQ(   1, ScheduleHistory[0]);
  EXPECT_EQ(1000, ScheduleHistory[1]);
  EXPECT_EQ(   2, ScheduleHistory[2]);
  EXPECT_EQ(1001, ScheduleHistory[3]);
  EXPECT_EQ(   3, ScheduleHistory[4]);
  EXPECT_EQ(1002, ScheduleHistory[5]);
  EXPECT_EQ(   4, ScheduleHistory[6]);
  EXPECT_EQ(1003, ScheduleHistory[7]);
  EXPECT_EQ(   5, ScheduleHistory[8]);
  EXPECT_EQ(1004, ScheduleHistory[9]);

  // Check if there is no stack corruption.
  EXPECT_EQ(0xabcddc01UL, SV01); 
  EXPECT_EQ(0xabcddc02UL, SV02); 
  EXPECT_EQ(0xabcddc03UL, SV03); 
  EXPECT_EQ(0xabcddc04UL, SV04); 

  // Delete
  delete GR1; 
  delete GT1; 

  delete GT2; 
  delete GR2; 
}

TEST(FIFOSchedulerTest, Multithreaded) {
  HistoryIndex = 0;
  unsigned int SV01 = 0xabcddc01UL; 
  TestGreenRunnable<GreenScheduler<FIFOSchedulePolicy>>* GR1;
  unsigned int SV02 = 0xabcddc02UL;
  TestGreenRunnable<GreenScheduler<FIFOSchedulePolicy>>* GR2;
  unsigned int SV03 = 0xabcddc03UL;
  GreenThread<GreenScheduler<FIFOSchedulePolicy>>* GT1;
  unsigned int SV04 = 0xabcddc04UL;
  GreenThread<GreenScheduler<FIFOSchedulePolicy>>* GT2;

  // New 
  GR1 = new TestGreenRunnable<GreenScheduler<FIFOSchedulePolicy>>(1); 
  EXPECT_TRUE(GR1 != NULL); 
  GR2 = new TestGreenRunnable<GreenScheduler<FIFOSchedulePolicy>>(1000); 
  EXPECT_TRUE(GR2 != NULL); 
  GT1 = new GreenThread<GreenScheduler<FIFOSchedulePolicy>>(*GR1); 
  EXPECT_TRUE(GT1 != NULL); 
  GT2 = new GreenThread<GreenScheduler<FIFOSchedulePolicy>>(*GR2); 
  EXPECT_TRUE(GT2 != NULL); 
  EXPECT_EQ(0, GT1->initialize()); 
  EXPECT_EQ(0, GT2->initialize()); 
    
  // Check if there is no stack corruption.
  EXPECT_EQ(0xabcddc01UL, SV01); 
  EXPECT_EQ(0xabcddc02UL, SV02); 
  EXPECT_EQ(0xabcddc03UL, SV03); 
  EXPECT_EQ(0xabcddc04UL, SV04); 
    
  // Create native thread to run green threads
  EXPECT_EQ(0, 
            GreenScheduler<FIFOSchedulePolicy>::getInstance().createNativeThreads(2, true));

  // Run scheduler
  EXPECT_EQ(0, GreenScheduler<FIFOSchedulePolicy>::getInstance().start());

  // Join until the end
  EXPECT_EQ(0, GreenScheduler<FIFOSchedulePolicy>::getInstance().join());

  // Check all terminated
  EXPECT_TRUE(GT1->isTerminated()); 
  EXPECT_TRUE(GT2->isTerminated()); 

  // Check if there is no stack corruption.
  EXPECT_EQ(0xabcddc01UL, SV01); 
  EXPECT_EQ(0xabcddc02UL, SV02); 
  EXPECT_EQ(0xabcddc03UL, SV03); 
  EXPECT_EQ(0xabcddc04UL, SV04); 

  // Delete
  delete GR1; 
  delete GT1; 

  delete GT2; 
  delete GR2; 
}
