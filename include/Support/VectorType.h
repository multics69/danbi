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

  VectorType.h -- vector type definition using GCC vector extension 
 */
#ifndef DANBI_VECTOR_TYPE_H
#define DANBI_VECTOR_TYPE_H
namespace danbi {
#define DEFINE_VECTOR2_TYPE(__Ty, __VTy)                                \
  typedef __Ty __v2##__Ty __attribute__((vector_size(sizeof(__Ty)*2))); \
  union __VTy {                                                         \
    __v2##__Ty v;                                                       \
    struct {                                                            \
      __Ty x, y;                                                        \
    };                                                                  \
    struct {                                                            \
      __Ty s0, s1;                                                      \
    };                                                                  \
    __VTy() {}                                                          \
    __VTy(__Ty IV) {                                                    \
      x = y = IV;                                                       \
    }                                                                   \
    __VTy& operator+=(const __VTy& rhs) {                               \
      v += rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
    __VTy& operator-=(const __VTy& rhs) {                               \
      v -= rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
    __VTy& operator*=(const __VTy& rhs) {                               \
      v *= rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
    __VTy& operator/=(const __VTy& rhs) {                               \
      v /= rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
  }  __attribute__ ((aligned (sizeof(__Ty)*2)));                        \
  inline __VTy operator+(__VTy lhs, const __VTy& rhs) {                 \
    lhs += rhs;                                                         \
    return lhs;                                                         \
  }                                                                     \
  inline __VTy operator-(__VTy lhs, const __VTy& rhs) {                 \
    lhs -= rhs;                                                         \
    return lhs;                                                         \
  }                                                                     \
  inline __VTy operator*(__VTy lhs, const __VTy& rhs) {                 \
    lhs *= rhs;                                                         \
    return lhs;                                                         \
  }                                                                     \
  inline __VTy operator/(__VTy lhs, const __VTy& rhs) {                 \
    lhs /= rhs;                                                         \
    return lhs;                                                         \
  } 

#define DEFINE_VECTOR3_TYPE(__Ty, __VTy)                                \
  typedef __Ty __v3##__Ty __attribute__((vector_size(sizeof(__Ty)*4))); \
  union __VTy {                                                         \
    __v3##__Ty v;                                                       \
    struct {                                                            \
      __Ty x, y, z, __w;                                                \
    };                                                                  \
    struct {                                                            \
      __Ty s0, s1, s2, __s3;                                            \
    };                                                                  \
    __VTy() {}                                                          \
    __VTy(__Ty IV) {                                                    \
      x = y = z = IV;                                                   \
    }                                                                   \
    __VTy& operator+=(const __VTy& rhs) {                               \
      v += rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
    __VTy& operator-=(const __VTy& rhs) {                               \
      v -= rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
    __VTy& operator*=(const __VTy& rhs) {                               \
      v *= rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
    __VTy& operator/=(const __VTy& rhs) {                               \
      v /= rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
  }  __attribute__ ((aligned (sizeof(__Ty)*4)));                        \
  inline __VTy operator+(__VTy lhs, const __VTy& rhs) {                 \
    lhs += rhs;                                                         \
    return lhs;                                                         \
  }                                                                     \
  inline __VTy operator-(__VTy lhs, const __VTy& rhs) {                 \
    lhs -= rhs;                                                         \
    return lhs;                                                         \
  }                                                                     \
  inline __VTy operator*(__VTy lhs, const __VTy& rhs) {                 \
    lhs *= rhs;                                                         \
    return lhs;                                                         \
  }                                                                     \
  inline __VTy operator/(__VTy lhs, const __VTy& rhs) {                 \
    lhs /= rhs;                                                         \
    return lhs;                                                         \
  } 
      
#define DEFINE_VECTOR4_TYPE(__Ty, __VTy)                                \
  typedef __Ty __v4##__Ty __attribute__((vector_size(sizeof(__Ty)*4))); \
  union __VTy {                                                         \
    __v4##__Ty v;                                                       \
    struct {                                                            \
      __Ty x, y, z, w;                                                  \
    };                                                                  \
    struct {                                                            \
      __Ty s0, s1, s2, s3;                                              \
    };                                                                  \
    __VTy() {}                                                          \
    __VTy(__Ty IV) {                                                    \
      x = y = z = w = IV;                                               \
    }                                                                   \
    __VTy& operator+=(const __VTy& rhs) {                               \
      v += rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
    __VTy& operator-=(const __VTy& rhs) {                               \
      v -= rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
    __VTy& operator*=(const __VTy& rhs) {                               \
      v *= rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
    __VTy& operator/=(const __VTy& rhs) {                               \
      v /= rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
  }  __attribute__ ((aligned (sizeof(__Ty)*4)));                        \
  inline __VTy operator+(__VTy lhs, const __VTy& rhs) {                 \
    lhs += rhs;                                                         \
    return lhs;                                                         \
  }                                                                     \
  inline __VTy operator-(__VTy lhs, const __VTy& rhs) {                 \
    lhs -= rhs;                                                         \
    return lhs;                                                         \
  }                                                                     \
  inline __VTy operator*(__VTy lhs, const __VTy& rhs) {                 \
    lhs *= rhs;                                                         \
    return lhs;                                                         \
  }                                                                     \
  inline __VTy operator/(__VTy lhs, const __VTy& rhs) {                 \
    lhs /= rhs;                                                         \
    return lhs;                                                         \
  } 
      
#define DEFINE_VECTOR8_TYPE(__Ty, __VTy)                                \
  typedef __Ty __v8##__Ty __attribute__((vector_size(sizeof(__Ty)*8))); \
  union __VTy {                                                         \
    __v8##__Ty v;                                                       \
    struct {                                                            \
      __Ty s0, s1, s2, s3, s4, s5, s6, s7;                              \
    };                                                                  \
    __VTy() {}                                                          \
    __VTy(__Ty IV) {                                                    \
      s0 = s1 = s2 = s3 = s4 = s5 = s6 = s7 = IV;                       \
    }                                                                   \
    __VTy& operator+=(const __VTy& rhs) {                               \
      v += rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
    __VTy& operator-=(const __VTy& rhs) {                               \
      v -= rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
    __VTy& operator*=(const __VTy& rhs) {                               \
      v *= rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
    __VTy& operator/=(const __VTy& rhs) {                               \
      v /= rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
  }  __attribute__ ((aligned (sizeof(__Ty)*8)));                        \
  inline __VTy operator+(__VTy lhs, const __VTy& rhs) {                 \
    lhs += rhs;                                                         \
    return lhs;                                                         \
  }                                                                     \
  inline __VTy operator-(__VTy lhs, const __VTy& rhs) {                 \
    lhs -= rhs;                                                         \
    return lhs;                                                         \
  }                                                                     \
  inline __VTy operator*(__VTy lhs, const __VTy& rhs) {                 \
    lhs *= rhs;                                                         \
    return lhs;                                                         \
  }                                                                     \
  inline __VTy operator/(__VTy lhs, const __VTy& rhs) {                 \
    lhs /= rhs;                                                         \
    return lhs;                                                         \
  } 
      
#define DEFINE_VECTOR16_TYPE(__Ty, __VTy)                               \
  typedef __Ty __vF##__Ty __attribute__((vector_size(sizeof(__Ty)*16)));\
  union __VTy {                                                         \
    __vF##__Ty v;                                                       \
    struct {                                                            \
      __Ty s0, s1, s2, s3, s4, s5, s6, s7;                              \
      __Ty s8, s9, sa, sb, sc, sd, se, sf;                              \
    };                                                                  \
    __VTy() {}                                                          \
    __VTy(__Ty IV) {                                                    \
      s0 = s1 = s2 = s3 = s4 = s5 = s6 = s7 =                           \
      s8 = s9 = sa = sb = sc = sd = se = sf = IV;                       \
    }                                                                   \
    __VTy& operator+=(const __VTy& rhs) {                               \
      v += rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
    __VTy& operator-=(const __VTy& rhs) {                               \
      v -= rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
    __VTy& operator*=(const __VTy& rhs) {                               \
      v *= rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
    __VTy& operator/=(const __VTy& rhs) {                               \
      v /= rhs.v;                                                       \
      return *this;                                                     \
    }                                                                   \
  }  __attribute__ ((aligned (sizeof(__Ty)*16)));                       \
  inline __VTy operator+(__VTy lhs, const __VTy& rhs) {                 \
    lhs += rhs;                                                         \
    return lhs;                                                         \
  }                                                                     \
  inline __VTy operator-(__VTy lhs, const __VTy& rhs) {                 \
    lhs -= rhs;                                                         \
    return lhs;                                                         \
  }                                                                     \
  inline __VTy operator*(__VTy lhs, const __VTy& rhs) {                 \
    lhs *= rhs;                                                         \
    return lhs;                                                         \
  }                                                                     \
  inline __VTy operator/(__VTy lhs, const __VTy& rhs) {                 \
    lhs /= rhs;                                                         \
    return lhs;                                                         \
  } 

DEFINE_VECTOR2_TYPE(char,  char2); 
DEFINE_VECTOR3_TYPE(char,  char3); 
DEFINE_VECTOR4_TYPE(char,  char4); 
DEFINE_VECTOR8_TYPE(char,  char8); 
DEFINE_VECTOR16_TYPE(char, char16); 

DEFINE_VECTOR2_TYPE(short,  short2); 
DEFINE_VECTOR3_TYPE(short,  short3); 
DEFINE_VECTOR4_TYPE(short,  short4); 
DEFINE_VECTOR8_TYPE(short,  short8); 
DEFINE_VECTOR16_TYPE(short, short16); 

DEFINE_VECTOR2_TYPE(int,  int2); 
DEFINE_VECTOR3_TYPE(int,  int3); 
DEFINE_VECTOR4_TYPE(int,  int4); 
DEFINE_VECTOR8_TYPE(int,  int8); 
DEFINE_VECTOR16_TYPE(int, int16); 

DEFINE_VECTOR2_TYPE(float,  float2); 
DEFINE_VECTOR3_TYPE(float,  float3); 
DEFINE_VECTOR4_TYPE(float,  float4); 
DEFINE_VECTOR8_TYPE(float,  float8); 
DEFINE_VECTOR16_TYPE(float, float16); 

} // End of danbi namespace
#endif 
