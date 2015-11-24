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

  NullBufferManager.h -- Null buffer manager for circular quque. 
  It is for CPU memory and does not provide double buffering. 
 */

#ifndef DANBI_NULL_BUFFER_MANAGER_H
#define DANBI_NULL_BUFFER_MANAGER_H
#include <cassert>
#include <cstdlib>

namespace danbi {
class NullBufferManager {
private:
  long NumElm; 
  double* Buff; 

  void operator=(const NullBufferManager&); // Do not implement
  NullBufferManager(const NullBufferManager&); // Do not implement 
public:
  /// constructor 
  NullBufferManager(long ElmSize_, long NumElm_) 
    : NumElm(NumElm_), 
      Buff(new double[((NumElm*ElmSize_) + sizeof(double) - 1) / 
                      sizeof(double)]) {
#ifndef DANBI_DO_NOT_PREVENT_CONCURRENT_PAGE_FAULTS
    for (long i = 0; i < (NumElm_ * ElmSize_); i += 2048)
      Buff[i/sizeof(double)] = 0.0; 
#endif 
  }

  /// destructor
  virtual ~NullBufferManager() {
    delete [] Buff; 
  }

  /// get the initial buffer
  inline void* getInitialBuffer() {
    return Buff; 
  }
};

} // End of danbi namespace

#endif 
