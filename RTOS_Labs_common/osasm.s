;/*****************************************************************************/
;/* OSasm.s: low-level OS commands, written in assembly                       */
;/* derived from uCOS-II                                                      */
;/*****************************************************************************/
;Jonathan Valvano, OS Lab2/3/4/5, 1/12/20
;Students will implement these functions as part of EE445M/EE380L.12 Lab

        AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB
        REQUIRE8
        PRESERVE8

        EXTERN  currThread            ; currently running thread

        EXPORT  StartOS
        EXPORT  PendSV_Handler
        EXPORT  SVC_Handler

NVIC_INT_CTRL   EQU     0xE000ED04                              ; Interrupt control state register.
NVIC_SYSPRI14   EQU     0xE000ED22                              ; PendSV priority register (position 14).
NVIC_SYSPRI15   EQU     0xE000ED23                              ; Systick priority register (position 15).
NVIC_LEVEL14    EQU           0xEF                              ; Systick priority value (second lowest).
NVIC_LEVEL15    EQU           0xFF                              ; PendSV priority value (lowest).
NVIC_PENDSVSET  EQU     0x10000000                              ; Value to trigger PendSV exception.



StartOS
; put your code here
    LDR R0, =currThread             ;   get address of current thread
    LDR R1, [R0]                    ;   R1 = currThread
    LDR SP, [R1]                      ;   SP = currThread{0} ; {0}->first item which is SP
    POP {R4-R11}
    POP {R0-R3}
    POP {R12}
    POP {LR}                   ; pop LR
    POP {LR}                   ; assign PC to LR
	POP {R1}				   ; dump last one

    CPSIE I                     ; enable ints
    BX      LR                 ; start first thread

OSStartHang
    B       OSStartHang        ; Should never get here


;********************************************************************************************************
;                               PERFORM A CONTEXT SWITCH (From task level)
;                                           void ContextSwitch(void)
;
; Note(s) : 1) ContextSwitch() is called when OS wants to perform a task context switch.  This function
;              triggers the PendSV exception which is where the real work is done.
;********************************************************************************************************

;ContextSwitch
;    LDR R0, =0xE000ED04   ; load the address of the INTCTRL register into R0
;    LDR R1, [R0]         ; load the current value of the INTCTRL register into R1
;    ORR R1, R1, #0x1000000 ; set the PendSV bit by ORing with 0x1000000
;    STR R1, [R0]         ; store the new value of the INTCTRL register

; trigger pendsv, and have both systick and os_sus call contextswitch


    

;********************************************************************************************************
;                                         HANDLE PendSV EXCEPTION
;                                     void OS_CPU_PendSVHandler(void)
;
; Note(s) : 1) PendSV is used to cause a context switch.  This is a recommended method for performing
;              context switches with Cortex-M.  This is because the Cortex-M3 auto-saves half of the
;              processor context on any exception, and restores same on return from exception.  So only
;              saving of R4-R11 is required and fixing up the stack pointers.  Using the PendSV exception
;              this way means that context saving and restoring is identical whether it is initiated from
;              a thread or occurs due to an interrupt or exception.
;
;           2) Pseudo-code is:
;              a) Get the process SP, if 0 then skip (goto d) the saving part (first context switch);
;              b) Save remaining regs r4-r11 on process stack;
;              c) Save the process SP in its TCB, OSTCBCur->OSTCBStkPtr = SP;
;              d) Call OSTaskSwHook();
;              e) Get current high priority, OSPrioCur = OSPrioHighRdy;
;              f) Get current ready thread TCB, OSTCBCur = OSTCBHighRdy;
;              g) Get new process SP from TCB, SP = OSTCBHighRdy->OSTCBStkPtr;
;              h) Restore R4-R11 from new process stack;
;              i) Perform exception return which will restore remaining context.
;
;           3) On entry into PendSV handler:
;              a) The following have been saved on the process stack (by processor):
;                 xPSR, PC, LR, R12, R0-R3
;              b) Processor mode is switched to Handler mode (from Thread mode)
;              c) Stack is Main stack (switched from Process stack)
;              d) OSTCBCur      points to the OS_TCB of the task to suspend
;                 OSTCBHighRdy  points to the OS_TCB of the task to resume
;
;           4) Since PendSV is set to lowest priority in the system (by OSStartHighRdy() above), we
;              know that it will only be run when no other exception or interrupt is active, and
;              therefore safe to assume that context being switched out was using the process stack (PSP).
;********************************************************************************************************

		IMPORT Scheduler


