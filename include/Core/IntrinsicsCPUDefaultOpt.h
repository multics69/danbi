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

  IntrinsicsCPUDefaultOpt.h -- Intrinsics with default optimization
 */
#include <cassert>

/** 
 * Define kernel body
 */ 
#define BEGIN_KERNEL_BODY()                     \
  {

#define END_KERNEL_BODY()                       \
  }                                             \
__exit:

/** 
 * Queue intrincis
 */ 
#define DECLARE_INPUT_QUEUE(__Q, __T)           \
  const int __iq##__Q = __Q;                    \
  ReserveCommitQueueAccessor __lliqa##__Q;      \
  int __iq_ticket##__Q

#define DECLARE_OUTPUT_QUEUE(__Q, __T)          \
  const int __oq##__Q = __Q;                    \
  ReserveCommitQueueAccessor __lloqa##__Q;      \
  int __oq_ticket##__Q

#define IS_VALID_INPUT_QUEUE(__Q) (__iq##__Q >= 0)

#define IS_VALID_OUTPUT_QUEUE(__Q) (__oq##__Q >= 0)

#define INPUT_QUEUE(__Q)                        \
  __iq##__Q, __lliqa##__Q, __iq_ticket##__Q

#define ARG_INPUT_QUEUE(__Q)                                            \
  const int __iq##__Q, ReserveCommitQueueAccessor &__lliqa##__Q, int & __iq_ticket##__Q

#define OUTPUT_QUEUE(__Q)                       \
  __oq##__Q, __lloqa##__Q, __oq_ticket##__Q

#define ARG_OUTPUT_QUEUE(__Q)                                           \
  const int __oq##__Q, ReserveCommitQueueAccessor &__lloqa##__Q, int & __oq_ticket##__Q

