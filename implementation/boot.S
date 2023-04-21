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