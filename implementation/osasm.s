#include "OS.h"

.globl cpu_switch_to
cpu_switch_to:
	mov	x10, #THREAD_CPU_CONTEXT
	add	x8, x0, x10
	mov	x9, sp
	stp	x19, x20, [x8], #16		// store callee-saved registers
	stp	x21, x22, [x8], #16
	stp	x23, x24, [x8], #16
	stp	x25, x26, [x8], #16
	stp	x27, x28, [x8], #16
	stp	x29, x9, [x8], #16
	str	x30, [x8]
	add	x8, x1, x10
	ldp	x19, x20, [x8], #16		// restore callee-saved registers
	ldp	x21, x22, [x8], #16
	ldp	x23, x24, [x8], #16
	ldp	x25, x26, [x8], #16
	ldp	x27, x28, [x8], #16
	ldp	x29, x9, [x8], #16
	ldr	x30, [x8]
	mov	sp, x9
	ret

; should start current thread
.globl StartOS
StartOS:
   	mov	x10, #THREAD_CPU_CONTEXT
    ldr x0, =currThread   ; stores the address of the currThread variable  
    ldr x0, [x0]          ; loads the value of the currThread variable (pointer to the current thread)
    add	x8, x0, x10       ; x8 = x0 + x10  (x8 = address of the current thread's cpu context)(x10 = offset of the cpu context in the thread struct)
	ldp	x19, x20, [x8], #16		// restore callee-saved registers
	ldp	x21, x22, [x8], #16
	ldp	x23, x24, [x8], #16
	ldp	x25, x26, [x8], #16
	ldp	x27, x28, [x8], #16
	ldp	x29, x9, [x8], #16
	ldr	x30, [x8]           ; TODO: maybe add x31???

    CPSIE I                     ; enable ints

    BX x30                   ; idk if this is correct may just need to do their implementation 
    ; BX to LR 