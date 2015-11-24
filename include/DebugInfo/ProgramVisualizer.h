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

  ProgramVisualizer.h -- danbi program graph visualizer
 */
#ifndef DANBI_PROGRAM_VISUALIZER_H
#define DANBI_PROGRAM_VISUALIZER_H
#include <cerrno>
#include <string>

namespace danbi {
struct ProgramDescriptor; 

class ProgramVisualizer {
private:
  ProgramDescriptor& PD; 

  void operator=(const ProgramVisualizer&); // Do not implement
  ProgramVisualizer(const ProgramVisualizer&); // Do not implement

  std::string generateNormalizedString(const std::string& String); 
  std::string generateKernelColor(int Kernel, int NumKernel);
  int generateGraphInDot(std::string Path); 
public:
  /// Constructor
  ProgramVisualizer(ProgramDescriptor& PD_);

  /// Destructor
  ~ProgramVisualizer(); 

  /// Generate program graph
  int generateGraph(std::string Path); 
};   

} // End of danbi namespace

#endif 

