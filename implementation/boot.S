.section ".text.boot"  // Make sure the linker puts this at the start of the kernel image

.global _start  // Execution starts here

_start:
    // Check processor ID is zero (executing on main core), else hang
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
    // We're not on the main core, so hang in an infinite wait loop
1:  wfe
    b       1b
2:  // We're on the main core!

    // Set stack to start below our code
    ldr     x1, =_start
    mov     sp, x1

    // Clean the BSS section
    ldr     x1, =__bss_start     // Start address
    ldr     w2, =__bss_size      // Size of the section
3:  cbz     w2, 4f               // Quit loop if zero
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b               // Loop if non-zero

    // Jump to our main() routine in C (make sure it doesn't return)
4:  bl      main
    // In case it does return, halt the master core too
    b       1b


.global DisableInterrupts
DisableInterrupts:
        msr    daifset, #2
        ret

.global EnableInterrupts
EnableInterrupts:
    msr    daifclr, #2 
    ret


.global StartCritical
StartCritical:
        mrs x0, daif
        b DisableInterrupts
       ret

.global EndCritical
EndCritical:
        mrs x1, daif
        and x0, x0, 1
        orr x1, x0, x1
        msr daif, x1 
        ret

.global WaitForInterrupt
WaitForInterrupt:
        WFI
        ret



.globl cpu_switch_to
cpu_switch_to:
	mov	x10, 0
	add	x8, x0, x10
	mov	x9, sp
	stp	x19, x20, [x8], #16		
	stp	x21, x22, [x8], #16
	stp	x23, x24, [x8], #16
	stp	x25, x26, [x8], #16
	stp	x27, x28, [x8], #16
	stp	x29, x9, [x8], #16
	str	x30, [x8]
	add	x8, x1, x10
	ldp	x19, x20, [x8], #16		
	ldp	x21, x22, [x8], #16
	ldp	x23, x24, [x8], #16
	ldp	x25, x26, [x8], #16
	ldp	x27, x28, [x8], #16
	ldp	x29, x9, [x8], #16
	ldr	x30, [x8]
	mov	sp, x9
	ret


.globl StartOS
StartOS:
   	mov	x10, 0
    ldr x0, =currThread    
    ldr x0, [x0]          
    add	x8, x0, x10       
	ldp	x19, x20, [x8], #16		
	ldp	x21, x22, [x8], #16
	ldp	x23, x24, [x8], #16
	ldp	x25, x26, [x8], #16
	ldp	x27, x28, [x8], #16
	ldp	x29, x9, [x8], #16
	ldr	x30, [x8]           

    msr  daifclr, #2                 

    br x30             



// IRQ.S
.globl irq_init_vectors
irq_init_vectors:
    adr x0, vectors
    msr vbar_el1, x0
    ret

.globl irq_enable
irq_enable:
    msr daifclr, #2 /* clear the 'i' mask */
    ret

.globl irq_disable
irq_disable:
    msr daifset, #2 /* set the 'i' mask */
    ret

// ENTRY.S

#include "entry.h"
#include "sysregs.h"

