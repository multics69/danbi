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

  IntrinsicsCPUVanilla.h -- default unoptimized implementation of push/pop
 */
#define DECLARE_STRICTLY_REGULAR_POP_PUSH_RATIO(__POPR, __PUSHR)

#define DECLARE_BOUNDED_REGULAR_POP_PUSH_RATIO(__POPR, __PUSHR)

#define DECLARE_SYNCHRONOUSLY_REGULAR_POP_PUSH()

#define DECLARE_IRREGULAR_POP_PUSH()

#define DECLARE_INPUT_QUEUE(__Q, __T)                                   \
  __T __iq##__Q;                                                        \
  int __iq_ticket##__Q = 0

#define DECLARE_OUTPUT_QUEUE(__Q, __T)                                  \
  __T __oq##__Q;                                                        \
  int __oq_ticket##__Q = 0

#define BEGIN_KERNEL_BODY()                                             \
  {

#define END_KERNEL_BODY()                                               \
  }                                                                     \
__exit:

#define POP(__Q, __V) do {                                              \
    if (__pop(__Q, 1, (void*)&__iq##__Q,                                \
              0, &__iq_ticket##__Q) < 0)                                \
      goto __exit;                                                      \
    __V = &__iq##__Q;                                                   \
  } while(0)

#define POP_TICKET_INQ(__Q, __INQ, __V) do {                            \
    if (__pop(__Q, 1, (void*)&__iq##__Q,                                \
              __iq_ticket##__INQ, &__iq_ticket##__Q) < 0)               \
      goto __exit;                                                      \
    __V = &__iq##__Q;                                                   \
  } while(0)

#define POP_TICKET_OUTQ(__Q, __OUTQ, __V) do {                          \
    if (__pop(__Q, 1, (void*)&__iq##__Q,                                \
              __oq_ticket##__OUTQ, &__iq_ticket##__Q) < 0)              \
      goto __exit;                                                      \
    __V = &__iq##__Q;                                                   \
  } while(0)

#define CONSUME_POP_TICKET_INQ(__Q, __INQ) do {                         \
    __consume_pop_ticket(__Q, __iq_ticket##__Q);                        \
  } while(0)

#define CONSUME_POP_TICKET_OUTQ(__Q, __OUTQ) do {                       \
    __consume_pop_ticket(__Q, __oq_ticket##__Q);                        \
  } while(0)

#define PUSH_ADDRESS(__Q)                                               \
  (&__oq##__Q)

#define PUSH(__Q) do {                                                  \
    __push(__Q, 1, (void*)&__oq##__Q,                                   \
           0, &__oq_ticket##__Q);                                       \
  } while(0)

#define PUSH_TICKET_INQ(__Q, __INQ) do {                                \
    __push(__Q, 1, (void*)&__oq##__Q,                                   \
           __iq_ticket##__INQ, &__oq_ticket##__Q);                      \
  } while(0)

#define PUSH_TICKET_OUTQ(__Q, __OUTQ) do {                              \
    __push(__Q, 1, (void*)&__oq##__Q,                                   \
           __oq_ticket##__OUTQ, &__oq_ticket##__Q);                     \
  } while(0)

#define CONSUME_PUSH_TICKET_INQ(__Q, __INQ) do {                        \
    __consume_push_ticket(__Q, __iq_ticket##__INQ);                     \
  } while(0)

#define CONSUME_PUSH_TICKET_OUTQ(__Q, __OUTQ) do {                      \
    __consume_push_ticket(__Q, __oq_ticket##__OUTQ);                    \
  } while(0)

#define FLUSH_OUTPUT_QUEUE(__Q)
#define FLUSH_OUTPUT_QUEUE_TICKET_INQ(__Q, __INQ)
#define FLUSH_OUTPUT_QUEUE_TICKET_OUTQ(__Q, __OUTQ)


