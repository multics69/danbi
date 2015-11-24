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

  BranchHint.h -- compiler specific branch hint
 */
#ifndef DANBI_BRANCH_HINT_H
#define DANBI_BRANCH_HINT_H

namespace danbi {

#ifdef __GNUC__
#define likely(x)       __builtin_expect((long int)(x),1)
#define unlikely(x)     __builtin_expect((long int)(x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

} // End of danbi namespace 

#endif 
