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

  IntrinsicsMultipleIterOpt.h -- Intrinsics with multiple iteration optimization
 */
#include <cassert>

/** 
 * Define kernel body
 */ 
#define BEGIN_KERNEL_BODY()                     \
  int __num_repeat = DANBI_MULTIPLE_ITER_COUNT; \
  int __cur_repeat = 0;                         \
  do {                                                                  

#define END_KERNEL_BODY()                       \
  __cur_repeat++;                               \
  } while(__cur_repeat < __num_repeat);         \
__exit:

/** 
 * Queue intrincis
 */ 
#define DECLARE_INPUT_QUEUE(__Q, __T)           \
  const int __iq##__Q = __Q;                    \
  ReserveCommitQueueAccessor __lliqa##__Q;      \
  int __iq_ticket##__Q, __iq_num_elm##__Q

#define DECLARE_OUTPUT_QUEUE(__Q, __T)          \
  const int __oq##__Q = __Q;                    \
  ReserveCommitQueueAccessor __lloqa##__Q;      \
  int __oq_ticket##__Q, __oq_num_elm##__Q

#define IS_VALID_INPUT_QUEUE(__Q) (__iq##__Q >= 0)

#define IS_VALID_OUTPUT_QUEUE(__Q) (__oq##__Q >= 0)

#define INPUT_QUEUE(__Q)                                                \
  __iq##__Q, __cur_repeat, __iq_num_elm##__Q, __lliqa##__Q, __iq_ticket##__Q

#define ARG_INPUT_QUEUE(__Q)                                            \
  const int __iq##__Q, int __cur_repeat, int __iq_num_elm##__Q,         \
    ReserveCommitQueueAccessor &__lliqa##__Q, int & __iq_ticket##__Q

#define OUTPUT_QUEUE(__Q)                                               \
  __oq##__Q, __cur_repeat, __oq_num_elm##__Q, __lloqa##__Q, __oq_ticket##__Q

#define ARG_OUTPUT_QUEUE(__Q)                                           \
  const int __oq##__Q, int __cur_repeat, int __oq_num_elm##__Q,         \
    ReserveCommitQueueAccessor &__lloqa##__Q, int & __oq_ticket##__Q

#define RESERVE_POP(__Q, __N) do {                                      \
    if (__cur_repeat == 0) {                                            \
      __lliqa##__Q.RevInfo.__Num = __reserve_pop(__iq##__Q, &__lliqa##__Q, \
                                                 __N, __num_repeat,     \
                                                 0, &__iq_ticket##__Q); \
      if (__lliqa##__Q.RevInfo.__Num < 0)                               \
        goto __exit;                                                    \
      __num_repeat = __lliqa##__Q.RevInfo.__Num / (__N);                \
      __iq_num_elm##__Q = __N;                                          \
    }                                                                   \
  } while (0)

#define TRY_RESERVE_POP(__Q, __N, __NR) do {                            \
    if (__cur_repeat == 0) {                                            \
      __lliqa##__Q.RevInfo.__Num = __reserve_pop(__iq##__Q, &__lliqa##__Q, \
                                                 __N, __num_repeat,     \
                                                 0, &__iq_ticket##__Q); \
      if (__lliqa##__Q.RevInfo.__Num < 0)                               \
        goto __exit;                                                    \
      __num_repeat = __lliqa##__Q.RevInfo.__Num / (__N);                \
      __iq_num_elm##__Q = __N;                                          \
    }                                                                   \
  } while (0)

#define RESERVE_POP_TICKET_INQ(__Q, __INQ,  __N) do {                   \
    if (__cur_repeat == 0) {                                            \
      __lliqa##__Q.RevInfo.__Num = __reserve_pop(__iq##__Q, &__lliqa##__Q, \
                                                 __N, __num_repeat,     \
                                                 __iq_ticket##__INQ,    \
                                                 &__iq_ticket##__Q);    \
      if (__lliqa##__Q.RevInfo.__Num < 0)                               \
        goto __exit;                                                    \
      __num_repeat = __lliqa##__Q.RevInfo.__Num / (__N);                \
      __iq_num_elm##__Q = __N;                                          \
    }                                                                   \
  } while (0)

#define TRY_RESERVE_POP_TICKET_INQ(__Q, __INQ, __N, __NR) do {          \
    if (__cur_repeat == 0) {                                            \
      __lliqa##__Q.RevInfo.__Num = __reserve_pop(__iq##__Q, &__lliqa##__Q, \
                                                 __N, __num_repeat,     \
                                                 __iq_ticket##__INQ,    \
                                                 &__iq_ticket##__Q);    \
      if (__lliqa##__Q.RevInfo.__Num < 0)                               \
        goto __exit;                                                    \
      __num_repeat = __lliqa##__Q.RevInfo.__Num / (__N);                \
      __iq_num_elm##__Q = __N;                                          \
    }                                                                   \
  } while (0)

