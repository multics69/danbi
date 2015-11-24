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

  MemEventLogger.cpp -- Event logger
 */
#include <limits.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "DebugInfo/MemEventLogger.h"
#include "Support/Machine.h"
#include "Support/BranchHint.h"
#include "Core/SchedulingEvent.h"

using namespace danbi; 
__thread MemEventLogger::EventChunk* MemEventLogger::CurChunk; 

MemEventLogger::MemEventLogger() {
  // Do nothing
}

MemEventLogger::~MemEventLogger() {
  // For each per-CPU event chunk list
  EventChunk* Chunk; 
  while (!EventChunkPerThr.pop(Chunk)) {
    // Free all teh chained chunk
    EventChunk* Next;
    while ( (Next = Chunk->Next) ) {
      delete Chunk; 
      Chunk = Next; 
    }
  }
}

int MemEventLogger::initialize_(int NumKernels_, int NumCores_) {
  NumKernels = NumKernels_; 
  NumCores = NumCores_;
  CurrThreadCount = 0;
  AccThreadCount = 0;
  MaxThreadCount = 0; 
  ThreadLifeEvent = 0;
  return 0; 
}

MemEventLogger& MemEventLogger::getInstance() {
  static MemEventLogger Instance; 
  return Instance; 
}

MemEventLogger::EventChunk* MemEventLogger::allocChunk() {
  MemEventLogger::EventChunk* Chunk; 
  Chunk = new MemEventLogger::EventChunk(); 
  Chunk->Next = NULL; 
  Chunk->Index = 0; 
  return Chunk; 
}

void MemEventLogger::appendQueueEvent_(int Kernel, int Queue, 
                                       SchedulingEventKind Event, SchedulingEventKind OrgEvent) {
  static __thread int PrevKernel = 0xdeadbeef; 
  static __thread SchedulingEventKind PrevEvent  = (SchedulingEventKind)0xdead; 

  // Check if it is skippable or not 
  if ((Kernel == PrevKernel) && (Kernel == PrevEvent)) 
    return; 

  // If it is the first time for this thread, 
  // allocate one and add to the global list
  if (unlikely(MemEventLogger::CurChunk == NULL)) {
    CurChunk = allocChunk(); 
    EventChunkPerThr.push(CurChunk); 
  }

  // Check if a new chunk should be allocated or not
  if (unlikely(CurChunk->Index >= ChunkEventNum))
    CurChunk = allocChunk(); 
  assert(0 <= CurChunk->Index && CurChunk->Index < ChunkEventNum); 

  // Append the event 
  MemEventLogger::EventTuple* EventTpl = &CurChunk->Events[CurChunk->Index]; 
  EventTpl->Time = Machine::rdtsc();
  EventTpl->Kernel = PrevKernel = Kernel; 
  EventTpl->Queue = Queue; 
  EventTpl->Event  = PrevEvent  = Event; 
  EventTpl->OrgEvent = OrgEvent; 
  CurChunk->Index++; 
}

void MemEventLogger::appendThreadLifeEvent_(int Kernel, bool Create) {
  volatile long ThreadCount = 
    Machine::atomicWordAddReturn<volatile long>(&CurrThreadCount, Create ? 1 : -1); 

  // averaging only for steady state
  if (ThreadCount >= NumCores) {
    Machine::atomicWordAddReturn<volatile long>(&AccThreadCount, ThreadCount);
    Machine::atomicWordInc<volatile long>(&ThreadLifeEvent); 
  }

  volatile long maxThreadCount = MaxThreadCount; 
  if (ThreadCount > maxThreadCount)
    Machine::casWord<volatile long>(&MaxThreadCount, maxThreadCount, ThreadCount); 
}

void MemEventLogger::findStartEndTime() {
  EventChunk* PerCPUChunk = const_cast<EventChunk*>( EventChunkPerThr.peekPop() );
  StartTime = ULLONG_MAX; 
  EndTime = 0; 

  // For each per-CPU log, find the minimum timep
  NumCPUs = 0; 
  while (PerCPUChunk != NULL) {
    if (PerCPUChunk->Index && PerCPUChunk->Events[0].Time < StartTime)
      StartTime = PerCPUChunk->Events[0].Time;
    EventChunk* Chunk = PerCPUChunk; 
    while (true) {
      if (Chunk->Next == NULL) {
        if (Chunk->Index > 0 && Chunk->Events[Chunk->Index-1].Time > EndTime) 
          EndTime = Chunk->Events[Chunk->Index-1].Time;
        break; 
      }
      Chunk = Chunk->Next; 
    }

    PerCPUChunk = const_cast<EventChunk*>(PerCPUChunk->__Next); 
    ++NumCPUs;
  }
}