PendSV_Handler
; edit this code
    ;// context switch for two

    CPSID   I         ;     disaable interrupts
    PUSH {R4-R11}     ;     push  registers to stack
    LDR R0,=currThread;     load address of currThread to R0
    LDR R1, [R0]      ;     load currThread object into R1
    STR SP, [R1]      ;     save SP in currThread (sp is first 4 bytes in currThread)

    ; eventually call BL with Scheduler here or have it be its own thread
    ; for now go to currThread.next
    ;    LDR R2, [R1,#4]      ;     load currThread.next ptr into R1
    ;    STR R2, [R0]         ;     store currThread.next into MEM @R0 (currThread)
    PUSH {R0,LR}
    BL Scheduler    
    POP {R0,LR}
    LDR R1, [R0]

	LDR SP, [R1]      	 ;     load new currTHread into into SP
   
	POP {R4-R11}  		 ;     pop context off of stack

    CPSIE   I         ;     enable interrupts
    BX LR             ;     begin new thread

 
    

;********************************************************************************************************
;                                         HANDLE SVC EXCEPTION
;                                     void OS_CPU_SVCHandler(void)
;
; Note(s) : SVC is a software-triggered exception to make OS kernel calls from user land. 
;           The function ID to call is encoded in the instruction itself, the location of which can be
;           found relative to the return address saved on the stack on exception entry.
;           Function-call paramters in R0..R3 are also auto-saved on stack on exception entry.
;********************************************************************************************************

        IMPORT    OS_Id
        IMPORT    OS_Kill
        IMPORT    OS_Sleep
        IMPORT    OS_Time
        IMPORT    OS_AddThread

SVC_Handler
; put your Lab 5 code here

    LDR R12,[SP,#24] ; Return address
    LDRH R12,[R12,#-2] ; SVC instruction is 2 bytes
    BIC R12,#0xFF00 ; Extract trap number in R12
    LDM SP,{R0-R3} ; Get any parameters
    PUSH {LR} ; Save LR value on the stack


;    // If R12 is 0, call OS_Id
;    // If R12 is 1, call OS_Kill
;   // If R12 is 2, call OS_Sleep
;    // If R12 is 3, call OS_Time
;    // If R12 is 4, call OS_AddThread
    CMP R12,#0 ; Compare R12 to 0
    BEQ ID ; Branch if equal to

    CMP R12,#1 ; Compare R12 to 1
    BEQ KILL ; Branch if equal to
    
    CMP R12,#2 ; Compare R12 to 2
    BEQ SLEEP ; Branch if equal to
    
    CMP R12,#3 ; Compare R12 to 3
    BEQ TIME ; Branch if equal to
    
    CMP R12,#4 ; Compare R12 to 4
    BEQ ADDTHREAD ; Branch if equal to

    B FINISH ; Jump to finish
    
ID    
    BL OS_Id ; Call OS routine by number
    B FINISH ; Jump to finish
KILL
    BL OS_Kill ; Call OS routine by number
    B FINISH ; Jump to finish
SLEEP
    BL OS_Sleep ; Call OS routine by number
    B FINISH ; Jump to finish
TIME
    BL OS_Time ; Call OS routine by number
    B FINISH ; Jump to finish
ADDTHREAD
    BL OS_AddThread ; Call OS routine by number
   

FINISH
    POP {LR} ; Restore LR
    STR R0,[SP] ; Store return value
    BX LR ; Return from exception





    ALIGN
    END