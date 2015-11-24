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

  ContextSwitchIA64.S -- User-level context switching of IA32 architecture
	void danbi_ContextSwitch(void** OldSP, void* NewSP)
        void danbi_ContextSwitchWithCallback(void** OldSP, void* NewSP,
                                             void (*Callback)(void*),
	                                     void* CallbackArg)
	void danbi_TerminateHelper_(void)
 */
	.text
/* void danbi_ContextSwitch(void** OldSP, void* NewSP) */
.global danbi_ContextSwitch
danbi_ContextSwitch:
	/* Save registers */
	/* : No need to save scratch registers which are already stored by compiler. */ 
	push %rdi
	push %rbp
	push %rbx
	push %r12
	push %r13
	push %r14
	push %r15
	/* Save old stack */
	mov %rsp, (%rdi)
	/* Load new stack */
	mov %rsi, %rsp
	/* Restore stacks */
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rbx
	pop %rbp
	pop %rdi
	ret

/* void danbi_ContextSwitchWithCallback(void** OldSP, void* NewSP,
                                        void (*Callback)(void*), void* CallbackArg) */
.global danbi_ContextSwitchWithCallback
danbi_ContextSwitchWithCallback:	
	/* Save registers */
	/* : No need to save scratch registers which are already stored by compiler. */ 
	push %rdi
	push %rbp
	push %rbx
	push %r12
	push %r13
	push %r14
	push %r15
	/* Save old stack */
	mov %rsp, (%rdi)
	/* Load new stack */
	mov %rsi, %rsp
	/* call Callback(CallbackArg) */
	mov %rcx, %rdi
	call *%rdx
	/* Restore stacks */
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rbx
	pop %rbp
	pop %rdi
	ret

/* void danbi_TerminateHelper_(void) */
.global danbi_TerminateHelper_
danbi_TerminateHelper_:	
	/* get thread local native thread SP */ 
	call danbi_GetNativeThreadSP
	/* Load new stack */
	mov %rax, %rsp
	/* Restore registers */
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rbx
	pop %rbp
	pop %rdi
	ret
