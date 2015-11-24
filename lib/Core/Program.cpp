/*                                                                 --*- C++ -*-
  Copyright (C) 2012 Changwoo Min. All Rights Reserved.

  This file is part of DANBI project. 

  NOTICE: All information contained herein is and remains the property 
  of Changwoo Min. The intellectual and technical concepts contained 
  herein are proprietary to Changwoo Min and may be covered  by patents 
  or patents in process, and are protected by trade secret or copyright law. 
  Dissemination of this information or reproduction of this material is 
  strictly forbidden unless prior written permission is obtained 
  from Changwoo Min(multics69@gmail.com). 

  Program.cpp -- implementation of Program class
 */
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include "Support/AbstractReserveCommitQueue.h"
#include "Core/Program.h"
#include "Core/AbstractCode.h"
#include "Core/Kernel.h"
#include "Core/QueueInfo.h"
#include "Core/ReadOnlyBuffer.h"

using namespace danbi;

Program::Program() 
  : ConnMatrix(StartKernels, QueueInfoTable, FeedbackQueues) {
  // do nothing
}

Program::~Program() {
  // delete all kernels
  for (std::vector<Kernel*>::iterator 
         i = KernelTable.begin(), 
         e = KernelTable.end(); i < e; ++i) {
    delete *i; 
  }
    
  // delete all codes
  for (std::vector<AbstractCode*>::iterator 
         i = CodeTable.begin(), 
         e = CodeTable.end(); i < e; ++i) {
    delete *i; 
  }
    
  // delete all queues
  for (std::vector<QueueInfo*>::iterator 
         i = QueueInfoTable.begin(), 
         e = QueueInfoTable.end(); i < e; ++i) {
    delete *i; 
  }
    
  // delete all read-only buffer
  for (std::vector<ReadOnlyBuffer*>::iterator 
         i = ReadOnlyBufferTable.begin(), 
         e = ReadOnlyBufferTable.end(); i < e; ++i) {
    delete *i; 
  }
}

int Program::addCode(AbstractCode* Code_) {
  if (Code_ == NULL)
    return -EINVAL; 

  CodeTable.push_back(Code_); 
  int Index = CodeTable.size() - 1; 
  Code_->setIndex(Index); 

  return 0; 
}

int Program::addKernel(Kernel *Kernel_) {
  if (Kernel_ == NULL)
    return -EINVAL; 

  KernelTable.push_back(Kernel_); 
  int Index = KernelTable.size() - 1;
  Kernel_->setIndex(Index); 

  if (Kernel_->isStart())
    StartKernels.push_back(Index); 
  return 0; 
}

int Program::addReadOnlyBuffer(ReadOnlyBuffer* ROBuffer_) {
  if (ROBuffer_ == NULL)
    return -EINVAL; 

  ReadOnlyBufferTable.push_back(ROBuffer_); 
  int Index = ReadOnlyBufferTable.size() - 1; 
  ROBuffer_->setIndex(Index); 

  return 0;
}

int Program::addQueue(AbstractReserveCommitQueue* Queue_, 
                      Kernel* ProducerKernel_, 
                      Kernel* ConsumerKernel_, 
                      bool isFeedbackQueue) {
  if (Queue_ == NULL)
    return -EINVAL; 

  int ProducerKernelIndex_ = ProducerKernel_->getIndex();
  if ( !((0 <= ProducerKernelIndex_) && 
         (ProducerKernelIndex_ < KernelTable.size())) )
    return -EINVAL;
    
  int ConsumerKernelIndex_ = ConsumerKernel_->getIndex();
  if ( !((0 <= ConsumerKernelIndex_) && 
         (ConsumerKernelIndex_ < KernelTable.size())) )
    return -EINVAL;

  QueueInfo* QInfo = new QueueInfo(Queue_, 
                                   ProducerKernelIndex_, 
                                   ConsumerKernelIndex_, 
                                   isFeedbackQueue);
  if (!QInfo)
    return -ENOMEM; 
  QueueInfoTable.push_back(QInfo); 
  int Index = QueueInfoTable.size() - 1; 
  Queue_->setIndex(Index); 
  QInfo->setIndex(Index); 

  if (isFeedbackQueue)
    FeedbackQueues.push_back(QInfo); 

  return 0; 
}

bool Program::verify() {
  // For all kernels
  int index = 0;
  for (std::vector<Kernel*>::iterator 
         i = KernelTable.begin(), e = KernelTable.end(); 
       i < e; ++i, ++index) {
    // Verify
    if (!i[0]->verify())
      return false; 
  }

  // Build connectivity matrix
  ConnMatrix.build(); 
  return true; 
}