.macro kernel_entry, el
    sub sp, sp,   #S_FRAME_SIZE
    stp x0, x1,   [sp, #16 * 0]
    stp x2, x3,   [sp, #16 * 1]
    stp x4, x5,   [sp, #16 * 2]
    stp x6, x7,   [sp, #16 * 3]
    stp x8, x9,   [sp, #16 * 4]
    stp x10, x11, [sp, #16 * 5]
    stp x12, x13, [sp, #16 * 6]
    stp x14, x15, [sp, #16 * 7]
    stp x16, x17, [sp, #16 * 8]
    stp x18, x19, [sp, #16 * 9]
    stp x20, x21, [sp, #16 * 10]
    stp x22, x23, [sp, #16 * 11]
    stp x24, x25, [sp, #16 * 12]
    stp x26, x27, [sp, #16 * 13]
    stp x28, x29, [sp, #16 * 14]

    /*
     * we have to save sp_el0 on the kernel stack because
     * we may context switch
     */
    .if  \el == 0
    mrs x21, sp_el0
    .else
    add x21, sp, #S_FRAME_SIZE
    .endif /* \el == 0 */

    mrs x22, elr_el1
    mrs x23, spsr_el1

    stp x30, x21, [sp, #16 * 15]
    stp x22, x23, [sp, #16 * 16]
.endm

.macro kernel_exit, el
    ldp x22, x23, [sp, #16 * 16]
    ldp x30, x21, [sp, #16 * 15]

    /* 
     * we don't have to restore sp if el == 1 because 
     * core_switch_to helps us with that
     */
    .if  \el == 0
    msr sp_el0, x21
    .endif /* \el == 0 */

    msr elr_el1, x22
    msr spsr_el1, x23

    ldp x0, x1,   [sp, #16 * 0]
    ldp x2, x3,   [sp, #16 * 1]
    ldp x4, x5,   [sp, #16 * 2]
    ldp x6, x7,   [sp, #16 * 3]
    ldp x8, x9,   [sp, #16 * 4]
    ldp x10, x11, [sp, #16 * 5]
    ldp x12, x13, [sp, #16 * 6]
    ldp x14, x15, [sp, #16 * 7]
    ldp x16, x17, [sp, #16 * 8]
    ldp x18, x19, [sp, #16 * 9]
    ldp x20, x21, [sp, #16 * 10]
    ldp x22, x23, [sp, #16 * 11]
    ldp x24, x25, [sp, #16 * 12]
    ldp x26, x27, [sp, #16 * 13]
    ldp x28, x29, [sp, #16 * 14]
    add sp, sp,   #S_FRAME_SIZE
    eret
.endm

.macro  ventry  label
.align  7
    b \label
.endm

.align 11
.globl vectors
vectors:
    ventry sync_invalid_el1t
    ventry irq_invalid_el1t
    ventry fiq_invalid_el1t
    ventry serror_invalid_el1t

    ventry sync_invalid_el1h
    ventry handle_irq_el1h
    ventry fiq_invalid_el1h
    ventry serror_invalid_el1h

    ventry handle_sync_el0_64
    ventry handle_irq_el0_64
    ventry fiq_invalid_el0_64
    ventry serror_invalid_el0_64

    ventry sync_invalid_el0_32
    ventry irq_invalid_el0_32
    ventry fiq_invalid_el0_32
    ventry serror_invalid_el0_32

.macro handle_invalid_entry el, type
    kernel_entry \el
    mov x0, #\type
    mrs x1, esr_el1
    mrs x2, elr_el1
    bl show_invalid_entry_message
    b err_hang
.endm

sync_invalid_el1t:
    handle_invalid_entry 1, SYNC_INVALID_EL1t

irq_invalid_el1t:
    handle_invalid_entry 1, IRQ_INVALID_EL1t

fiq_invalid_el1t:
    handle_invalid_entry 1, FIQ_INVALID_EL1t

serror_invalid_el1t:
    handle_invalid_entry 1, SERROR_INVALID_EL1t

sync_invalid_el1h:
    handle_invalid_entry 1, SYNC_INVALID_EL1h

irq_invalid_el1h:
    handle_invalid_entry 1, IRQ_INVALID_EL1h

fiq_invalid_el1h:
    handle_invalid_entry 1, FIQ_INVALID_EL1h

serror_invalid_el1h:
    handle_invalid_entry 1, SERROR_INVALID_EL1h

sync_invalid_el0_64:
    handle_invalid_entry 0, SYNC_INVALID_EL0_64

irq_invalid_el0_64:
    handle_invalid_entry 0, IRQ_INVALID_EL0_64

fiq_invalid_el0_64:
    handle_invalid_entry 0, FIQ_INVALID_EL0_64

serror_invalid_el0_64:
    handle_invalid_entry 0, SERROR_INVALID_EL0_64

sync_invalid_el0_32:
    handle_invalid_entry 0, SYNC_INVALID_EL0_32

irq_invalid_el0_32:
    handle_invalid_entry 0, IRQ_INVALID_EL0_32

fiq_invalid_el0_32:
    handle_invalid_entry 0, FIQ_INVALID_EL0_32

serror_invalid_el0_32:
    handle_invalid_entry 0, SERROR_INVALID_EL0_32

handle_irq_el1h:
    kernel_entry 1
    bl handle_irq
    kernel_exit 1

handle_irq_el0_64:
    kernel_entry 0
    bl handle_irq
    kernel_exit 0

handle_sync_el0_64:
    kernel_entry 0
    /* check esr_el1 if it is svc or da */
    mrs x25, esr_el1
    lsr x24, x25, #ESR_ELx_EC_SHIFT
    cmp x24, #ESR_ELx_EC_SVC64
    b.eq el0_svc
    cmp x24, #ESR_ELx_EC_DA_LOW
    b.eq el0_da
    handle_invalid_entry 0, SYNC_ERROR

/*
 * x25 = NR_SYSCALLS
 * x26 = syscall number
 * x27 = sys_call_table 
 */ 

el0_svc:
    adr x27, sys_call_table
    /* zero extend the syscall number */
    uxtw x26, w8
    mov x25, #NR_SYSCALLS
    bl irq_enable
    cmp x26, x25
    /* branch if syscall number >= NR_SYSCALLS */ 
    b.hs invalid_syscall_num
    /* call syscall */

    b ret_from_syscall

invalid_syscall_num:
    handle_invalid_entry 0, SYSCALL_ERROR

ret_from_syscall:
    bl irq_disable
    /*
     * store the return value of the syscall on the stack so 
     * kernel_exit will pop it back to x0
     */
    str x0, [sp, 0]
    kernel_exit 0

el0_da:
    bl irq_enable
    mrs x0, far_el1
    mrs x1, esr_el1
    bl do_data_abort
    cmp x0, 0
    b.eq 1f
    handle_invalid_entry 0, DATA_ABORT_ERROR
1:
    bl irq_disable
    kernel_exit 0

.globl ret_from_fork
ret_from_fork:
    bl preempt_enable
    /* x19 == 0 means we're cloning */
    cbz x19, ret_to_user
    mov x0, x20
    blr x19

ret_to_user:
    bl irq_disable
    kernel_exit 0

.globl err_hang
err_hang: b err_hang




// UTILS.S

.globl get_el
get_el:
	mrs x0, CurrentEL
	lsr x0, x0, #2
	ret

.globl put32
put32:
	str w1,[x0]
	ret

.globl get32
get32:
	ldr w0,[x0]
	ret

.globl delay
delay:
	subs x0, x0, #1
	bne delay
	ret