#define RESERVE_POP_TICKET_OUTQ(__Q, __OUTQ, __N) do {                  \
    if (__cur_repeat == 0) {                                            \
      __lliqa##__Q.RevInfo.__Num = __reserve_pop(__iq##__Q, &__lliqa##__Q, \
                                                 __N, __num_repeat,     \
                                                 __oq_ticket##__OUTQ,   \
                                                 &__iq_ticket##__Q);    \
      if (__lliqa##__Q.RevInfo.__Num < 0)                               \
        goto __exit;                                                    \
      __num_repeat = __lliqa##__Q.RevInfo.__Num / (__N);                \
      __iq_num_elm##__Q = __N;                                          \
    }                                                                   \
  } while (0)

#define TRY_RESERVE_POP_TICKET_OUTQ(__Q, __OUTQ, __N, __NR) do {        \
    if (__cur_repeat == 0) {                                            \
      __lliqa##__Q.RevInfo.__Num = __reserve_pop(__iq##__Q, &__lliqa##__Q, \
                                                 __N, __num_repeat,     \
                                                 __oq_ticket##__OUTQ,   \
                                                 &__iq_ticket##__Q);    \
      if (__lliqa##__Q.RevInfo.__Num < 0)                               \
        goto __exit;                                                    \
      __num_repeat = __lliqa##__Q.RevInfo.__Num / (__N);                \
      __iq_num_elm##__Q = __N;                                          \
    }                                                                   \
  } while (0)

#define COMMIT_POP(__Q) do {                    \
    if ((__cur_repeat+1) == __num_repeat)       \
      __commit_pop(__iq##__Q, &__lliqa##__Q);   \
  } while (0)

