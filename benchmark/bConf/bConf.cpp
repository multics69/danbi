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

  bConf.cpp -- print out build configuration 
 */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
#ifdef DANBI_DISABLE_SPECULATIVE_SCHEDULING
  bool bDANBI_DISABLE_SPECULATIVE_SCHEDULING = true; 
#else
  bool bDANBI_DISABLE_SPECULATIVE_SCHEDULING = false; 
#endif
#ifdef DANBI_DISABLE_RANDOM_SCHEDULING 
  bool bDANBI_DISABLE_RANDOM_SCHEDULING  = true; 
#else
  bool bDANBI_DISABLE_RANDOM_SCHEDULING  = false; 
#endif
#ifdef DANBI_ENABLE_EVENT_LOGGER 
  bool bDANBI_ENABLE_EVENT_LOGGER  = true; 
#else
  bool bDANBI_ENABLE_EVENT_LOGGER  = false; 
#endif
#ifdef DANBI_DISABLE_MULTIPLE_ITER_OPTIMIZATION
  bool bDANBI_DISABLE_MULTIPLE_ITER_OPTIMIZATION = true; 
#else
  bool bDANBI_DISABLE_MULTIPLE_ITER_OPTIMIZATION = false; 
#endif
#ifdef DANBI_DISABLE_PER_CPU_ORDERING_TICKET_SERVER
  bool bDANBI_DISABLE_PER_CPU_ORDERING_TICKET_SERVER = true; 
#else
  bool bDANBI_DISABLE_PER_CPU_ORDERING_TICKET_SERVER = false; 
#endif
#ifdef DANBI_DO_NOT_TICKET
  bool bDANBI_DO_NOT_TICKET = true; 
#else
  bool bDANBI_DO_NOT_TICKET = false; 
#endif
#ifdef DANBI_ENABLE_BACKOFF_DELAY
  bool bDANBI_ENABLE_BACKOFF_DELAY = true; 
#else
  bool bDANBI_ENABLE_BACKOFF_DELAY = false; 
#endif
#ifdef DNABI_READY_QUEUE_USE_MSQ
  bool bDNABI_READY_QUEUE_USE_MSQ = true; 
#else
  bool bDNABI_READY_QUEUE_USE_MSQ = false; 
#endif
#ifdef DNABI_READY_QUEUE_USE_LSQ
  bool bDNABI_READY_QUEUE_USE_LSQ = true; 
#else
  bool bDNABI_READY_QUEUE_USE_LSQ = false; 
#endif
#ifdef DANBI_DO_NOT_USE_NUMA_INTERLEAVE_MEM_POLICY
  bool bDANBI_DO_NOT_USE_NUMA_INTERLEAVE_MEM_POLICY = true; 
#else
  bool bDANBI_DO_NOT_USE_NUMA_INTERLEAVE_MEM_POLICY = false; 
#endif 
#ifdef DANBI_DO_NOT_PREVENT_CONCURRENT_PAGE_FAULTS
  bool bDANBI_DO_NOT_PREVENT_CONCURRENT_PAGE_FAULTS = true; 
#else
  bool bDANBI_DO_NOT_PREVENT_CONCURRENT_PAGE_FAULTS = false; 
#endif
#ifdef DANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL
  bool bDANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL = true; 
#else
  bool bDANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL = false; 
#endif 
#ifdef DANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING  
  bool bDANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING = true; 
#else
  bool bDANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING = false; 
#endif

#ifdef DANBI_ENABLE_EAGER_THREAD_DESTROY
  bool bDANBI_ENABLE_EAGER_THREAD_DESTROY = true; 
#else
  bool bDANBI_ENABLE_EAGER_THREAD_DESTROY = false;
#endif 

#ifdef DANBI_EVENT_LOGGER_DISABLE_QUEUE_EVENT
  bool bDANBI_EVENT_LOGGER_DISABLE_QUEUE_EVENT = true; 
#else 
  bool bDANBI_EVENT_LOGGER_DISABLE_QUEUE_EVENT = false; 
#endif 

#ifdef DANBI_EVENT_LOGGER_DISABLE_THREAD_LIFECYCLE
  bool bDANBI_EVENT_LOGGER_DISABLE_THREAD_LIFECYCLE = true; 
#else
  bool bDANBI_EVENT_LOGGER_DISABLE_THREAD_LIFECYCLE = false; 
#endif 

#ifdef DANBI_USE_BLOCKING_RESERVE_COMMIT_QUEUE
  bool bDANBI_USE_BLOCKING_RESERVE_COMMIT_QUEUE = true;
