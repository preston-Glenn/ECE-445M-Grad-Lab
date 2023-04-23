
#include "mm.h"
#include "mmu.h"
#include "sysregs.h"

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




	bl drop_to_el1
    mov sp, #LOW_MEMORY
	/* TODO: IDK if sp should be _start or LOW_MEMORY */

    bl map_identity
    bl map_high
    /* bl wake_up_cores */
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
    adr x1, kernel_main
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
    bl kernel_main

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


#include "entry.h"

	.macro handle_invalid_entry type
	kernel_entry
	mov	x0, #\type
	mrs	x1, esr_el1
	mrs	x2, elr_el1
	bl	show_invalid_entry_message
	b	err_hang
	.endm

	.macro	ventry	label
	.align	7
	b	\label
	.endm

	.macro	kernel_entry
	sub	sp, sp, #S_FRAME_SIZE
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	stp	x16, x17, [sp, #16 * 8]
	stp	x18, x19, [sp, #16 * 9]
	stp	x20, x21, [sp, #16 * 10]
	stp	x22, x23, [sp, #16 * 11]
	stp	x24, x25, [sp, #16 * 12]
	stp	x26, x27, [sp, #16 * 13]
	stp	x28, x29, [sp, #16 * 14]
	str	x30, [sp, #16 * 15] 
	.endm

	.macro	kernel_exit
	ldp	x0, x1, [sp, #16 * 0]
	ldp	x2, x3, [sp, #16 * 1]
	ldp	x4, x5, [sp, #16 * 2]
	ldp	x6, x7, [sp, #16 * 3]
	ldp	x8, x9, [sp, #16 * 4]
	ldp	x10, x11, [sp, #16 * 5]
	ldp	x12, x13, [sp, #16 * 6]
	ldp	x14, x15, [sp, #16 * 7]
	ldp	x16, x17, [sp, #16 * 8]
	ldp	x18, x19, [sp, #16 * 9]
	ldp	x20, x21, [sp, #16 * 10]
	ldp	x22, x23, [sp, #16 * 11]
	ldp	x24, x25, [sp, #16 * 12]
	ldp	x26, x27, [sp, #16 * 13]
	ldp	x28, x29, [sp, #16 * 14]
	ldr	x30, [sp, #16 * 15] 
	add	sp, sp, #S_FRAME_SIZE		
	eret
	.endm


/*
 * Exception vectors.
 */
.align	11
.globl vectors 
vectors:
	ventry	sync_invalid_el1t			// Synchronous EL1t
	ventry	irq_invalid_el1t			// IRQ EL1t
	ventry	fiq_invalid_el1t			// FIQ EL1t
	ventry	error_invalid_el1t			// Error EL1t

	ventry	sync_invalid_el1h			// Synchronous EL1h
	ventry	el1_irq					// IRQ EL1h
	ventry	fiq_invalid_el1h			// FIQ EL1h
	ventry	error_invalid_el1h			// Error EL1h

	ventry	sync_invalid_el0_64			// Synchronous 64-bit EL0
	ventry	irq_invalid_el0_64			// IRQ 64-bit EL0
	ventry	fiq_invalid_el0_64			// FIQ 64-bit EL0
	ventry	error_invalid_el0_64			// Error 64-bit EL0

	ventry	sync_invalid_el0_32			// Synchronous 32-bit EL0
	ventry	irq_invalid_el0_32			// IRQ 32-bit EL0
	ventry	fiq_invalid_el0_32			// FIQ 32-bit EL0
	ventry	error_invalid_el0_32			// Error 32-bit EL0

sync_invalid_el1t:
	handle_invalid_entry  SYNC_INVALID_EL1t

irq_invalid_el1t:
	handle_invalid_entry  IRQ_INVALID_EL1t

fiq_invalid_el1t:
	handle_invalid_entry  FIQ_INVALID_EL1t

error_invalid_el1t:
	handle_invalid_entry  ERROR_INVALID_EL1t

sync_invalid_el1h:
	handle_invalid_entry  SYNC_INVALID_EL1h

fiq_invalid_el1h:
	handle_invalid_entry  FIQ_INVALID_EL1h

error_invalid_el1h:
	handle_invalid_entry  ERROR_INVALID_EL1h

sync_invalid_el0_64:
	handle_invalid_entry  SYNC_INVALID_EL0_64

irq_invalid_el0_64:
	handle_invalid_entry  IRQ_INVALID_EL0_64

fiq_invalid_el0_64:
	handle_invalid_entry  FIQ_INVALID_EL0_64

error_invalid_el0_64:
	handle_invalid_entry  ERROR_INVALID_EL0_64

sync_invalid_el0_32:
	handle_invalid_entry  SYNC_INVALID_EL0_32

irq_invalid_el0_32:
	handle_invalid_entry  IRQ_INVALID_EL0_32

fiq_invalid_el0_32:
	handle_invalid_entry  FIQ_INVALID_EL0_32

error_invalid_el0_32:
	handle_invalid_entry  ERROR_INVALID_EL0_32

el1_irq:
	kernel_entry 
	bl	handle_irq
	kernel_exit 

.globl err_hang
err_hang: b err_hang



.globl irq_vector_init
irq_vector_init:
	adr	x0, vectors		// load VBAR_EL1 with virtual
	msr	vbar_el1, x0		// vector table address
	ret

.globl enable_irq
enable_irq:
	msr    daifclr, #2 
	ret

.globl disable_irq
disable_irq:
	msr	daifset, #2
	ret


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
.globl get_core
get_core:
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    ret

// generic timer.S
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


// irq.S
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


.globl memzero
memzero:
    /* store 8 bytes of zero to [x0], then x0 += 8 */
    str xzr, [x0], #8
    subs x1, x1, #8
    /* branch if greater than zero */
    b.gt memzero
    ret
    


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