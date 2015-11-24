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

  Intrinsics.h -- danbi C intrinsics can be called from danbi C kernel 
 */
#ifndef DANBI_INTRINSICS_H
#define DANBI_INTRINSICS_H
#include <errno.h>
#include <string.h>
#include "Support/ReserveCommitQueueAccessor.h"
#include "Support/Machine.h"
using namespace danbi;

#ifdef __cplusplus
extern "C" {
#endif
/** 
 * Types of optimzation 
 */
#define START_OF_DEFAULT_OPTIMIZATION "Core/IntrinsicsCPUDefaultOpt.h"
#define END_OF_DEFAULT_OPTIMIZATION "Core/IntrinsicsCPUEndOfOpt.h"

#ifdef DANBI_DISABLE_MULTIPLE_ITER_OPTIMIZATION
#define START_OF_MULTIPLE_ITER_OPTIMIZATION "Core/IntrinsicsCPUDefaultOpt.h"
#else
#define START_OF_MULTIPLE_ITER_OPTIMIZATION "Core/IntrinsicsCPUMultipleIterOpt.h"
#endif

#define END_OF_MULTIPLE_ITER_OPTIMIZATION "Core/IntrinsicsCPUEndOfOpt.h"

const static int DANBI_MULTIPLE_ITER_COUNT = 1000; 

/**
 * Read-only buffer intrinsics
 */
#define DECLARE_ROBX(__B, __T, __X)
#define DECLARE_ROBXY(__B, __T, __X, __Y)
#define DECLARE_ROBXYZ(__B, __T, __X, __Y)
#define ROBX(__B, __T, __X)           ((__T *)__robx(__B, __X))
#define ROBXY(__B, __T, __X, __Y)       ((__T *)__robxy(__B, __X, __Y))
#define ROBXYZ(__B, __T, __X, __Y, __Z)   ((__T *)__robxyz(__B, __X, __Y, __Z))

/**
 * Cache-line prefetch
 */ 
#define PREFETCH_NEXT(P) Machine::prefetch<char>(       \
    (char *)(((char*)(P)) + Machine::CachelineSize) )
#define PREFETCH_PREV(P) Machine::prefetch<char>(       \
    (char *)(((char*)(P)) + Machine::CachelineSize) )

/**
 * Intrinsics internal functions
 */
int __is_input_queue_empty(int InputQueueIndex);

int __reserve_pop(int InputQueueIndex, ReserveCommitQueueAccessor* Accessor, 
                  int NumElm, int NumRepeat, int ServingTicket, int* IssuingTicket);
void __commit_pop(int InputQueueIndex, ReserveCommitQueueAccessor* Accessor);

int __reserve_peek_pop(int InputQueueIndex, ReserveCommitQueueAccessor* Accessor, 
                       int PeekNumElm, int PopNumElm, int NumRepeat, 
                       int ServingTicket, int* IssuingTicket);
void __commit_peek_pop(int InputQueueIndex, ReserveCommitQueueAccessor* Accessor);

int __reserve_push(int OutputQueueIndex, ReserveCommitQueueAccessor* Accessor,
                   int NumElm, int ServingTicket, int* IssuingTicket);
void __commit_push(int OutputQueueIndex, ReserveCommitQueueAccessor* Accessor);

void __consume_push_ticket(int OutputQueueIndex, int Ticket);
void __consume_pop_ticket(int InputQueueIndex, int Ticket);

const void* __robx(int ROBIndex, int X); 
const void* __robxy(int ROBIndex, int X, int Y); 
const void* __robxyz(int ROBIndex, int X, int Y, int Z); 
#ifdef __cplusplus
}
#endif
#endif 