#else
  bool bDANBI_USE_BLOCKING_RESERVE_COMMIT_QUEUE = false;
#endif 

#ifdef DANBI_USE_RESIZABLE_BUFFER_MANAGER_BY_DEFAULT
  bool bDANBI_USE_RESIZABLE_BUFFER_MANAGER_BY_DEFAULT = true; 
#else
  bool bDANBI_USE_RESIZABLE_BUFFER_MANAGER_BY_DEFAULT = false; 
#endif

#ifdef DANBI_USE_RESIZABLE_QUEUE
  bool bDANBI_USE_RESIZABLE_QUEUE = true; 
#else
  bool bDANBI_USE_RESIZABLE_QUEUE = false; 
#endif

  printf("# DANBI Build Configuration\n");
  printf("# DANBI_DISABLE_SPECULATIVE_SCHEDULING: %s\n", 
         bDANBI_DISABLE_SPECULATIVE_SCHEDULING ?
         "yes" : "no");
  printf("# DANBI_DISABLE_RANDOM_SCHEDULING : %s\n", 
         bDANBI_DISABLE_RANDOM_SCHEDULING  ?
         "yes" : "no");
  printf("# DANBI_ENABLE_EVENT_LOGGER : %s\n", 
         bDANBI_ENABLE_EVENT_LOGGER  ?
         "yes" : "no");
  printf("# DANBI_DISABLE_MULTIPLE_ITER_OPTIMIZATION: %s\n", 
         bDANBI_DISABLE_MULTIPLE_ITER_OPTIMIZATION ?
         "yes" : "no");
  printf("# DANBI_DISABLE_PER_CPU_ORDERING_TICKET_SERVER: %s\n", 
         bDANBI_DISABLE_PER_CPU_ORDERING_TICKET_SERVER ?
         "yes" : "no");
  printf("# DANBI_DO_NOT_TICKET: %s\n", 
         bDANBI_DO_NOT_TICKET ?
         "yes" : "no");
  printf("# DANBI_ENABLE_BACKOFF_DELAY: %s\n", 
         bDANBI_ENABLE_BACKOFF_DELAY ?
         "yes" : "no");
  printf("# DNABI_READY_QUEUE_USE_MSQ: %s\n", 
         bDNABI_READY_QUEUE_USE_MSQ ?
         "yes" : "no");
  printf("# DNABI_READY_QUEUE_USE_LSQ: %s\n", 
         bDNABI_READY_QUEUE_USE_LSQ ?
         "yes" : "no");
  printf("# DANBI_DO_NOT_USE_NUMA_INTERLEAVE_MEM_POLICY: %s\n", 
         bDANBI_DO_NOT_USE_NUMA_INTERLEAVE_MEM_POLICY ?
         "yes" : "no");
  printf("# DANBI_DO_NOT_PREVENT_CONCURRENT_PAGE_FAULTS: %s\n", 
         bDANBI_DO_NOT_PREVENT_CONCURRENT_PAGE_FAULTS ?
         "yes" : "no");
  printf("# DANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL: %s\n", 
         bDANBI_ENABLE_RANDOM_JUMP_FOR_VISITED_KERNEL ?
         "yes" : "no");
  printf("# DANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING: %s\n", 
         bDANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING ?
         "yes" : "no");
  printf("# DANBI_ENABLE_EAGER_THREAD_DESTROY: %s\n", 
         bDANBI_ENABLE_EAGER_THREAD_DESTROY ?
         "yes" : "no");
  printf("# DANBI_EVENT_LOGGER_DISABLE_QUEUE_EVENT: %s\n", 
         bDANBI_EVENT_LOGGER_DISABLE_QUEUE_EVENT ?
         "yes" : "no");
  printf("# DANBI_ENABLE_EAGER_THREAD_DESTROY: %s\n", 
         bDANBI_EVENT_LOGGER_DISABLE_THREAD_LIFECYCLE ?
         "yes" : "no");
  printf("# DANBI_USE_BLOCKING_RESERVE_COMMIT_QUEUE: %s\n", 
         bDANBI_USE_BLOCKING_RESERVE_COMMIT_QUEUE ?
         "yes" : "no");
  printf("# DANBI_USE_RESIZABLE_BUFFER_MANAGER_BY_DEFAULT: %s\n", 
         bDANBI_USE_RESIZABLE_BUFFER_MANAGER_BY_DEFAULT ?
         "yes" : "no");
  printf("# DANBI_USE_RESIZABLE_QUEUE: %s\n", 
         bDANBI_USE_RESIZABLE_QUEUE ?
         "yes" : "no");
}
