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

  Random.h -- linear congruential random number generator
 */
#ifndef DANBI_RANDOM_H
#define DANBI_RANDOM_H
#include <Support/BranchHint.h>

namespace danbi {

class Random {
private:
  static __thread int Rand; 

  static int genPerThreadoRandomSeed() {
    volatile unsigned short S[4]; 
    // Random seed based on stack garbage value
    int SeedVal = (int)((long)&Rand * (long)&S) + (S[0] ^ S[3]) + (S[1] & S[2]);
    return SeedVal & 0x7fffffff; 
  }

public:
  static inline void setSeed(int Seed) {
    Rand = Seed & 0x7fffffff; 
  }

  static int randomInt() {
    // Set random seed if it is the first try. 
    if (unlikely(Rand == 0x80000000))
      Rand = genPerThreadoRandomSeed(); 

    // Linear congruential random number generator
    // http://en.wikipedia.org/wiki/Linear_congruential_generator
    Rand = (1103515245*Rand + 12345) & 0x7fffffff;
    return Rand; 
  }

  static int randomIntRange(int low, int high) { 
    return low + (randomInt() % (high - low + 1));
  }

  static float randomFloat() {
    return (float)randomInt() / (float)0x7fffffff; 
  }

  static bool randomBool() {
    return randomInt() >= (0x7fffffff >> 1); 
  }

}; 

} // End of danbi namespace

#endif 
