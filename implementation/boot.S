#include "mm.h"
#include "mmu.h"
#include "sysregs.h"
#include "sys.h"

/* changes tmp1, tmp2 only */
.macro create_table_entry, tbl, ntbl, va, shift, flags, tmp1, tmp2
    /* get entry index in tmp1 */
    lsr \tmp1, \va, #\shift
    and \tmp1, \tmp1, #ENTRIES_PER_TABLE - 1
    /* tmp2 = entry value */
    mov \tmp2, \ntbl
    orr \tmp2, \tmp2, #\flags
    /* install entry */
    str \tmp2, [\tbl, \tmp1, lsl #3]
.endm

/* changes vstart, vend, pa, tmp1 */
/* vstart and vend should not point to the same block! */
.macro create_block_map, pmd, vstart, vend, pa, flags, tmp1
    /* turn vstart, vend into indices */
    lsr \vstart, \vstart, #SECTION_SHIFT
    and \vstart, \vstart, #ENTRIES_PER_TABLE - 1
    lsr \vend, \vend, #SECTION_SHIFT
    /* minus one to handle the last entry */
    sub \vend, \vend, #1
    and \vend, \vend, #ENTRIES_PER_TABLE - 1
    /* loop init, pa = pa | flags */
    lsr \pa, \pa, #SECTION_SHIFT
    lsl \pa, \pa, #SECTION_SHIFT
    ldr \tmp1, =\flags
    orr \pa, \pa, \tmp1
    /* loop */
    /* pmd[vstart] = pa */
2:
    str \pa, [\pmd, \vstart, lsl #3]
    /* pa += section size */
    add \pa, \pa, #SECTION_SIZE
    /* vstart += 1 */
    add \vstart, \vstart, #1
    cmp \vstart, \vend
    b.le 2b
.endm



.section ".text.boot"

#include "mm.h"

.globl _start
_start:
    /* only core 0 starts here */
    b master
    /* shouldn't come here */
    b proc_hang

#include "mm.h"

master: /* entry point of the primary core */
    bl drop_to_el1
    mov sp, #LOW_MEMORY
    /* adr is load address */
    adr x0, __bss_start
    adr x1, __bss_end
    sub x1, x1, x0
    /* clear out the bss section */
    /* branch and link, x30 is set to the address of the next instruction */

    bl memzero
    bl map_identity
    bl map_high
    bl wake_up_cores
    /* save kernel pa base */
    adr x0, _start
    adr x1, KERNEL_PA_BASE
    str x0, [x1]
    /* set ttbr's */
    adrp x0, id_pg_dir
    msr ttbr0_el1, x0
    adrp x0, high_pg_dir
    msr ttbr1_el1, x0
    /* turn on the mmu */
    mrs x0, sctlr_el1
    mov x1, #SCTLR_EL1_MMU_ENABLED
    orr x0, x0, x1
    msr sctlr_el1, x0
    /* prepare jumping to high mem */
    ldr x2, =LINEAR_MAP_BASE
    add sp, sp, x2
    adr x1, main
    add x1, x1, x2
    /* we are core 0 */
    mov x0, #0
    /* jump to high mem */
    blr x1
    /* shouldn't get here */
    b proc_hang

app: /* entry point of the secondary cores */
    bl drop_to_el1
    /* setup stack */
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    mov x1, #SECTION_SIZE
    mul x1, x1, x0
    add x1, x1, #LOW_MEMORY
    mov sp, x1
    // if x0 isnt 0
    bl main

drop_to_el1:
    adr x0, el1_entry
    msr ELR_EL3, x0
    eret
el1_entry:
    ret

wake_up_cores:
    sev
    mov x0, #0
    adr x0, app
    mov x1, #0xe0
    str x0, [x1]
    mov x1, #0xe8
    str x0, [x1]
    mov x1, #0xf0
    str x0, [x1]
    ret

map_identity:
    /* save return address */
    mov x29, x30
    adrp x0, id_pg_dir
    mov x1, #ID_MAP_TABLE_SIZE
    /* clear id page tables */
    bl memzero
    adrp x0, id_pg_dir
    /* x1 = address of id map pud */
    add x1, x0, #PAGE_SIZE
    eor x4, x4, x4
    /* install PGD entry */
    create_table_entry x0, x1, x4, PGD_SHIFT, TD_KERNEL_TABLE_FLAGS, x2, x3
    /* goto next level */
    add x0, x0, #PAGE_SIZE
    add x1, x1, #PAGE_SIZE
    /* install PUD entry */
    create_table_entry x0, x1, x4, PUD_SHIFT, TD_KERNEL_TABLE_FLAGS, x2, x3
    /* id map 0-16M */
    mov x0, x1
    eor x2, x2, x2
    ldr x3, =ID_MAP_SIZE
    eor x4, x4, x4
    create_block_map x0, x2, x3, x4, TD_KERNEL_BLOCK_FLAGS, x5
    
    /* restore return address */
    mov x30, x29
    ret

map_high:
    /* save return address */
    mov x29, x30
    adrp x0, high_pg_dir
    mov x1, #HIGH_MAP_TABLE_SIZE
    /* clear high page tables */
    bl memzero
    adrp x0, high_pg_dir
    /* x1 = address of high map pud */
    add x1, x0, #PAGE_SIZE
    /* x4 = address of va we map (pgd) */
    ldr x4, =LINEAR_MAP_BASE
    /* install PGD entry */
    create_table_entry x0, x1, x4, PGD_SHIFT, TD_KERNEL_TABLE_FLAGS, x2, x3
    /* goto next level */
    add x0, x0, #PAGE_SIZE
    add x1, x1, #PAGE_SIZE
    /* x4 = address of va we map (pud) */
    ldr x4, =LINEAR_MAP_BASE
    ldr x5, =PUD_ENTRY_MAP_SIZE
    /* install first PUD entry */
    create_table_entry x0, x1, x4, PUD_SHIFT, TD_KERNEL_TABLE_FLAGS, x2, x3
    add x1, x1, #PAGE_SIZE
    add x4, x4, x5
    create_table_entry x0, x1, x4, PUD_SHIFT, TD_KERNEL_TABLE_FLAGS, x2, x3
    add x1, x1, #PAGE_SIZE
    add x4, x4, x5
    create_table_entry x0, x1, x4, PUD_SHIFT, TD_KERNEL_TABLE_FLAGS, x2, x3
    add x1, x1, #PAGE_SIZE
    add x4, x4, x5
    create_table_entry x0, x1, x4, PUD_SHIFT, TD_KERNEL_TABLE_FLAGS, x2, x3
    /* load some values */
    ldr x10, =HIGH_MAP_FIRST_START
    ldr x11, =HIGH_MAP_FIRST_END
    ldr x12, =HIGH_MAP_SECOND_START
    ldr x13, =HIGH_MAP_SECOND_END
    ldr x14, =HIGH_MAP_THIRD_START
    ldr x15, =HIGH_MAP_THIRD_END
    ldr x16, =HIGH_MAP_FOURTH_START
    ldr x17, =HIGH_MAP_FOURTH_END
    ldr x18, =HIGH_MAP_DEVICE_START
    ldr x19, =HIGH_MAP_DEVICE_END
    ldr x20, =FIRST_START
    ldr x21, =SECOND_START
    ldr x22, =THIRD_START
    ldr x23, =FOURTH_START
    ldr x24, =DEVICE_START
    /* map first high part */
    add x0, x0, #PAGE_SIZE
    mov x2, x10
    mov x3, x11
    mov x4, x20
    create_block_map x0, x2, x3, x4, TD_KERNEL_BLOCK_FLAGS, x5
    /* map second high part */
    add x0, x0, #PAGE_SIZE
    mov x2, x12
    mov x3, x13
    mov x4, x21
    create_block_map x0, x2, x3, x4, TD_KERNEL_BLOCK_FLAGS, x5
    /* map third high part */
    add x0, x0, #PAGE_SIZE
    mov x2, x14
    mov x3, x15
    mov x4, x22
    create_block_map x0, x2, x3, x4, TD_KERNEL_BLOCK_FLAGS, x5
    /* map fourth high part */
    add x0, x0, #PAGE_SIZE
    mov x2, x16
    mov x3, x17
    mov x4, x23
    create_block_map x0, x2, x3, x4, TD_KERNEL_BLOCK_FLAGS, x5
    /* map device */
    mov x2, x18
    mov x3, x19
    mov x4, x24
    create_block_map x0, x2, x3, x4, TD_DEVICE_BLOCK_FLAGS, x5
    
    /* restore return address */
    mov x30, x29
    ret

proc_hang:
    /* wait for event */
    wfe
    b proc_hang



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

#include "OS.h"

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

    bl set_PGD_CURR        

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


.globl get_core
get_core:
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
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



// Generic_Timers.S
.globl get_sys_count
get_sys_count:
    mrs x0, CNTPCT_EL0
    ret

.globl set_CNTP_TVAL
set_CNTP_TVAL:
    msr CNTP_TVAL_EL0, x0
    ret

.globl setup_CNTP_CTL
setup_CNTP_CTL:
    mov x9, 1
    msr CNTP_CTL_EL0, x9
    ret


//Sched.S
.globl set_pgd
set_pgd:
    msr ttbr0_el1, x0
    /* tlb invalidate, vm all el1 inner sharable */
    tlbi vmalle1is
    /* data sync inner sharable */
    dsb ish
    /* instruction sync barrier, flushes pipeline */
    isb
    ret


.globl memzero
memzero:
    /* store 8 bytes of zero to [x0], then x0 += 8 */
    str xzr, [x0], #8
    subs x1, x1, #8
    /* branch if greater than zero */
    b.gt memzero
    ret
    
