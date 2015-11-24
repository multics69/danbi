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

  NullEventLogger.h -- Null event logger
 */
#ifndef DANBI_NULL_EVENT_LOGGER_H
#define DANBI_NULL_EVNET_LoGGER_H
#include <string>
#include <cerrno>
#include "Core/SchedulingEvent.h"

namespace danbi {

class NullEventLogger {
public:
  /// Initilize
  static inline int initialize(int NumKernels_, int NumCores_) {
    // Do nothing
    return 0; 
  }

  /// Append queue event
  static inline void appendQueueEvent(int Kernel, int Queue, 
                                      SchedulingEventKind Event, SchedulingEventKind OrgEvent) {
    // Do nothing
  }

  /// Append thread lifecycle event
  static inline void appendThreadLifeEvent(int Kernel, bool Create) {
    // Do nothing
  }

  /// Generate schedule graph
  static inline int generateGraph(std::string SimpleGraphPath, 
                                  std::string FullGraphPath) {
    // Do nothing
    return -ENOTSUP; 
  }
}; 

} // End of danbi namespace

#endif 