double MemEventLogger::convertTickToUnit(unsigned long long Tick) {
  return (double)Tick / 100000.0;
}

void MemEventLogger::generateThreadCountHeader(std::ofstream& Out) {
  Out << "################################\n";
  Out << "# Max Thread Count: " << MaxThreadCount << "\n"; 
  Out << "# Avg Thread Count: " << double(AccThreadCount) / double(ThreadLifeEvent) << "\n";
}

void MemEventLogger::generateHeader(std::ofstream& Out) {
  const char Header1[] = 
    "set terminal pdf color size 1000cm,";
  const char Header2[] = 
    "cm\n"
    "set xrange [0:";
  const char Header3[] = 
    "]\n"
    "set yrange [0:";
  const char Header4[] = 
    "]\n"
    "set autoscale x\n"
    "set xlabel \"time\"\n"
    "set ylabel \"CPU ID\"\n"
    "set key outside width +2\n"
    "set grid xtics\n"
    "set palette model RGB\n"
    "unset colorbox\n";
  Out << Header1 << NumCPUs * 10
      << Header2 << convertTickToUnit(EndTime - StartTime + 1) 
      << Header3 << NumCPUs 
      << Header4; 
}

void MemEventLogger::generateCPUHeader(std::ofstream& Out, int CPUID) {
  const char CPUHeader1[] = 
    "\n\n"
    "################################\n"
    "# CPU ";
  const char CPUHeader2[] = 
    "\n";
  Out << CPUHeader1 << CPUID << CPUHeader2; 
}

std::string MemEventLogger::calcScheduleColor(EventTuple& Event)  {
  const static int RGBTable[4][3] = {
    {0x00, 0x00, 0xFF}, // blue
    {0x00, 0xFF, 0x00}, // green
    {0xFF, 0xFF, 0x00}, // yellow
    {0xFF, 0x00, 0x00}, //red
  };
  int MixedRGB[3] = {0x00, 0x00, 0x00};              // black

  // If it is not idle 
  if ( (Event.Kernel != CPUIsIdle) && 
       (Event.Event != SchedulingEventKind::NotMyTurn) ) {
    // Linear interpolation of three colors
    float SPos = (float)Event.Kernel / (float)(NumKernels - 1); 
    int Base; 
    if (SPos < 0.5) {
      Base = 0; 
      SPos *= 2.0f; 
    }
    else {
      Base = 2; 
      SPos = (SPos - 0.5f) * 2.0f; 
    }
    float EPos = 1.0f - SPos; 
    
    for (int i = 0; i < 3; ++i)
      MixedRGB[i] = (float)RGBTable[Base][i] * EPos + (float)RGBTable[Base+1][i] * SPos; 
  }

  // Hexadecimal representation of color
  char Color[32]; 
  ::sprintf(Color, "#%02x%02x%02x", MixedRGB[0], MixedRGB[1], MixedRGB[2]); 
  return std::string(Color);
}

std::string MemEventLogger::convertEventToString(SchedulingEventKind Event) {
  switch (Event) {
  case InputQueueIsEmpty:
    return "e";
  case OutputQueueIsFull:
    return "f";
  case NotMyTurn:
    return "t";
  case KernelTerminated:
    return "x";
  case RandomJump:
    return "r";
  case ProbabilisticHint:
    return "p";
  }
  return "?";
}

void MemEventLogger::generateEventTuple(std::ofstream& Out, 
                                        int CPUID, int EventID, int PerCPUEventID, 
                                        EventTuple& PrevEvent, EventTuple& ThisEvent) {
  double EventStartTime = convertTickToUnit(PrevEvent.Time - StartTime);
  double EventEndTime = convertTickToUnit(ThisEvent.Time - StartTime);
  double CPUIDStart = (double)CPUID + 0.1; 
  double CPUIDEnd = (double)CPUID + 0.9;
  
  Out << "# " << PerCPUEventID 
      << " K " << PrevEvent.Kernel 
      << " Q " << PrevEvent.Queue
      << " E " << convertEventToString(PrevEvent.Event) 
      << " OE " << convertEventToString(PrevEvent.OrgEvent) << "\n";

  Out << "set object " << EventID << " "
      << "rectangle from " << EventStartTime << ", " << CPUIDStart << " " 
      << "to " << EventEndTime << ", " << CPUIDEnd << " "
      << "fillcolor rgb \"" << calcScheduleColor(PrevEvent) << "\" "
      << "fillstyle solid 1.0 lw 0\n";
}

