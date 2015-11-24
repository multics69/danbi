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

  MemEventLogger.h -- Memory Event logger
 */
#ifndef DANBI_MEM_EVENT_LOGGER_H
#define DANBI_MEM_EVENT_LOGGER_H
#include <iostream>
#include <fstream>
#include <string>
#include <cerrno>
#include "DebugInfo/MemEventLogger.h"
#include "Support/SuperScalableQueue.h"
#include "Core/SchedulingEvent.h"

namespace danbi {

class MemEventLogger {
private:
  struct EventTuple {
    unsigned long long Time;
    int Kernel;
    int Queue; 
    SchedulingEventKind Event;
    SchedulingEventKind OrgEvent; 
  };

  enum {
    ChunkEventNum = 1024 * 1024 - 1,
    CPUIsIdle = -1, 
  };

  struct EventChunk {
    DANBI_SSQ_ITERABLE(EventChunk);
    EventChunk* Next; 
    int Index; 
    EventTuple Events[ChunkEventNum];
  };

  static __thread EventChunk* CurChunk;
  SuperScalableQueue<EventChunk> EventChunkPerThr;
  unsigned long long StartTime, EndTime; 
  int NumKernels; 
  int NumCores;
  int NumCPUs;
  int EventIDSimple, EventIDFull;
  volatile long CurrThreadCount;
  volatile long AccThreadCount;
  volatile long MaxThreadCount;
  volatile long ThreadLifeEvent; 

  void operator=(const MemEventLogger&); // Do not implement
  MemEventLogger(const MemEventLogger&); // Do not implement
  MemEventLogger(); // getInstance() is the only way to create this class
  ~MemEventLogger();

  EventChunk* allocChunk(); 
  static MemEventLogger& getInstance(); 

  int initialize_(int NumKernels_, int NumCores_); 

  void appendQueueEvent_(int Kernel, int Queue, 
               SchedulingEventKind Event, SchedulingEventKind OrgEvent); 
  void appendThreadLifeEvent_(int Kernel, bool Create);

  void findStartEndTime(); 

  double convertTickToUnit(unsigned long long Tick);
  std::string convertEventToString(SchedulingEventKind Event);
  void generateThreadCountHeader(std::ofstream& Out); 
  void generateHeader(std::ofstream& Out); 
  void generateCPUHeader(std::ofstream& Out, int CPUID); 
  std::string calcScheduleColor(EventTuple& Event); 
  void generateEventTuple(std::ofstream& Out, 
                          int CPUID, int EventID, int PerCPUEventID,
                          EventTuple& PrevEvent, EventTuple& ThisEvent);
  void generateCPUFooter(std::ofstream& Out);
  void generateFooter(std::ofstream& Out);
  void generateCPU(int CPUID, EventChunk* Chunk, 
                   std::ofstream& SimpleGraph, std::ofstream& FullGraph); 
  int generateGraph_(std::string SimpleGraphPath, std::string FullGraphPath);
public:
  /// Initilize
  static inline int initialize(int NumKernels_, int NumCores_) {
    return MemEventLogger::getInstance().initialize_(NumKernels_, NumCores_);
  }

  /// Append queue event
  static inline void appendQueueEvent(int Kernel, int Queue, 
                            SchedulingEventKind Event, SchedulingEventKind OrgEvent) {
#ifndef DANBI_EVENT_LOGGER_DISABLE_QUEUE_EVENT
    MemEventLogger::getInstance().appendQueueEvent_(Kernel, Queue, Event, OrgEvent); 
#endif 
  }

  /// Append thread lifecycle event
  static inline void appendThreadLifeEvent(int Kernel, bool Create) {
#ifndef DANBI_EVENT_LOGGER_DISABLE_THREAD_LIFECYCLE
    MemEventLogger::getInstance().appendThreadLifeEvent_(Kernel, Create); 
#endif
  }

  /// Generate schedule graph
  static inline int generateGraph(std::string SimpleGraphPath, 
                                  std::string FullGraphPath) {
    MemEventLogger::getInstance().generateGraph_(SimpleGraphPath, FullGraphPath);
  }
}; 

} // End of danbi namespace

#endif 
