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

  IntrinsicsCPUDeprecated.h -- Deprecated intrinsics
 */
#ifndef DANBI_INTRINSICS_CPU_DEPRECATED_H
#define DANBI_INTRINSICS_CPU_DEPRECATED_H

#ifdef __cplusplus
extern "C" {
#endif

#define POPN(InputQueueIndex, NumElm, Data, Ticket) do {                \
    if (__pop(InputQueueIndex, NumElm, (void*)Data, 0, Ticket) < 0)     \
      return;                                                           \
  } while(0)

#define PUSHN(OutputQueueIndex, NumElm, Data, Ticket) do {              \
    __push(OutputQueueIndex, NumElm, (void*)Data, Ticket, 0);           \
  } while(0)


#ifdef __cplusplus
}
#endif
#endif 