#define POP_ADDRESS_AT(__Q, __T, __I)                           \
  (__T *)(__lliqa##__Q).getElementAt(                           \
    sizeof(__T), (__I) + (__cur_repeat * __iq_num_elm##__Q))

#define RESERVE_PEEK_POP(__Q, __PEEKN, __POPN) do {             \
    if (__cur_repeat == 0) {                                    \
      __lliqa##__Q.RevInfo.__Num =                              \
        __reserve_peek_pop(__iq##__Q, &__lliqa##__Q,            \
                           __PEEKN, __POPN, __num_repeat,       \
                           0, &__iq_ticket##__Q);               \
      if (__lliqa##__Q.RevInfo.__Num < 0)                       \
        goto __exit;                                            \
      __num_repeat = __lliqa##__Q.RevInfo.__Num / (__POPN);     \
      __iq_num_elm##__Q = __POPN;                               \
    }                                                           \
  } while (0)

#define RESERVE_PEEK_POP_TICKET_INQ(__Q, __INQ, __PEEKN, __POPN) do {   \
    if (__cur_repeat == 0) {                                            \
      __lliqa##__Q.RevInfo.__Num =                                      \
        __reserve_peek_pop(__iq##__Q, &__lliqa##__Q,                    \
                           __PEEKN, __POPN, __num_repeat,               \
                           __iq_ticket##__INQ,                          \
                           &__iq_ticket##__Q);                          \
      if (__lliqa##__Q.RevInfo.__Num < 0)                               \
        goto __exit;                                                    \
      __num_repeat = __lliqa##__Q.RevInfo.__Num / (__POPN);             \
      __iq_num_elm##__Q = __POPN;                                       \
    }                                                                   \
  } while (0)

#define RESERVE_PEEK_POP_TICKET_OUTQ(__Q, __OUTQ, __PEEKN, __POPN) do { \
    if (__cur_repeat == 0) {                                            \
      __lliqa##__Q.RevInfo.__Num =                                      \
        __reserve_peek_pop(__iq##__Q, &__lliqa##__Q,                    \
                           __PEEKN, __POPN, __num_repeat,               \
                           __oq_ticket##__OUTQ,                         \
                           &__iq_ticket##__Q);                          \
      if (__lliqa##__Q.RevInfo.__Num < 0)                               \
        goto __exit;                                                    \
      __num_repeat = __lliqa##__Q.RevInfo.__Num / (__POPN);             \
      __iq_num_elm##__Q = __POPN;                                       \
    }                                                                   \
  } while (0)

#define COMMIT_PEEK_POP(__Q) do {                       \
    if ((__cur_repeat+1) == __num_repeat)               \
      __commit_peek_pop(__iq##__Q, &__lliqa##__Q);      \
  } while (0)

#define PEEK_POP_ADDRESS_AT(__Q, __T, __I)                      \
  (__T *)(__lliqa##__Q).getElementAt(                           \
    sizeof(__T), (__I) + (__cur_repeat * __iq_num_elm##__Q))

#define RESERVE_PUSH(__Q, __N) do {                             \
    if (__cur_repeat == 0) {                                    \
      __lloqa##__Q.RevInfo.__Num = (__N) * __num_repeat;        \
      if (__reserve_push(__oq##__Q, &__lloqa##__Q,              \
                         __lloqa##__Q.RevInfo.__Num,            \
                         0, &__oq_ticket##__Q) < 0)             \
        goto __exit;                                            \
      __oq_num_elm##__Q = __N;                                  \
    }                                                           \
  } while (0)

#define RESERVE_PUSH_TICKET_INQ(__Q, __INQ, __N) do {                   \
    if (__cur_repeat == 0) {                                            \
      __lloqa##__Q.RevInfo.__Num = (__N) * __num_repeat;                \
      if (__reserve_push(__oq##__Q, &__lloqa##__Q,                      \
                         __lloqa##__Q.RevInfo.__Num,                    \
                         __iq_ticket##__INQ, &__oq_ticket##__Q) < 0)    \
        goto __exit;                                                    \
      __oq_num_elm##__Q = __N;                                          \
    }                                                                   \
  } while (0)

#define RESERVE_PUSH_TICKET_OUTQ(__Q, __OUTQ, __N) do {                 \
    if (__cur_repeat == 0) {                                            \
      __lloqa##__Q.RevInfo.__Num = (__N) * __num_repeat;                \
      if (__reserve_push(__oq##__Q, &__lloqa##__Q,                      \
                         __lloqa##__Q.RevInfo.__Num,                    \
                         __oq_ticket##__OUTQ, &__oq_ticket##__Q) < 0)   \
        goto __exit;                                                    \
      __oq_num_elm##__Q = __N;                                          \
    }                                                                   \
  } while (0)

#define COMMIT_PUSH(__Q) do {                   \
    if ((__cur_repeat+1) == __num_repeat)       \
      __commit_push(__oq##__Q, &__lloqa##__Q);  \
  } while (0)

#define PUSH_ADDRESS_AT(__Q, __T, __I)                          \
  (__T *)(__lloqa##__Q).getElementAt(                           \
    sizeof(__T), (__I) + (__cur_repeat * __oq_num_elm##__Q))

#define COPY_FROM_MEM(__Q, __T, __I, __N, __S) do {             \
    int __idx = (__I) + (__cur_repeat * __oq_num_elm##__Q);     \
    assert(0 <= __idx && __idx < __lloqa##__Q.RevInfo.__Num);   \
    assert(0 <= (__idx + (__N)));                               \
    assert((__idx + (__N)) <= __lloqa##__Q.RevInfo.__Num);      \
    (__lloqa##__Q).copyFromMem(sizeof(__T), __idx, __N, __S);   \
  } while (0)

#define COPY_TO_MEM(__Q, __T, __I, __N, __D) do {               \
    int __idx = (__I) + (__cur_repeat * __iq_num_elm##__Q);     \
    assert(0 <= __idx && __idx < __lliqa##__Q.RevInfo.__Num);   \
    assert(0 <= (__idx + (__N)));                               \
    assert((__idx + (__N)) <= __lliqa##__Q.RevInfo.__Num);      \
    (__lliqa##__Q).copyToMem(sizeof(__T), __idx, __N, __D);     \
  } while (0)

#define COPY_QUEUE(__OQ, __OT, __OI, __ON, __IQ, __II) do {             \
    int __oidx = (__OI) + (__cur_repeat * __oq_num_elm##__OQ);          \
    (__lloqa##__OQ).copy(                                               \
      sizeof(__OT), __oidx, __ON,                                       \
      __lliqa##__IQ, (__II) + (__cur_repeat * __iq_num_elm##__IQ));     \
  } while (0)

#define GET_TICKET_INQ(__INQ)                                   \
  00_ERROR_not_supported_in_multiple_iteration_optimization

#define GET_TICKET_OUTQ(__OUTQ)                                 \
  00_ERROR_not_supported_in_multiple_iteration_optimization

#define CONSUME_POP_TICKET_INQ(__Q, __INQ) do {                 \
    if ((__cur_repeat+1) == __num_repeat)                       \
      __consume_pop_ticket(__iq##__Q, __iq_ticket##__INQ);      \
  } while(0)

#define CONSUME_POP_TICKET_OUTQ(__Q, __OUTQ) do {               \
    if ((__cur_repeat+1) == __num_repeat)                       \
      __consume_pop_ticket(__iq##__Q, __oq_ticket##__OUTQ);     \
  } while(0)

#define CONSUME_PUSH_TICKET_INQ(__Q, __INQ) do {                \
    if ((__cur_repeat+1) == __num_repeat)                       \
      __consume_push_ticket(__oq##__Q, __iq_ticket##__INQ);     \
  } while(0)

#define CONSUME_PUSH_TICKET_OUTQ(__Q, __OUTQ) do {              \
    if ((__cur_repeat+1) == __num_repeat)                       \
      __consume_push_ticket(__oq##__Q, __oq_ticket##__OUTQ);    \
  } while(0)