#define RESERVE_POP(__Q, __N) do {                              \
    if (__reserve_pop(__iq##__Q, &__lliqa##__Q,                 \
                      __N, 1, 0, &__iq_ticket##__Q) < 0)        \
      goto __exit;                                              \
    assert(__lliqa##__Q.RevInfo.__Num = __N);                   \
  } while (0)

#define TRY_RESERVE_POP(__Q, __N, __NR) do {                            \
    if ( (__NR = __reserve_pop(__iq##__Q, &__lliqa##__Q,                \
                               __N, __NR, 0, &__iq_ticket##__Q)) < 0)   \
      goto __exit;                                                      \
    assert(__lliqa##__Q.RevInfo.__Num = __N);                           \
  } while (0)

#define RESERVE_POP_TICKET_INQ(__Q, __INQ,  __N) do {   \
    if (__reserve_pop(__iq##__Q, &__lliqa##__Q,         \
                      __N, 1, __iq_ticket##__INQ,       \
                      &__iq_ticket##__Q) < 0)           \
      goto __exit;                                      \
    assert(__lliqa##__Q.RevInfo.__Num = __N);           \
  } while (0)

#define TRY_RESERVE_POP_TICKET_INQ(__Q, __INQ,  __N, __NR) do { \
    if ( (__NR = __reserve_pop(__iq##__Q, &__lliqa##__Q,        \
                               __N, __NR, __iq_ticket##__INQ,   \
                               &__iq_ticket##__Q)) < 0)         \
      goto __exit;                                              \
    assert(__lliqa##__Q.RevInfo.__Num = __N);                   \
  } while (0)

#define RESERVE_POP_TICKET_OUTQ(__Q, __OUTQ,  __N) do { \
    if (__reserve_pop(__iq##__Q, &__lliqa##__Q,         \
                      __N, 1, __oq_ticket##__OUTQ,      \
                      &__iq_ticket##__Q) < 0)           \
      goto __exit;                                      \
    assert(__lliqa##__Q.RevInfo.__Num = __N);           \
  } while (0)

#define TRY_RESERVE_POP_TICKET_OUTQ(__Q, __OUTQ,  __N, __NR) do {       \
    if ( (__NR = __reserve_pop(__iq##__Q, &__lliqa##__Q,                \
                               __N, __NR, __oq_ticket##__OUTQ,          \
                               &__iq_ticket##__Q)) < 0)                 \
      goto __exit;                                                      \
    assert(__lliqa##__Q.RevInfo.__Num = __N);                           \
  } while (0)

#define COMMIT_POP(__Q) do {                    \
    __commit_pop(__iq##__Q, &__lliqa##__Q);     \
  } while (0)

#define POP_ADDRESS_AT(__Q, __T, __I)                   \
  (__T *)(__lliqa##__Q).getElementAt(sizeof(__T), __I)

#define RESERVE_PEEK_POP(__Q, __PEEKN, __POPN) do {                     \
    if (__reserve_peek_pop(__iq##__Q, &__lliqa##__Q,                    \
                           __PEEKN, __POPN, 1, 0, &__iq_ticket##__Q) < 0) \
      goto __exit;                                                      \
    assert(__lliqa##__Q.RevInfo.__Num = __PEEKN);                       \
  } while (0)

#define RESERVE_PEEK_POP_TICKET_INQ(__Q, __INQ, __PEEKN, __POPN) do {   \
    if (__reserve_peek_pop(__iq##__Q, &__lliqa##__Q,                    \
                           __PEEKN, __POPN, 1, __iq_ticket##__INQ,      \
                           &__iq_ticket##__Q) < 0)                      \
      goto __exit;                                                      \
    assert(__lliqa##__Q.RevInfo.__Num = __PEEKN);                       \
  } while (0)

#define RESERVE_PEEK_POP_TICKET_OUTQ(__Q, __OUTQ, __PEEKN, __POPN) do { \
    if (__reserve_peek_pop(__iq##__Q, &__lliqa##__Q,                    \
                           __PEEKN, __POPN, 1, __oq_ticket##__OUTQ,     \
                           &__iq_ticket##__Q) < 0)                      \
      goto __exit;                                                      \
    assert(__lliqa##__Q.RevInfo.__Num = __PEEKN);                       \
  } while (0)

#define COMMIT_PEEK_POP(__Q) do {                       \
    __commit_peek_pop(__iq##__Q, &__lliqa##__Q);        \
  } while (0)

#define PEEK_POP_ADDRESS_AT(__Q, __T, __I)              \
  (__T *)(__lliqa##__Q).getElementAt(sizeof(__T), __I)

#define RESERVE_PUSH(__Q, __N) do {                     \
    if (__reserve_push(__oq##__Q, &__lloqa##__Q,        \
                       __N, 0, &__oq_ticket##__Q) < 0)  \
      goto __exit;                                      \
    assert(__lloqa##__Q.RevInfo.__Num = __N);           \
  } while (0)

#define RESERVE_PUSH_TICKET_INQ(__Q, __INQ, __N) do {                   \
    if (__reserve_push(__oq##__Q, &__lloqa##__Q, __N,                   \
                       __iq_ticket##__INQ, &__oq_ticket##__Q) < 0)      \
      goto __exit;                                                      \
    assert(__lloqa##__Q.RevInfo.__Num = __N);                           \
  } while (0)

#define RESERVE_PUSH_TICKET_OUTQ(__Q, __OUTQ, __N) do {                 \
    if (__reserve_push(__oq##__Q, &__lloqa##__Q, __N,                   \
                       __oq_ticket##__OUTQ, &__oq_ticket##__Q) < 0)     \
      goto __exit;                                                      \
    assert(__lloqa##__Q.RevInfo.__Num = __N);                           \
  } while (0)

#define COMMIT_PUSH(__Q) do {                   \
    __commit_push(__oq##__Q, &__lloqa##__Q);    \
  } while (0)

#define PUSH_ADDRESS_AT(__Q, __T, __I)                  \
  (__T *)(__lloqa##__Q).getElementAt(sizeof(__T), __I)

#define COPY_FROM_MEM(__Q, __T, __I, __N, __S)                  \
  (__lloqa##__Q).copyFromMem(sizeof(__T), __I, __N, __S)

#define COPY_TO_MEM(__Q, __T, __I, __N, __D)            \
  (__lliqa##__Q).copyToMem(sizeof(__T), __I, __N, __D)

#define COPY_QUEUE(__OQ, __OT, __OI, __ON, __IQ, __II)                  \
  (__lloqa##__OQ).copy(sizeof(__OT), __OI, __ON, __lliqa##__IQ, __II)

#define GET_TICKET_INQ(__INQ)                   \
  __iq_ticket##__INQ

#define GET_TICKET_OUTQ(__OUTQ)                 \
  __oq_ticket##__OUTQ

#define CONSUME_POP_TICKET_INQ(__Q, __INQ) do {                 \
    __consume_pop_ticket(__iq##__Q, __iq_ticket##__INQ);        \
  } while(0)

#define CONSUME_POP_TICKET_OUTQ(__Q, __OUTQ) do {               \
    __consume_pop_ticket(__iq##__Q, __oq_ticket##__OUTQ);       \
  } while(0)

#define CONSUME_PUSH_TICKET_INQ(__Q, __INQ) do {                \
    __consume_push_ticket(__oq##__Q, __iq_ticket##__INQ);       \
  } while(0)

#define CONSUME_PUSH_TICKET_OUTQ(__Q, __OUTQ) do {              \
    __consume_push_ticket(__oq##__Q, __oq_ticket##__OUTQ);      \
  } while(0)