void MemEventLogger::generateCPUFooter(std::ofstream& Out) {
  // Do nothing
}

void MemEventLogger::generateFooter(std::ofstream& Out) {
  EventTuple Event = {0, 0, (SchedulingEventKind)0, (SchedulingEventKind)0};
  Out << "plot ";
  for (int i = 0; i < NumKernels; ++i) {
    Event.Kernel = i; 
    Out << "-1 title \"\" with lines linecolor rgb " 
        << "\"" << calcScheduleColor(Event) << "\",\\\n";
  }
  Event.Kernel = CPUIsIdle; 
  Out << "-1 title \"\" with lines linecolor rgb " 
      << "\"" << calcScheduleColor(Event) << "\"\n";
}

void MemEventLogger::generateCPU(int CPUID, EventChunk* Chunk, 
                                 std::ofstream& SimpleGraph, std::ofstream& FullGraph) {
  // Generate CPU header
  generateCPUHeader(SimpleGraph, CPUID); 
  generateCPUHeader(FullGraph, CPUID); 

  // Generate event tuples
  MemEventLogger::EventTuple InitialEvent = {
    StartTime, 
    CPUIsIdle, 
    (SchedulingEventKind)0xdead,
    (SchedulingEventKind)0xdead,
  };
  MemEventLogger::EventTuple* PrevEventSimple = &InitialEvent;
  MemEventLogger::EventTuple* PrevEventFull = &InitialEvent;
  //  - for each chunk 
  while (Chunk) {
    //   - for each event tuples in a chunk 
    MemEventLogger::EventTuple* EventTuples = Chunk->Events; 
    for (int i = 0; i < Chunk->Index; ++i) {
      // In case of full graph, generate every event. 
      generateEventTuple(FullGraph, CPUID, ++EventIDFull, i, 
                         *PrevEventFull, EventTuples[i]);
      PrevEventFull = &EventTuples[i];
      
      // In case of simple graph, generate only edge event. 
      // - to a new kernel 
      if (PrevEventSimple->Kernel != EventTuples[i].Kernel) {
        generateEventTuple(SimpleGraph, CPUID, ++EventIDSimple, i,
                           *PrevEventSimple, EventTuples[i]);
        PrevEventSimple = &EventTuples[i];
      }
      // - to the same kernel but NotMyTurn edge
      else {
        if ( (PrevEventSimple->Event != EventTuples[i].Event) && 
             ((PrevEventSimple->Event == SchedulingEventKind::NotMyTurn) ||
              (EventTuples[i].Event == SchedulingEventKind::NotMyTurn)) ) {
          generateEventTuple(SimpleGraph, CPUID, ++EventIDSimple, i, 
                             *PrevEventSimple, EventTuples[i]);
          PrevEventSimple = &EventTuples[i];
        }
      }
    }
    Chunk = Chunk->Next;
  }
    
  generateCPUFooter(SimpleGraph); 
  generateCPUFooter(FullGraph); 
}

int MemEventLogger::generateGraph_(std::string SimpleGraphPath, 
                                   std::string FullGraphPath) {
  // Find the start and end time
  findStartEndTime(); 

  // Open a file 
  std::ofstream SimpleGraph(SimpleGraphPath); 
  if (!SimpleGraph.is_open())
    return -EINVAL;
  std::ofstream FullGraph(FullGraphPath); 
  if (!FullGraph.is_open())
    return -EINVAL;

  // Generate header
  generateThreadCountHeader(SimpleGraph);
  generateHeader(SimpleGraph); 

  generateThreadCountHeader(FullGraph);
  generateHeader(FullGraph); 

  // Generate for each CPU
  EventIDSimple = EventIDFull = 0; 
  int CPUID = 0; 
  EventChunk* Chunk = const_cast<EventChunk*>( EventChunkPerThr.peekPop() );
  while (Chunk != NULL) {
    generateCPU(CPUID, Chunk, SimpleGraph, FullGraph); 
    Chunk = const_cast<EventChunk*>(Chunk->__Next); 
    ++CPUID; 
  }

  // Generate footer
  generateFooter(SimpleGraph); 
  generateFooter(FullGraph); 
  return 0; 
}

