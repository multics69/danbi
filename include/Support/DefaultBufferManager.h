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

  DefaultBufferManager.h -- placeholder of buffer manager
 */
#ifndef DANBI_DEFAULT_BUFFER_MANAGER_H
#define DANBI_DEFAULT_BUFFER_MANAGER_H
#include "Support/NullBufferManager.h"
#include "Support/ResizableBufferManager.h"

namespace danbi {
#ifdef DANBI_USE_RESIZABLE_BUFFER_MANAGER_BY_DEFAULT
typedef ResizableBufferManager DefaultBufferManager;
#else
typedef NullBufferManager DefaultBufferManager; 
#endif
}


#endif 
