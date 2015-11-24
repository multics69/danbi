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

  IntrinsicsCPUSubQueueOptimization.h -- push/pop intrisics with sub-queue optimization 
 */

/// All input queueus are cosumed in the same rate, 
/// and all output queues are also produced in the same rate. 
/// : pop() buffering and ticketing are possible.
#define DECLARE_STRICTLY_REGULAR_POP_PUSH_RATIO(__POPR, __PUSHR)        \
  enum {                                                                \
    __iq_size = 32 * (__POPR),                                          \
    __oq_size = 32 * (__PUSHR),                                         \
    __iqt_size = 32 * (__POPR),                                         \
    __oqt_size = 32 * (__PUSHR),                                        \
  }

/// All input queues are consumed in the same rate, 
/// however te production rate of output queues could be different. 
/// But its upper limit of production rates is known. 
/// : pop() buffering and ticketing are possible.
#define DECLARE_BOUNDED_REGULAR_POP_PUSH_RATIO(__POPR, __PUSHR)         \
  enum {                                                                \
    __iq_size = 32 * (__POPR),                                          \
    __oq_size = 32 * (__PUSHR),                                         \
    __iqt_size = 32 * (__POPR),                                         \
    __oqt_size = 32 * (__PUSHR),                                        \
  }

/// All input queues are consumed in the same rate, 
/// however the production rate of output queues could be different. 
/// Moreover, its upper limit of production rates is unknown. 
/// : pop() buffering is possible. Ticketing is not possible. 
#define DECLARE_SYNCHRONOUSLY_REGULAR_POP_PUSH()                        \
  enum {                                                                \
    __iq_size = 32,                                                     \
    __oq_size = 32,                                                     \
    __iqt_size = -1,                                                    \
    __oqt_size = -1,                                                    \
  }

/// All input queues are NOT consumed in the same rate.
/// : pop() buffering and ticketing are NOT possible.
#define DECLARE_IRREGULAR_POP_PUSH()                                    \
  enum {                                                                \
    __iq_size = 1,                                                      \
    __oq_size = 32,                                                     \
    __iqt_size = -1,                                                    \
    __oqt_size = -1,                                                    \
  }

#define DECLARE_INPUT_QUEUE(__Q, __T)                                   \
  __T __iq##__Q[__iq_size];                                             \
  int __iq_head##__Q = 0, __iq_tail##__Q = 0;                           \
  int __iq_ticket##__Q

#define DECLARE_OUTPUT_QUEUE(__Q, __T)                                  \
  __T __oq##__Q[__oq_size];                                             \
  int __oq_head##__Q = 0;                                               \
  int __oq_ticket##__Q, __oq_committed##__Q = 0

#define BEGIN_KERNEL_BODY()                                             \
  int __iq_data_num = 0;                                                \
  do {

#define END_KERNEL_BODY()                                               \
  } while (__iq_data_num);                                              \
__exit:

