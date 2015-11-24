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

  TaggedPointer.h -- tagged pointer to avoid ABA problem
 */
#ifndef DANBI_TAGGED_POINTER_H
#define DANBI_TAGGED_POINTER_H
#include "Support/Machine.h"

namespace danbi {
template <typename Ty>
struct TaggedPointer {
  volatile Ty* P;
  volatile long T;

  /// Constructor 
  TaggedPointer() {}

  TaggedPointer(volatile Ty* P_, volatile long T_) 
    : P(P_), T(T_) {}

  // Assignment operator 
  inline TaggedPointer<Ty>& operator=(const TaggedPointer<Ty>&Other) {
    P = Other.P; 
    T = Other.T; 
    return *this; 
  }

  /// Comparison operator
  inline bool operator==(const TaggedPointer<Ty>&X) const {
    return (P == X.P) && (T == X.T); 
  }

  inline bool operator!=(const TaggedPointer<Ty>&X) const {
    return (P != X.P) || (T != X.T); 
  }
} __doubleword_aligned;

#define DANBI_CONCURRENT_FORWARD_ITERABLE(Ty)           \
  public:                                               \
  TaggedPointer<Ty> Next;                    

#define DANBI_CONCURRENT_BACKWARD_ITERABLE(Ty)          \
  public:                                               \
  TaggedPointer<Ty> Prev;                    

#define DANBI_CONCURRENT_BIDIRECTIONALLY_ITERABLE(Ty)   \
  public:                                               \
  TaggedPointer<Ty> Next;                               \
  TaggedPointer<Ty> Prev;                    

}; // End of danbi name space

#endif 
