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

  ProgramDescriptor.h -- program descriptor how a danbi program is organized. 
 */
#ifndef DANBI_PROGRAM_DESCRIPTOR_H
#define DANBI_PROGRAM_DESCRIPTOR_H
#include <map>
#include <string>

namespace danbi {
class AbstractCode; 
class ReadOnlyBuffer; 
class AbstractReserveCommitQueue; 
class Kernel; 

struct CodeDescriptor {
  void (*Code)(void); 

  AbstractCode* Instance; 

  CodeDescriptor(void (*Code_)(void)): Code(Code_) {}
};

struct ReadOnlyBufferDescriptor {
  void* Buffer;
  int Size;
  int X, Y, Z; 

  ReadOnlyBuffer* Instance;

  ReadOnlyBufferDescriptor(void* Buffer_, int Size_, int X_, int Y_=1, int Z_=1) 
    : Buffer(Buffer_), Size(Size_), X(X_), Y(Y_), Z(Z_) {}
};

struct QueueDescriptor {
  int NumElm; 
  int ElmSize; 
  bool IsFeedback; 
  std::string ProducerKernel; 
  std::string ConsumerKernel;
  bool IssuePushTicket; 
  bool ServePushTicket; 
  bool IssuePopTicket; 
  bool ServePopTicket; 
  bool DebugTrace; 

  AbstractReserveCommitQueue* Instance; 

  QueueDescriptor(int NumElm_, int ElmSize_, bool IsFeedback_,  
                  std::string ProducerKernel_, std::string ConsumerKernel_,
                  bool IssuePushTicket_, bool ServePushTicket_, 
                  bool IssuePopTicket_, bool ServePopTicket_, bool DebugTrace_ = false)
    : NumElm(NumElm_), ElmSize(ElmSize_), IsFeedback(IsFeedback_), 
      ProducerKernel(ProducerKernel_), ConsumerKernel(ConsumerKernel_), 
      IssuePushTicket(IssuePushTicket_), ServePushTicket(ServePushTicket_), 
      IssuePopTicket(IssuePopTicket_), ServePopTicket(ServePopTicket_), 
      DebugTrace(DebugTrace_) {}
};

struct KernelDescriptor {
  std::string Code; 
  bool IsStart; 
  bool IsParallel; 
  std::map<int, std::string> InQ; 
  std::map<int, std::string> OutQ; 
  std::map<int, std::string> ROB;

  Kernel* Instance; 

  KernelDescriptor(std::string Code_, bool IsStart_, bool IsParallel_)
    : Code(Code_), IsStart(IsStart_), IsParallel(IsParallel_) {}
};

struct ProgramDescriptor {
  std::string Name; 
  std::map<std::string, CodeDescriptor*> CodeTable; 
  std::map<std::string, ReadOnlyBufferDescriptor*> ROBTable;
  std::map<std::string, QueueDescriptor*> QTable; 
  std::map<std::string, KernelDescriptor*> KernelTable; 
  ProgramDescriptor(std::string Name_) 
    : Name(Name_) {}
}; 

} // End of danbi namespace 

#endif 
