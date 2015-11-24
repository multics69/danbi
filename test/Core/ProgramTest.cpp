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

  ProgramTest.cpp  -- unit test case for Program, Kernel class
 */
#include <cstdlib>
#include "gtest/gtest.h"
#include "Core/Program.h"
#include "Core/Kernel.h"
#include "Core/CPUCode.h"
#include "Core/QueueFactory.h"
#include "Core/ReadOnlyBuffer.h"
#include "Core/SchedulingEvent.h"
#include "Core/AbstractPartitioner.h"


using namespace danbi; 
namespace {
static int HelloWorld = 0; 

void KernelMain1(void) {
  ++HelloWorld; 
}

void KernelMain2(void) {
  ++HelloWorld; 
}

TEST(ProgramTest, Basic) {
  
  // create program
  Program* Pgm = new Program(); 
  EXPECT_TRUE(Pgm != NULL); 

  // add kernel
  AbstractCode* Code1 = new CPUCode(KernelMain1);
  EXPECT_TRUE(Code1 != NULL); 
  EXPECT_EQ(0, Pgm->addCode(Code1));
  AbstractCode* Code2 = new CPUCode(KernelMain2);
  EXPECT_TRUE(Code2 != NULL); 
  EXPECT_EQ(0, Pgm->addCode(Code2));

  Kernel* Kernel1 = new Kernel(*Pgm, SeqeuentialExec, true); 
  EXPECT_TRUE(Kernel1 != NULL); 
  EXPECT_EQ(0, Kernel1->setCode(Code1));
  EXPECT_EQ(0, Pgm->addKernel(Kernel1));

  Kernel* Kernel2 = new Kernel(*Pgm, SeqeuentialExec, false); 
  EXPECT_TRUE(Kernel2 != NULL); 
  EXPECT_EQ(0, Kernel2->setCode(Code2));
  EXPECT_EQ(0, Pgm->addKernel(Kernel2));

  Kernel* Kernel3 = new Kernel(*Pgm, SeqeuentialExec, false); 
  EXPECT_TRUE(Kernel3 != NULL); 
  EXPECT_EQ(0, Kernel3->setCode(Code2));
  EXPECT_EQ(0, Pgm->addKernel(Kernel3));
  
  // add read-only buffer
  int ConstantArray[4] = {1, 2, 3, 4};
  ReadOnlyBuffer* ROBuffer = new ReadOnlyBuffer(ConstantArray, sizeof(int), 4); 
  EXPECT_TRUE(ROBuffer != NULL); 
  EXPECT_EQ(0, ROBuffer->loadToDevice(AbstractPartitioner::CPU_DEVICE_INDEX));
  EXPECT_EQ(0, Pgm->addReadOnlyBuffer(ROBuffer));

  // add queues
  AbstractReserveCommitQueue* Queue1 = 
    QueueFactory::newQueue(sizeof(int), 10, 
                           false, false, false, false,
                           false, false); 
  EXPECT_EQ(0, Pgm->addQueue(Queue1, Kernel1, Kernel2, false));

  AbstractReserveCommitQueue* Queue2 = 
    QueueFactory::newQueue(sizeof(int), 10,
                           false, false, false, false,
                           false, false); 
  EXPECT_EQ(0, Pgm->addQueue(Queue2, Kernel2, Kernel3, false));

  // configure kernel 
  EXPECT_EQ(0, Kernel1->addOutputQueue(Queue1));

  EXPECT_EQ(0, Kernel2->addInputQueue(Queue1));
  EXPECT_EQ(0, Kernel2->addOutputQueue(Queue2));
  EXPECT_EQ(0, Kernel2->addReadOnlyBuffer(ROBuffer));

  EXPECT_EQ(0, Kernel3->addInputQueue(Queue2));
  EXPECT_EQ(0, Kernel3->addReadOnlyBuffer(ROBuffer));

  // verify the context
  EXPECT_TRUE(Pgm->verify());

  // delete program
  delete Pgm; 
}

} // end of anonymous namespace

