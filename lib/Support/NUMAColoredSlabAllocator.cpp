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

  NUMAColoredSlabAllocator.cpp -- slab allocator with NUMA coloring
 */
#include <cstdlib>
#include <cassert>
#include <sys/mman.h>
#include <numaif.h>
#include <math.h>
#include "Support/NUMAColoredSlabAllocator.h"

using namespace danbi; 

NUMAColoredSlabAllocator::NUMAColoredSlabAllocator(
  long NumNUMADomains_, long SlabSize_, long MaxSlabNum) 
  : DomChunks(NULL), DomIndex(0), 
    NumNUMADomains(NumNUMADomains_), SlabSize(SlabSize_) {
  SlabPerChunk = (MaxSlabNum + NumNUMADomains - 1) / NumNUMADomains; 
  ChunkSize = ::ceil(double(SlabPerChunk * SlabSize) / double(PageSize)) * PageSize;
  SlabPerChunk = ChunkSize / SlabSize; 
}

int NUMAColoredSlabAllocator::initialize() {
  DomChunks = (DomChunk*)::calloc(NumNUMADomains, sizeof(*DomChunks));
  if (DomChunks == NULL) 
    return -ENOMEM;

  for (int i = 0; i < NumNUMADomains; ++i) {
    // Allocate chunk 
    DomChunks[i].Chunk = (char*)::mmap(
      NULL, ChunkSize, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    if (DomChunks[i].Chunk == NULL) 
      return -ENOMEM;

    // Put it to a NUMA domain
    unsigned long NodeMask = 1UL << i; 
    if ( !::mbind(DomChunks[i].Chunk, ChunkSize, MPOL_BIND, 
                  &NodeMask, NumNUMADomains, MPOL_MF_MOVE_ALL) )
      return -errno;

    DomChunks[i].Start = (SlabPerChunk / NumNUMADomains) * i; 
    DomChunks[i].NumAlloc = 0;
  }

  return 0; 
}

NUMAColoredSlabAllocator::~NUMAColoredSlabAllocator() {
  if (DomChunks) {
    for (int i = 0; i < NumNUMADomains; ++i) {
      if (DomChunks[i].Chunk) 
        ::munmap(DomChunks[i].Chunk, ChunkSize);
    }
    ::free(DomChunks); 
  }
}

void * NUMAColoredSlabAllocator::alloc() {
  // Find a domain
  DomChunk* Dom = &DomChunks[DomIndex];
  if (Dom->NumAlloc >= SlabPerChunk) {
    assert(0); 
    return NULL;
  }
  DomIndex = (DomIndex + 1) % NumNUMADomains;

  // Calculate the allocated slab address
  char* Base = Dom->Chunk;
  int Index = (Dom->Start + Dom->NumAlloc++) % SlabPerChunk;
  char* Slab = Base + SlabSize * Index; 
  return Slab; 
}

void NUMAColoredSlabAllocator::free(void* Ptr) {
  // FIXME: do nothing
}
