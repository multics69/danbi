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

  ProgramFactory.cpp -- implementation of object factory for Program class
 */
#include <numaif.h>
#include "Core/ProgramFactory.h"
#include "Core/QueueFactory.h"
#include "Core/ReadOnlyBuffer.h"
#include "Core/CPUCode.h"
#include "Core/Kernel.h"
#include "Support/Debug.h"

using namespace danbi; 

int ProgramFactory::createCodes() {
  for (std::map<std::string, CodeDescriptor*>::iterator 
         i = CodeTable->begin(), e = CodeTable->end(); 
       i != e; ++i) {
    CodeDescriptor* d = i->second; 
    d->Instance = new CPUCode(d->Code); 
    if (d->Instance == NULL)
      return -ENOMEM; 

    int ret = Pgm->addCode(d->Instance); 
    if (ret)
      return ret; 
  }
  return 0; 
}

int ProgramFactory::createReadOnlyBuffers() {
  for (std::map<std::string, ReadOnlyBufferDescriptor*>::iterator 
         i = ROBTable->begin(), e = ROBTable->end(); 
       i != e; ++i) {
    ReadOnlyBufferDescriptor* d = i->second; 
    d->Instance = new ReadOnlyBuffer(d->Buffer, d->Size, d->X, d->Y, d->Z); 
    if (d->Instance == NULL)
      return -ENOMEM; 

    int ret = Pgm->addReadOnlyBuffer(d->Instance); 
    if (ret)
      return ret; 
  }
  return 0; 
}

int ProgramFactory::createKernels() {
  int c = 0;  
  for (std::map<std::string, KernelDescriptor*>::iterator 
         i = KernelTable->begin(), e = KernelTable->end(); 
       i != e; ++i, ++c) {
    KernelDescriptor* d = i->second;
    DBG(
      std::cout << " Kernel Index: " << c << " : " << i->first << std::endl; 
      ); 
    d->Instance = new Kernel(*Pgm, d->IsParallel ? 
                             ExecutionKind::ParallelExec : 
                             ExecutionKind::SeqeuentialExec,
                             d->IsStart); 
    if (d->Instance == NULL)
      return -ENOMEM; 

    int ret = Pgm->addKernel(d->Instance); 
    if (ret)
      return ret; 
  }
  return 0; 
}

int ProgramFactory::createQueues() {
  int c = 0;  
  for (std::map<std::string, QueueDescriptor*>::iterator 
         i = QTable->begin(), e = QTable->end(); 
       i != e; ++i, ++c) {
    QueueDescriptor* d = i->second;

    DBG(
      std::cout << " Queue Index: " << c << " : " << i->first << std::endl; 
      ); 

    std::map<std::string, KernelDescriptor*>::iterator Producer = 
      KernelTable->find(d->ProducerKernel); 
    if (Producer == KernelTable->end()) return -EINVAL;

    std::map<std::string, KernelDescriptor*>::iterator Consumer = 
      KernelTable->find(d->ConsumerKernel); 
    if (Consumer == KernelTable->end()) return -EINVAL;

    d->Instance = QueueFactory::newQueue(d->ElmSize, d->NumElm, 
                                         d->IssuePushTicket, d->ServePushTicket, 
                                         d->IssuePopTicket, d->ServePopTicket, 
                                         false, false); 
    if (d->Instance == NULL)
      return -ENOMEM; 
    d->Instance->__setDebugTrace(d->DebugTrace);

    int ret = Pgm->addQueue(d->Instance, 
                            Producer->second->Instance, 
                            Consumer->second->Instance, 
                            d->IsFeedback); 
    if (ret)
      return ret; 
  }
  return 0; 
}

int ProgramFactory::constructKernels() {
  for (std::map<std::string, KernelDescriptor*>::iterator 
         i = KernelTable->begin(), e = KernelTable->end(); 
       i != e; ++i) {
    KernelDescriptor* d = i->second;
    Kernel* k = d->Instance; 
    int Num; 
    int ret; 

    DBG( HERE() << " " << i->first << std::endl; ); 

    // Set code 
    ret = k->setCode((*CodeTable)[d->Code]->Instance); 
    if (ret) return ret; 

    // Add input queues
    Num = d->InQ.size();
    for (int i = 0; i < Num; ++i) {
      std::map<int, std::string>::iterator v = d->InQ.find(i);
      if (v == d->InQ.end()) return -EINVAL; 

      DBG( HERE() << " " << v->second << std::endl; ); 
      std::map<std::string, QueueDescriptor*>::iterator q = QTable->find(v->second); 
      if (q == QTable->end()) return -EINVAL;

      ret = k->addInputQueue(q->second->Instance); 
      if (ret) return ret; 
    }
    DBG( HERE() << std::endl; ); 

    // Add output queues
    Num = d->OutQ.size();
    for (int i = 0; i < Num; ++i) {
      std::map<int, std::string>::iterator v = d->OutQ.find(i);
      if (v == d->OutQ.end()) return -EINVAL; 

      DBG( HERE() << " " << v->second << std::endl; ); 
      std::map<std::string, QueueDescriptor*>::iterator q = QTable->find(v->second); 
      if (q == QTable->end()) return -EINVAL;

      ret = k->addOutputQueue(q->second->Instance); 
      if (ret) return ret; 
    }
    DBG( HERE() << std::endl; ); 
      
    // Add read only buffer
    Num = d->ROB.size();
    for (int i = 0; i < Num; ++i) {
      std::map<int, std::string>::iterator v = d->ROB.find(i);
      if (v == d->ROB.end()) return -EINVAL; 

      DBG( HERE() << " " << v->second << std::endl; ); 
      std::map<std::string, ReadOnlyBufferDescriptor*>::iterator b = ROBTable->find(v->second); 
      if (b == ROBTable->end()) return -EINVAL;

      ret = k->addReadOnlyBuffer(b->second->Instance); 
      if (ret) return ret; 
    }
    DBG( HERE() << std::endl; ); 
  }
  return 0; 
}

int ProgramFactory::initMemPolicy() {
#ifndef DANBI_DO_NOT_USE_NUMA_INTERLEAVE_MEM_POLICY
  unsigned long NodeMask = (unsigned long)-1; 

  // For NUMA architecture, set physical memory allocation policy to interleave. 
  // It is bandwith oriented configuration rather than latency oriented one. 
  return ::set_mempolicy(MPOL_INTERLEAVE, &NodeMask, sizeof(NodeMask) * 8);
#else
  return 0; 
#endif 
}

Program* ProgramFactory::newProgram(ProgramDescriptor& Descriptor_) {
  /// Initialize memory allocation policy
  if (initMemPolicy()) 
    return NULL; 

  /// Initialize argument
  CodeTable = &Descriptor_.CodeTable; 
  ROBTable = &Descriptor_.ROBTable; 
  QTable = &Descriptor_.QTable; 
  KernelTable = &Descriptor_.KernelTable; 
  Pgm = new Program(); 

  /// Create objects
  int ret; 
  if ( (ret = createCodes()) ) goto error; 
  if ( (ret = createReadOnlyBuffers()) ) goto error; 
  if ( (ret = createKernels()) ) goto error; 
  if ( (ret = createQueues()) ) goto error; 

  /// Construct kernels
  if ( (ret = constructKernels()) ) goto error; 

  /// Success return 
  return Pgm; 

  /// Error return 
 error:
  delete Pgm; 
  return NULL; 
}
