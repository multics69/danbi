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

  Program.h -- danbi program instance
 */
#ifndef DANBI_PROGRAM_H
#define DANBI_PROGRAM_H
#include <cerrno>
#include <vector>
#include "Support/ConnectivityMatrix.h"

namespace danbi {
class AbstractCode; 
class Kernel; 
class ReadOnlyBuffer; 
class QueueInfo; 
class AbstractReserveCommitQueue; 
class AbstractPartitioner; 

class Program {
  friend class Kernel; 
  friend class AbstractPartitioner; 
  friend class AbstractScheduler; 
  friend class Runtime; 

private:
  std::vector<AbstractCode*> CodeTable; 
  std::vector<Kernel*> KernelTable; 
  std::vector<int> StartKernels; 
  std::vector<QueueInfo*> QueueInfoTable; 
  std::vector<QueueInfo*> FeedbackQueues; 
  std::vector<ReadOnlyBuffer*> ReadOnlyBufferTable; 
  ConnectivityMatrix<QueueInfo*> ConnMatrix; 

  void operator=(const Program&); // Do not implement
  Program(const Program&); // Do not implement

public:
  /// Constructor
  Program(); 
  
  /// Destructor: deep destruction deleting all objects in the tables
  virtual ~Program(); 

  /// Add code
  int addCode(AbstractCode* Code_);

  /// Add kernel
  int addKernel(Kernel *Kernel_);

  /// Add read-only buffer: returns assigned buffer index
  int addReadOnlyBuffer(ReadOnlyBuffer* ROBuffer_);

  /// Add Queue: returns assigned queue index
  int addQueue(AbstractReserveCommitQueue* Queue_, 
               Kernel* ProducerKernel_, 
               Kernel* ConsumerKernel_, 
               bool isFeedbackQueue = false);

  /// Verify if the configuration is valid
  bool verify(); 

  /// Get number of kernels
  int getNumberOfKernels() {
    return KernelTable.size();
  }
};

} // End of danbi namespace
#endif
