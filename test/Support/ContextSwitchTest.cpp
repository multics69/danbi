
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

  ContextSwitchTest.cpp -- unit test cases for context switching
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "gtest/gtest.h"
#include "Support/ContextSwitch.h"

#define SCHEDULE_PER_THREAD 5
static int ScheduleHistory[SCHEDULE_PER_THREAD*2]; 
static int HistoryIndex;
static int* Stack1; 
static int* Stack2; 

static void* Thr1SP; 
static void* Thr2SP;  

static int CallbackCount; 

struct GTParam {
  void* SP; 
  int WhoAmI; 
  int Start; 
  GTParam* Next; 
};

void MyYield(GTParam* Arg) {
  danbi_ContextSwitch(&Arg->SP, Arg->Next->SP); 
}

void GreenThread(GTParam* Arg) {
  int Start = Arg->Start; 
  int i; 
  for (i = Start; i < (Start+SCHEDULE_PER_THREAD); ++i) {
    ScheduleHistory[HistoryIndex] = i; 
    ++HistoryIndex; 
    MyYield(Arg); 
   }
  MyYield(Arg); 
}

void TestCallback(int Arg) {
  EXPECT_EQ(1, Arg); 
  CallbackCount++;
}

void MyYieldWithCallback(GTParam* Arg) {
  danbi_ContextSwitchWithCallback(&Arg->SP, Arg->Next->SP, 
                                  (void (*)(void*))TestCallback, (void *)1); 
}

void GreenThreadWithCallback(GTParam* Arg) {
  int Start = Arg->Start; 
  int i; 
  for (i = Start; i < (Start+SCHEDULE_PER_THREAD); ++i) {
    ScheduleHistory[HistoryIndex] = i; 
    ++HistoryIndex; 
    MyYieldWithCallback(Arg); 
   }
  MyYieldWithCallback(Arg); 
}

TEST(ContextSwitchTest, Basic) {
  volatile unsigned int SV01 = 0xabcddc01UL;
  GTParam Param1; 
  volatile unsigned int SV02 = 0xabcddc02UL;
  volatile unsigned int SV03 = 0xabcddc03UL;
  GTParam Param2; 
  volatile unsigned int SV04 = 0xabcddc04UL;

  /* Init */ 
  HistoryIndex = 0;
  Stack1 = new int[1024];
  EXPECT_TRUE(Stack1 != NULL); 
  Stack2 = new int[1024];
  EXPECT_TRUE(Stack2 != NULL); 
  Param1.SP = danbi_InitGreenStack(Stack1, 4096, (void*)GreenThread, &Param1);
  Param2.SP = danbi_InitGreenStack(Stack2, 4096, (void*)GreenThread, &Param2); 

  Param1.WhoAmI = 1; 
  Param2.WhoAmI = 2; 
  Param1.Start  = 1; 
  Param2.Start  = 1000; 
  Param1.Next   = &Param2; 
  Param2.Next   = &Param1; 

  /* Start from Thread1 */ 
  danbi_StartGreenThread(Param1.SP); 

  /* Check if there is no stack corruption. */ 
  EXPECT_EQ(0xabcddc01UL, SV01); 
  EXPECT_EQ(0xabcddc02UL, SV02); 
  EXPECT_EQ(0xabcddc03UL, SV03); 
  EXPECT_EQ(0xabcddc04UL, SV04); 

  /* Check if scheduling is correct. */ 
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

  /* Check if there is no stack corruption. */ 
  EXPECT_EQ(0xabcddc01UL, SV01); 
  EXPECT_EQ(0xabcddc02UL, SV02); 
  EXPECT_EQ(0xabcddc03UL, SV03); 
  EXPECT_EQ(0xabcddc04UL, SV04); 
  EXPECT_EQ(0, danbi_IsGreenStackSane(Stack1, 4096));
  EXPECT_EQ(0, danbi_IsGreenStackSane(Stack2, 4096));

  delete Stack1; 
  delete Stack2; 
}

TEST(ContextSwitchTest, Callback) {
  volatile unsigned int SV01 = 0xabcddc01UL;
  GTParam Param1; 
  volatile unsigned int SV02 = 0xabcddc02UL;
  unsigned int SV03 = 0xabcddc03UL;
  GTParam Param2; 
  volatile unsigned int SV04 = 0xabcddc04UL;

  /* Init */ 
  HistoryIndex = 0;
  Stack1 = new int[1024];
  EXPECT_TRUE(Stack1 != NULL); 
  Stack2 = new int[1024];
  EXPECT_TRUE(Stack2 != NULL); 
  Param1.SP = danbi_InitGreenStack(Stack1, 4096, 
                                   (void*)GreenThreadWithCallback, &Param1);
  Param2.SP = danbi_InitGreenStack(Stack2, 4096, 
                                   (void*)GreenThreadWithCallback, &Param2); 

  Param1.WhoAmI = 1; 
  Param2.WhoAmI = 2; 
  Param1.Start  = 1; 
  Param2.Start  = 1000; 
  Param1.Next   = &Param2; 
  Param2.Next   = &Param1; 

  /* Start from Thread1 */ 
  danbi_StartGreenThread(Param1.SP); 

  /* Check if there is no stack corruption. */ 
  EXPECT_EQ(0xabcddc01UL, SV01); 
  EXPECT_EQ(0xabcddc02UL, SV02); 
  EXPECT_EQ(0xabcddc03UL, SV03); 
  EXPECT_EQ(0xabcddc04UL, SV04); 

  /* Check if scheduling is correct. */ 
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

  /* Check if there is no stack corruption. */ 
  EXPECT_EQ(0xabcddc01UL, SV01); 
  EXPECT_EQ(0xabcddc02UL, SV02); 
  EXPECT_EQ(0xabcddc03UL, SV03); 
  EXPECT_EQ(0xabcddc04UL, SV04); 
  EXPECT_EQ(0, danbi_IsGreenStackSane(Stack1, 4096));
  EXPECT_EQ(0, danbi_IsGreenStackSane(Stack2, 4096));

  /* Check if callback is correctly called. */ 
  EXPECT_EQ(CallbackCount, (SCHEDULE_PER_THREAD + 1) * 2); 

  delete Stack1; 
  delete Stack2; 
}

