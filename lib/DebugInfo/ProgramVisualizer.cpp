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

  ProgramVisualizer.cpp -- danbi program graph visualizer
 */
#include <cassert>
#include <iostream>
#include <fstream>
#include <ctype.h>
#include "DebugInfo/ProgramVisualizer.h"
#include "Core/ProgramDescriptor.h"

using namespace danbi; 

ProgramVisualizer::ProgramVisualizer(ProgramDescriptor& PD_) 
  : PD(PD_) {}

ProgramVisualizer::~ProgramVisualizer() {
  // Do nothing
}

int ProgramVisualizer::generateGraph(std::string Path) {
  return generateGraphInDot(Path); 
}

std::string ProgramVisualizer::generateNormalizedString(const std::string& String) {
#ifdef SKIP_PROCEEDING_DIGITS
  const char* CString = String.c_str(); 

  // Skip heading digits
  for (; (*CString != '\0') && ::isdigit(*CString); ++CString) ;
  
  return CString; 
#else 
  return "\"" + String + "\"";
#endif
}

std::string ProgramVisualizer::generateKernelColor(int Kernel, int NumKernel)  {
  const static int RGBTable[4][3] = {
    {0x00, 0x00, 0xFF}, // blue
    {0x00, 0xFF, 0x00}, // green
    {0xFF, 0xFF, 0x00}, // yellow
    {0xFF, 0x00, 0x00}, //red
  };
  int MixedRGB[3];

  // Linear interpolation of two colors
  float SPos = (float)Kernel / (float)(NumKernel - 1); 
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
  
  // Hexadecimal representation of color
  char Color[32]; 
  ::sprintf(Color, "#%02x%02x%02x", MixedRGB[0], MixedRGB[1], MixedRGB[2]); 
  return std::string(Color);
}

int ProgramVisualizer::generateGraphInDot(std::string Path) {
  // Open a file 
  std::ofstream Dot(Path); 
  if (!Dot.is_open())
    return -EINVAL;

  // Header
  Dot << "digraph \"" << PD.Name << "\" {\n";

  // Shapes
  int NumKernel = PD.KernelTable.size();
  int k = 0; 
  for (std::map<std::string, KernelDescriptor*>::iterator 
         i = PD.KernelTable.begin(), e = PD.KernelTable.end(); 
       i != e; ++i, ++k) {
    KernelDescriptor* d = i->second;
    std::string ParallelString = d->IsParallel ? "box3d" : "box";
    std::string StartString = d->IsStart ? "rounded" : "solid";
    std::string ColorString = generateKernelColor(k, NumKernel); 
    std::string KernelString = generateNormalizedString(i->first); 
    Dot << "node [shape=" << ParallelString 
        << ",style=\"filled," << StartString 
        << "\",fillcolor=\"" << ColorString << "\"]; " 
        << KernelString << ";\n";
  }

  // Links
  for (std::map<std::string, QueueDescriptor*>::iterator 
         i = PD.QTable.begin(), e = PD.QTable.end(); 
       i != e; ++i) {
    QueueDescriptor* q = i->second; 
    std::string QueueString = generateNormalizedString(i->first); 
    std::string ProducerKernelString = generateNormalizedString(q->ProducerKernel);
    std::string ConsumerKernelString = generateNormalizedString(q->ConsumerKernel);
    Dot << ProducerKernelString << "->" << ConsumerKernelString
        << " [ label = " << QueueString << " ];\n";
  }
  
  // Footer
  Dot << "overlap=false\n";
  Dot << "label=\"" << PD.Name << "\"\n";
  Dot << "}\n"; 
}



