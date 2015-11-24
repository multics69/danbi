/*
  Copyright (C) 2012 Changwoo Min. All Rights Reserved.

  This file is part of DANBI project. 

  NOTICE: All information contained herein is, and remains the property 
  of Changwoo Min. The intellectual and technical concepts contained 
  herein are proprietary to Changwoo Min and may be covered  by patents 
  or patents in process, and are protected by trade secret or copyright law. 
  Dissemination of this information or reproduction of this material is 
  strictly forbidden unless prior written permission is obtained 
  from Changwoo Min(multics69@gmail.com). 

  ContextSwitch.C -- utility functions for IA32 context switching
 */
#include "Support/ContextSwitch.h"

static __thread void* NativeThreadSP; 

#define DEBUG_MARKER(x) 
#define STACK_BOTTOM_MARKER (void *)0xdeadff01UL
#define STACK_TOP_MARKER (void *)0xdeadff03UL
#define WORD_SIZE sizeof(void*)
#define GROW_STACK(Size) do {                                    \
    StackTop = (void*)((char *)StackTop - (Size));               \
  } while(0)

extern void danbi_TerminateHelper_(void); 

static inline 
void* danbi_InitGreenStack_IA32(void* StackLowAddr, int Size, void* PC, void* Arg) {
  void** StackTop; 

  /* Calculate stack top address */ 
  StackTop = (void**)((char *)StackLowAddr + Size); 

  /* Stack bottom-most marker for debugging */ 
  GROW_STACK(WORD_SIZE);   DEBUG_MARKER(*StackTop = STACK_BOTTOM_MARKER); 

  /* Argument */ 
  GROW_STACK(WORD_SIZE);   *StackTop = Arg; 

  /* Thread termination address */ 
  GROW_STACK(WORD_SIZE);   *StackTop = danbi_TerminateHelper_; 

  /* Thread start address */ 
  GROW_STACK(WORD_SIZE);   *StackTop = PC; 

  /* Add non-scratch registers and markers for debugging: ebp, ebx, esi, edi */  
  GROW_STACK(WORD_SIZE);   DEBUG_MARKER(*StackTop = (void *)0xcdcdff01UL);
  GROW_STACK(WORD_SIZE);   DEBUG_MARKER(*StackTop = (void *)0xcdcdff02UL);
  GROW_STACK(WORD_SIZE);   DEBUG_MARKER(*StackTop = (void *)0xcdcdff03UL);
  GROW_STACK(WORD_SIZE);   DEBUG_MARKER(*StackTop = (void *)0xcdcdff04UL);

  /* ... */ 

  /* Stack top-most marker for debugging */ 
  DEBUG_MARKER(((void**)StackLowAddr)[0] = STACK_TOP_MARKER);
  return StackTop; 
}

static inline 
void* danbi_InitGreenStack_IA64(void* StackLowAddr, int Size, void* PC, void* Arg) {
  void** StackTop; 

  /* Calculate stack top address */ 
  StackTop = (void**)((char *)StackLowAddr + Size); 

  /* Stack bottom-most marker for debugging */ 
  GROW_STACK(WORD_SIZE);   DEBUG_MARKER(*StackTop = STACK_BOTTOM_MARKER);

  /* Add a padding for a stack to get 16-byte aligned. */ 
  GROW_STACK(WORD_SIZE);   DEBUG_MARKER(*StackTop = STACK_BOTTOM_MARKER);

  /* Thread termination address */ 
  GROW_STACK(WORD_SIZE);   *StackTop = danbi_TerminateHelper_; 

  /* Thread start address */ 
  GROW_STACK(WORD_SIZE);   *StackTop = PC; 

  /* Add non-scratch registers and markers for debugging: rdi, rbp, rbx, r12, r13, r14, r15 */
  GROW_STACK(WORD_SIZE);   *StackTop = Arg; /* The first argument is in %rdi. */
  GROW_STACK(WORD_SIZE);   DEBUG_MARKER(*StackTop = (void *)0xcdcdff02UL);
  GROW_STACK(WORD_SIZE);   DEBUG_MARKER(*StackTop = (void *)0xcdcdff03UL);
  GROW_STACK(WORD_SIZE);   DEBUG_MARKER(*StackTop = (void *)0xcdcdff04UL);
  GROW_STACK(WORD_SIZE);   DEBUG_MARKER(*StackTop = (void *)0xcdcdff05UL);
  GROW_STACK(WORD_SIZE);   DEBUG_MARKER(*StackTop = (void *)0xcdcdff06UL);
  GROW_STACK(WORD_SIZE);   DEBUG_MARKER(*StackTop = (void *)0xcdcdff07UL);

  /* ... */ 

  /* Stack top-most marker for debugging */ 
  DEBUG_MARKER(((void**)StackLowAddr)[0] = STACK_TOP_MARKER);
  return StackTop; 
}

static inline
int danbi_IsGreenStackSane_IA32_64(void* StackLowAddr, int Size) {
  int IsDebugMarkerEnabled = 0; 

  DEBUG_MARKER(IsDebugMarkerEnabled = 1); 
  if (IsDebugMarkerEnabled) {
    /* Calculate stack top address */ 
    void** StackTop = (void**)((char *)StackLowAddr + Size); 
    
    /* Stack bottom-most marker for debugging */ 
    GROW_STACK(WORD_SIZE);   
    if ( *StackTop != STACK_BOTTOM_MARKER )
      return -1;
    
    /* Check top-most marker */ 
    if ( ((void**)StackLowAddr)[0] != STACK_TOP_MARKER ) 
      return -2; 
  }
  return 0; 
}

int danbi_IsGreenStackSane(void* StackLowAddr, int Size) {
  return danbi_IsGreenStackSane_IA32_64(StackLowAddr, Size); 
}

void* danbi_InitGreenStack(void* StackLowAddr, int Size, void* PC, void* Arg) {
  // Initialize stack frame according to its architecture
  if (sizeof(void*) == 4) 
    return danbi_InitGreenStack_IA32(StackLowAddr, Size, PC, Arg);
  else if (sizeof(void*) == 8) 
    return danbi_InitGreenStack_IA64(StackLowAddr, Size, PC, Arg);

  // Never be here!
  assert(0); 
  return 0; 
}

void* danbi_StartGreenThread(void* NewSP) {
  danbi_ContextSwitch(&NativeThreadSP, NewSP); 
  return NativeThreadSP; 
}

void* danbi_GetNativeThreadSP() {
  return NativeThreadSP; 
}
