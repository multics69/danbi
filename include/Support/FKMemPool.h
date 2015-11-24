/*                                                                 --*- C++ -*-
  Copyright (C) 2013 Changwoo Min. All Rights Reserved.

  This file is part of DANBI project. 

  NOTICE: All information contained herein is, and remains the property 
  of Changwoo Min. The intellectual and technical concepts contained 
  herein are proprietary to Changwoo Min and may be covered  by patents 
  or patents in process, and are protected by trade secret or copyright law. 
  Dissemination of this information or reproduction of this material is 
  strictly forbidden unless prior written permission is obtained 
  from Changwoo Min(multics69@gmail.com). 

  FKMemPool.h -- memory pool implementation inspired from the work of 
  Fatourou and Kallimanis, PPoPP 2012. But, it inherently has memory leak. 
  ** Never use for a production purpose. **
 */
#ifndef DANBI_FKMEM_POOL_H
#define DANBI_FKMEM_POOL_H
#include <cassert>
#include <malloc.h>
#include "Support/Machine.h"
#include "Support/BranchHint.h"

namespace danbi {

class FKMemPool {
private:
  void **Pool;
  int Index; 
  int ObjSize; 
  int PoolSize;
  bool AllocPage; 

  void operator=(const FKMemPool&); // Do not implement
  FKMemPool(const FKMemPool&); // Do not implement

  inline void init() {
    char* objects = (char*)::memalign(Machine::CachelineSize, PoolSize * ObjSize);
    Index = 0;
    for (int i = 0; i < PoolSize; i++)
      Pool[i] = (void *)(objects + (int)(i * ObjSize));
    // Note: if there was already an allocated chunk, simply ignore. 

    if (AllocPage) {
      for (int i = 0; i < (PoolSize*ObjSize); i += 2048)
        objects[i] = '\0';
    }
  }

public:
  FKMemPool(int ObjSize_, int PoolSize_, bool AllocPage_)
    : ObjSize(ObjSize_), PoolSize(PoolSize_), AllocPage(AllocPage_) {
    Pool = (void**)::malloc(sizeof(*Pool) * PoolSize);
    init();
  }

  ~FKMemPool() {
    ::free(Pool);
    // Note: Never ever free anything.
  }

  inline void *malloc() {
    if (unlikely(Index == PoolSize))
      init();
    void * obj = Pool[Index++];
    assert(obj != NULL);
    return obj;
  }

  inline void free(void *obj) {
    assert(obj != NULL);
    if (likely(Index > 0))
      Pool[--Index] = obj;
    // Note: giving up freeing the obj make it a dangling reference. 
  }
};

} // End of danbi name space

#endif 
