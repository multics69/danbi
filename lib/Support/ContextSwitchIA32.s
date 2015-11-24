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

  ContextSwitchIA32.S -- User-level context switching of IA32 architecture
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
	/* Load two arguments */ 
	mov 4(%esp), %eax /* Load OldSP */
	mov 8(%esp), %ecx /* Load NewSP */ 
	/* Save registers */
	/* : No need to save scratch registers which are already stored by compiler. */ 
	push %ebp
	push %ebx
	push %esi
	push %edi
	/* Save old stack */
	mov %esp, (%eax)
	/* Load new stack */
	mov %ecx, %esp
	/* Restore registers */
	pop %edi
	pop %esi
	pop %ebx
	pop %ebp
	ret

/* void danbi_ContextSwitchWithCallback(void** OldSP, void* NewSP,
                                        void (*Callback)(void*), void* CallbackArg) */
.global danbi_ContextSwitchWithCallback
danbi_ContextSwitchWithCallback:	
	/* Load two arguments */ 
	mov 4(%esp), %eax /* Load OldSP */
	mov 8(%esp), %ecx /* Load NewSP */
	/* Save registers */
	/* : No need to save scratch registers which are already stored by compiler. */ 
	push %ebp
	mov 20(%esp), %ebp /* Load CallbackArg */
	push %ebx
	mov 20(%esp), %ebx /* Load Callback */
	push %esi
	push %edi
	/* Save old stack */
	mov %esp, (%eax)
	/* Load new stack */
	mov %ecx, %esp
	/* call Callback(CallbackArg) */
	push %ebp
	call *%ebx
	addl $4, %esp
	/* Restore registers */
	pop %edi
	pop %esi
	pop %ebx
	pop %ebp
	ret

/* void danbi_TerminateHelper_(void) */
.global danbi_TerminateHelper_
danbi_TerminateHelper_:	
	/* get thread local native thread SP */ 
	call danbi_GetNativeThreadSP
	/* Load new stack */
	mov %eax, %esp
	/* Restore registers */
	pop %edi
	pop %esi
	pop %ebx
	pop %ebp
	ret
