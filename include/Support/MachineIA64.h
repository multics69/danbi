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

  MachineIA64.h -- Intel IA64 architecture dependent implementations
 */

static_assert( sizeof(int) != sizeof(long) && 
               sizeof(long) == sizeof(void*), 
               "Assuming size of long is same as one of pointer." );

/// prefetch data to cache
template <typename T> inline
void Machine::prefetch(T* P) {
  __asm__ __volatile__(
    "prefetcht0 %0"
    : : "m"(P) : 
    );
}

/// Read time-stamp counter
unsigned long long Machine::rdtsc() {
  unsigned int lo, hi; 
  static_assert(sizeof(unsigned int) == 4 && sizeof(unsigned long long) == 8, 
                "Incorrect data type"); 
  __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
  return (unsigned long long)hi << 32 | (unsigned long long)lo;
}

/// compiler memory barrier
inline void Machine::cmb() {
  __asm__ __volatile__("":::"memory");
}

/// read/write memory barrier
inline void Machine::mb() {
  __asm__ __volatile__("mfence":::"memory");
}

/// read memory barrier
inline void Machine::rmb() {
  __asm__ __volatile__("lfence":::"memory");
}

/// write memory barrier
inline void Machine::wmb() {
  __asm__ __volatile__("sfence":::"memory");
}

/// atomic word add
template <typename T> inline
long Machine::atomicWordAddReturn(T *P, long Inc) {
  long i = Inc; 
  
  assert(sizeof(T)==sizeof(long) && "Wrong type: not an atomic type.");
  __asm__ __volatile__(
    "lock\n\t xadd %0,%1"
    : "+r"(Inc), "+m"(*P)
    : : "memory"
    );
  return Inc + i; 
}

/// atomic int add
template <typename T> inline
int Machine::atomicIntAddReturn(T *P, int Inc) {
  int i = Inc; 
  
  assert(sizeof(T)==sizeof(int) && "Wrong type: not an atomic type.");
  __asm__ __volatile__(
    "lock\n\t xadd %0,%1"
    : "+r"(Inc), "+m"(*P)
    : : "memory"
    );
  return Inc + i; 
}

#if !DANBI_EMULATE_FAS
/// fetch and store a word
template <typename T> inline 
T Machine::fasWord(T *P, T V) {
  T r;

  __asm__ __volatile__(
    "lock\n\t xchg %0,%1"
    : "=r"(r), "=m"(*P)
    : "0"(V), "m"(*P)
    : "memory"
    );
  return r;
}
#endif 

/// fetch and store a int
template <typename T> inline 
T Machine::fasInt(T *P, T V) {
  return fasWord<T>(P, V); 
}

/// compare and swap a word
template <typename T> inline 
bool Machine::casWord(T *P, T O, T V) {
  bool eq; 

  __asm__ __volatile__(
    "lock\n\t cmpxchg %3,%1\n\t"
    "sete %2\n\t"
    : "=a"(O), "=m"(*P), "=g"(eq)
    : "r"(V), "m"(*P), "0"(O)
    : "memory"
    );
  return eq; 
}

/// compare and swap a int
template <typename T> inline
bool Machine::casInt(T *P, T O, T V) {
  return casWord<T>(P, O, V); 
}

/// compare and swap a double word
template <typename T> inline 
bool Machine::cas2Word(T *P, T O, T V) {
  unsigned long *o = (unsigned long *)&O; 
  unsigned long *v = (unsigned long *)&V;
  unsigned long tmp;
  bool eq; 

  assert(sizeof(T)==sizeof(void*)*2 && "Wrong type: not a double word.");
  __asm__ __volatile__(
    "mov %%rbx,%3\n\t"
    "mov %8,%%rbx\n\t"
    "lock\n\t cmpxchg16b %2\n\t"
    "sete %4\n\t"
    "mov %3,%%rbx\n\t"
    : "=a"(o[0]), "=d"(o[1]), "=m"(*P), "=&g"(tmp), "=g"(eq)
    : "m"(*P), "0"(o[0]), "1"(o[1]), "g"(v[0]), "c"(v[1])
    : "memory", "rbx"
    );
  return eq;
}

/// read a double word atomically
template <typename T> inline 
void Machine::atomicRead2Word(T *P, T *V) {
  assert(sizeof(T)==sizeof(void*)*2 && "Wrong type: not a double word.");
  unsigned long *v = (unsigned long *)V;
  unsigned long tmp;
  __asm__ __volatile__(
    "mov %%rbx,%3\n\t"
    "mov %7,%%rbx\n\t"
    "lock\n\t cmpxchg16b %2\n\t"
    "mov %3,%%rbx\n\t"
    : "=a"(v[0]), "=d"(v[1]), "=m"(*P), "=&g"(tmp)
    : "m"(*P), "0"(v[0]), "1"(v[1]), "g"(v[0]), "c"(v[1])
    : "memory", "rbx"
    );
}

/// write a double word atomically
template <typename T> inline 
void Machine::atomicWrite2Word(T *P, T V) {
  assert(sizeof(T)==sizeof(void*)*2 && "Wrong type: not a double word.");

  volatile unsigned long *p = (volatile unsigned long *)P;
  unsigned long *v = (unsigned long *)&V;
  unsigned long tmp;

  __asm__ __volatile__(
    "mov %%rbx,%1\n\t"
    "mov %5,%%rbx\n\t"
    // retry:
    "mov %3,%%rax\n\t" // 3
    "mov %4,%%rdx\n\t" // 3
    "lock\n\t cmpxchg16b %0\n\t" // 5 
    "jnz .-14\n\t" // 2 // goto retry:
    "mov %1,%%rbx\n\t"
    : "=m"(*P), "=&g"(tmp)
    : "m"(*P), "m"(p[0]), "m"(p[1]), "g"(v[0]), "c"(v[1])
    : "rbx", "rax", "rdx", "memory"
    );
}
