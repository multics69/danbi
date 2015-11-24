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

  Machine.h -- hardware dependent implementation such as atomic instructions
 */

#ifndef DANBI_MACHINE_H
#define DANBI_MACHINE_H
#include <cassert>
#include <string>

namespace danbi {
#define DANBI_EMULATE_FAS 0

class Machine {
private:
  static_assert( sizeof(long) == sizeof(void*), 
                 "Assuming pointer and long type is a word." ); 
  Machine(); // Do not implement 
  ~Machine(); // Do not implement

public:
  enum {
    CachelineSize = 64,
    CPUClockMHz = 2000, // FIXME: hardcoded to 2.0 GHz Xeon E7-4850 processor 
  }; 

  /// prefetch data to cache
  template <typename T> static inline
  void prefetch(T* P); 

  /// micro seconds to the number of CPU clocks
  static inline unsigned long long microToCycles(unsigned long long m) {
    return CPUClockMHz * m; 
  }

  static double cyclesToUSec(unsigned long long c) {
    return double(c)/double(CPUClockMHz);
  }

  /// Read time-stamp counter
  static inline unsigned long long rdtsc();

  /// compiler memory barrier
  static inline void cmb();

  /// read/write memory barrier
  static inline void mb();

  /// read memory barrier
  static inline void rmb();

  /// write memory barrier
  static inline void wmb();

  /// atomic int add
  template <typename T> static inline 
  int atomicIntAddReturn(T *P, int Inc);

  /// atomic int increment
  template <typename T> static inline 
  int atomicIntInc(T *P) {
    return atomicIntAddReturn<T>(P, 1); 
  }

  /// atomic int decrement
  template <typename T> static inline 
  int atomicIntDec(T *P) {
    return atomicIntAddReturn<T>(P, -1);
  }

  /// atomic word add
  template <typename T> static inline
  long atomicWordAddReturn(T *P, long Inc);

  /// atomic word increment
  template <typename T> static inline
  long atomicWordInc(T *P) {
    return atomicWordAddReturn<T>(P, 1L); 
  }

  /// atomic long decrement
  template <typename T> static inline
  long atomicWordDec(T *P) {
    return atomicWordAddReturn<T>(P, -1L); 
  }

  /// compare and swap a int
  template <typename T> static inline
  bool casInt(T *P, T O, T V); 

  /// compare and swap a word
  template <typename T> static inline
  bool casWord(T *P, T O, T V); 

  /// compare and swap a double word
  template <typename T> static inline
  bool cas2Word(T *P, T O, T V); 

  /// fetch and store a int
  template <typename T> static inline 
  T fasInt(T *P, T V);

#if !DANBI_EMULATE_FAS
  /// fetch and store a word
  template <typename T> static inline 
  T fasWord(T *P, T V);
#else
  template <typename T> static inline 
  T fasWord(T *P, T V) {
    volatile T O; 
    unsigned int B;
    volatile unsigned int b; 

    while (true) {
      O = *P; 
      if (Machine::casWord<T>(P, O, V))
        break;
    }
    return O;
  }
#endif // DANBI_EMULATE_FAS

  /// read a double word atomically
  template <typename T> static inline 
  void atomicRead2Word(T *P, T *V); 

  /// write a double word atomically
  template <typename T> static inline 
  void atomicWrite2Word(T *P, T V); 
};

/// Include hardware specific implementations
#include "Support/MachineIA64.h"

#define __cacheline_aligned __attribute__ ((aligned (Machine::CachelineSize)))
#define __doubleword_aligned __attribute__ ((aligned (sizeof(void*) * 2)))
} // End of danbi namespace

#endif 
