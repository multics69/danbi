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

  AbstractGreenRunnalbe.h -- Abstract runnable class
 */
#ifndef DANBI_ABSTRACT_GREEN_RUNNABLE_H
#define DANBI_ABSTRACT_GREEN_RUNNABLE_H
#include "Support/AbstractRunnable.h"

namespace danbi {
#define DANBI_GREEN_RUNNABLE_YIELDABLE(GreenSchedulerTy) private:             \
  inline void yield() {                                                       \
    GreenSchedulerTy::getInstance().yield(*GreenSchedulerTy::RunningThread);  \
  }
  
template <typename GreenSchedulerTy>
class AbstractGreenRunnable : virtual public AbstractRunnable {
  DANBI_GREEN_RUNNABLE_YIELDABLE(GreenSchedulerTy);
}; 

} // End of danbi namespace 

#endif 
