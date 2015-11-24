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

  ProgramFactory.h -- Object factory class for danbi program construction 
 */
#ifndef DANBI_PROGRAM_FACTORY_H
#define DANBI_PROGRAM_FACTORY_H
#include <cerrno>
#include "Core/Program.h"
#include "Core/ProgramDescriptor.h"

namespace danbi {

/// Objcectory factory for danbi program construction 
class ProgramFactory {
private:
  std::map<std::string, CodeDescriptor*>* CodeTable; 
  std::map<std::string, ReadOnlyBufferDescriptor*>* ROBTable;
  std::map<std::string, QueueDescriptor*>* QTable; 
  std::map<std::string, KernelDescriptor*>* KernelTable; 
  Program* Pgm;
  
  void operator=(const ProgramFactory&); // Do not implement
  ProgramFactory(const ProgramFactory&); // Do not implement 

  int createCodes(); 
  int createReadOnlyBuffers(); 
  int createKernels();
  int createQueues(); 
  int constructKernels(); 

  int initMemPolicy(); 
public:
  ProgramFactory() {}

  Program* newProgram(ProgramDescriptor& Descriptor_); 
};

} // End of danbi namespace

#endif 