#define POP(__Q, __V) do {                                              \
    if (__iq_head##__Q == __iq_tail##__Q) {                             \
      __iq_tail##__Q = __pop(__Q, __iq_size, (void*)__iq##__Q,          \
                             0, &__iq_ticket##__Q);                     \
      if (__iq_tail##__Q < 0) goto __exit;                              \
      __iq_head##__Q = 0;                                               \
      if (__iq_data_num < __iq_tail##__Q)                               \
        __iq_data_num = __iq_tail##__Q;                                 \
    }                                                                   \
    __V = __iq##__Q + __iq_head##__Q;                                   \
    ++__iq_head##__Q;                                                   \
    if ( __iq_data_num == (__iq_tail##__Q - __iq_head##__Q) )           \
      --__iq_data_num;                                                  \
  } while(0)

#define POP_TICKET_INQ(__Q, __INQ, __V) do {                            \
    if (__iq_head##__Q == __iq_tail##__Q) {                             \
      __iq_tail##__Q = __pop(__Q, __iq_size, (void*)__iq##__Q,          \
                             __iq_ticket##__INQ, &__iq_ticket##__Q);    \
      if (__iq_tail##__Q < 0) goto __exit;                              \
      __iq_head##__Q = 0;                                               \
      if (__iq_data_num < __iq_tail##__Q)                               \
      __iq_data_num = __iq_tail##__Q;                                   \
    }                                                                   \
    __V = __iq##__Q + __iq_head##__Q;                                   \
    ++__iq_head##__Q;                                                   \
    if ( __iq_data_num == (__iq_tail##__Q - __iq_head##__Q) )           \
      --__iq_data_num;                                                  \
  } while(0)

#define POP_TICKET_OUTQ(__Q, __OUTQ, __V) do {                          \
    if (__iq_head##__Q == __iq_tail##__Q) {                             \
      __iq_tail##__Q = __pop(__Q, __iq_size, (void*)__iq##__Q,          \
                             __oq_ticket##__OUTQ, &__iq_ticket##__Q);   \
      if (__iq_tail##__Q < 0) goto __exit;                              \
      __iq_head##__Q = 0;                                               \
      if (__iq_data_num < __iq_tail##__Q)                               \
        __iq_data_num = __iq_tail##__Q;                                 \
    }                                                                   \
    __V = __iq##__Q + __iq_head##__Q;                                   \
    ++__iq_head##__Q;                                                   \
    if ( __iq_data_num == (__iq_tail##__Q - __iq_head##__Q) )           \
      --__iq_data_num;                                                  \
  } while(0)

#define CONSUME_POP_TICKET_INQ(__Q, __INQ) do {                         \
    if (__iq_head##__Q == __iq_tail##__Q)                               \
      __consume_pop_ticket(__Q, __iq_ticket##__INQ);                    \
  } while(0)

#define CONSUME_POP_TICKET_OUTQ(__Q, __OUTQ) do {                       \
    if (__iq_head##__Q == __iq_tail##__Q)                               \
      __consume_pop_ticket(__Q, __oq_ticket##__OUTQ);                   \
  } while(0)

#define PUSH_ADDRESS(__Q)                                               \
  (__oq##__Q + __oq_head##__Q)

#define PUSH(__Q) do {                                                  \
    assert(__oq_head##__Q < __oq_size);                                 \
    __oq_head##__Q++;                                                   \
    if (__oq_head##__Q == __oq_size) {                                  \
      __push(__Q, __oq_size, (void*)__oq##__Q,                          \
             0, &__oq_ticket##__Q);                                     \
      __oq_head##__Q = 0;                                               \
      __oq_committed##__Q = 1;                                          \
    }                                                                   \
    else                                                                \
      __oq_committed##__Q = 0;                                          \
  } while(0)


#define PUSH_TICKET_INQ(__Q, __INQ) do {                                \
    extern int __x[__iqt_size];                                         \
    assert(__oq_head##__Q < __oq_size);                                 \
    __oq_head##__Q++;                                                   \
    if (__iq_head##__INQ == __iq_tail##__INQ) {                         \
      __push(__Q, __oq_head##__Q, (void*)__oq##__Q,                     \
             __iq_ticket##__INQ, &__oq_ticket##__Q);                    \
      __oq_head##__Q = 0;                                               \
      __oq_committed##__Q = 1;                                          \
    }                                                                   \
    else                                                                \
      __oq_committed##__Q = 0;                                          \
  } while(0)


#define PUSH_TICKET_OUTQ(__Q, __OUTQ) do {                              \
    extern int __x[__oqt_size];                                         \
    assert(__oq_head##__Q < __oq_size);                                 \
    __oq_head##__Q++;                                                   \
    if (__oq_committed##__OUTQ) {                                       \
      __push(__Q, __oq_head##__Q, (void*)__oq##__Q,                     \
             __iq_ticket##__OUTQ, &__oq_ticket##__Q);                   \
      __oq_head##__Q = 0;                                               \
      __oq_committed##__Q = 1;                                          \
    }                                                                   \
    else                                                                \
      __oq_committed##__Q = 0;                                          \
  } while(0)

#define CONSUME_PUSH_TICKET_INQ(__Q, __INQ) do {                        \
    if (__oq_head##__Q == 0)                                            \
      __consume_push_ticket(__Q, __iq_ticket##__INQ);                   \
  } while(0)

#define CONSUME_PUSH_TICKET_OUTQ(__Q, __OUTQ) do {                      \
    if (__oq_head##__Q == 0)                                            \
      __consume_push_ticket(__Q, __oq_ticket##__OUTQ);                  \
  } while(0)

#define FLUSH_OUTPUT_QUEUE(__Q) do {                                    \
    if (!__oq_committed##__Q && __oq_head##__Q)                         \
      __push(__Q, __oq_head##__Q, (void*)__oq##__Q,                     \
             0, &__oq_ticket##__Q);                                     \
  } while(0)

#define FLUSH_OUTPUT_QUEUE_TICKET_INQ(__Q, __INQ) do {                  \
    extern int __x[__iqt_size];                                         \
    if (!__oq_committed##__Q) {                                         \
      if (__oq_head##__Q)                                               \
        __push(__Q, __oq_head##__Q, (void*)__oq##__Q,                   \
               __iq_ticket##__INQ, &__oq_ticket##__Q);                  \
    }                                                                   \
  } while(0)

#define FLUSH_OUTPUT_QUEUE_TICKET_OUTQ(__Q, __OUTQ) do {                \
    extern int __x[__oqt_size];                                         \
    if (!__oq_committed##__Q) {                                         \
      if (__oq_head##__Q)                                               \
        __push(__Q, __oq_head##__Q, (void*)__oq##__Q,                   \
               __oq_ticket##__OUTQ, &__oq_ticket##__Q);                 \
    }                                                                   \
  } while(0)
