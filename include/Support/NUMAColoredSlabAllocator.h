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

  NUMAColoredSlabAllocator.h -- Slab allocator with NUMA coloring
 */
#ifndef DANBI_NUMA_COLORED_SLAB_ALLOCATOR_H
#define DANBI_NUMA_COLORED_SLAB_ALLOCATOR_H
#include <cerrno>

namespace danbi {

class NUMAColoredSlabAllocator {
private:
  enum {
    PageSize = 4096,
  }; 

  struct DomChunk {
    char *Chunk; 
    long Start; 
    long NumAlloc;
  }; 

  DomChunk* DomChunks;
  int DomIndex; 
  long NumNUMADomains; 
  long SlabSize;
  long SlabPerChunk;
  long ChunkSize; 

  void operator=(const NUMAColoredSlabAllocator&); // Do not implement
  NUMAColoredSlabAllocator(const NUMAColoredSlabAllocator&); // Do not implement

public:
  /// Constructor 
  NUMAColoredSlabAllocator(long NumNUMADomains_, long SlabSize_, long MaxSlabNum_); 

  /// Destructor 
  ~NUMAColoredSlabAllocator(); 

  /// Initialize
  int initialize();

  /// Allocate a slab
  void *alloc(); 

  /// Free a slab
  void free(void* Ptr);
}; 

} // End of danbi namespace

#endif 
