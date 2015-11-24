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

  BackoffDelay.h -- randomized exponential backoff delay
 */
#ifndef DANBI_BACKOFF_DELAY_H
#define DANBI_BACKOFF_DELAY_H
#include <algorithm>
#include "Support/Random.h"

namespace danbi { 

class BackoffDelay {
public:
  static inline void delay(int Times) {
    if (Times == 0) return; 
    volatile long Count = 200000 << std::min(Times, 20) + (Random::randomInt() % 200000);
    for (volatile long i = 0; i < Count; ++i) ; 
  }
}; 


} // End of danbi namespace 

#endif 

