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

  ContextSwitch.h -- user-level context switching
 */
#ifndef DANBI_SWITCH_H
#define DANBI_SWITCH_H
#ifdef __cplusplus
extern "C" {
#endif

/* user-level context switching */ 
void danbi_ContextSwitch(void** OldSP, void* NewSP); 

/* user-level context switching: 
   Before returning to the new context, registered callback function is called. */
void danbi_ContextSwitchWithCallback(void** OldSP, void* NewSP,
                                     void (*Callback)(void*), void* CallbackArg);

/* start new user-level stread */ 
void* danbi_StartGreenThread(void* NewSP); 

/* get native thread stack pointer */ 
void* danbi_GetNativeThreadSP(); 

/* Initialize stack in a target specific way */ 
void* danbi_InitGreenStack(void* StackLowAddr, int Size, void* PC, void* Arg);

/* Check if a stack is not corrupted */ 
int danbi_IsGreenStackSane(void* StackLowAddr, int Size);
#ifdef __cplusplus
}
#endif
#endif 
