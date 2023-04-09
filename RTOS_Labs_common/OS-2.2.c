// *************os.c**************
// EE445M/EE380L.6 Labs 1, 2, 3, and 4 
// High-level OS functions
// Students will implement these functions as part of Lab
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano 
// Jan 12, 2020, valvano@mail.utexas.edu


#include <stdint.h>
#include <stdio.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "../inc/PLL.h"
#include "../inc/LaunchPad.h"
#include "../inc/Timer4A.h"
#include "../inc/Timer5A.h"
#include "../inc/WTimer0A.h"
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Labs_common/ST7735.h"
#include "../inc/ADCT0ATrigger.h"
#include "../RTOS_Labs_common/UART0int.h"
#include "../RTOS_Labs_common/eFile.h"


// Performance Measurements 
int32_t MaxJitter;             // largest time jitter between interrupts in usec
#define JITTERSIZE 64
uint32_t const JitterSize=JITTERSIZE;
uint32_t JitterHistogram[JITTERSIZE]={0,};
uint32_t systemTime;

#define PREEMPTIVE

/*------------------------------------------------------------------------------
  Systick Interrupt Handler
  SysTick interrupt happens every 10 ms
  used for preemptive thread switch
 *------------------------------------------------------------------------------*/
void SysTick_Handler(void) {
  
	#ifdef PREEMPTIVE
	
		NVIC_INT_CTRL_R = 0x10000000;  //trigger pendsv   
	
	#endif
  
} // end SysTick_Handler

unsigned long OS_LockScheduler(void){
  // lab 4 might need this for disk formating
  return 0;// replace with solution
}
void OS_UnLockScheduler(unsigned long previous){
  // lab 4 might need this for disk formating
}


void SysTick_Init(unsigned long period){
  
}

#define NUM_THREADS 10
#define STACKSIZE 128
#define FIFOSIZE 4

uint32_t PutI;
uint32_t GetI;

uint32_t Fifo[FIFOSIZE];
Sema4Type CurrentSize;
uint32_t LostData;


struct TCB	{
	uint32_t* sp;
	struct TCB* next;
	//struct TCB* previous;
	uint32_t id;
	uint32_t sleep;
	uint8_t priority;
	uint32_t *blocked;
};

struct TCB TCBList[NUM_THREADS];

struct TCB *RunPt;

uint32_t Mail;	// shared data
Sema4Type Send;
Sema4Type Ack;

int8_t thread_count = 0, head = -1, tail = -1;

int threadVector[NUM_THREADS] = {0};	// One-hot vector

int free_thread(void)	{
	for(int i = 0; i < NUM_THREADS; i++)	{
		if(threadVector[i] == 0)
			return i;
	}
	
	return -1;
}


long stack[NUM_THREADS][STACKSIZE];

void SetInitialStack(int i)	{
	stack[i][STACKSIZE - 1] = 0x01000000; // Thumb bit
	//stack[i][STACKSIZE - 2] = PC;
	stack[i][STACKSIZE - 3] = 0x14141414; // R14
	stack[i][STACKSIZE - 4] = 0x12121212; // R12
	stack[i][STACKSIZE-5] = 0x03030303; // R3
	stack[i][STACKSIZE-6] = 0x02020202; // R2
	stack[i][STACKSIZE-7] = 0x01010101; // R1
	stack[i][STACKSIZE-8] = 0x00000000; // R0
	stack[i][STACKSIZE-9] = 0x11111111; // R11
	stack[i][STACKSIZE-10] = 0x10101010; // R10
	stack[i][STACKSIZE-11] = 0x09090909; // R9
	stack[i][STACKSIZE-12] = 0x08080808; // R8
	stack[i][STACKSIZE-13] = 0x07070707; // R7
	stack[i][STACKSIZE-14] = 0x06060606; // R6
	stack[i][STACKSIZE-15] = 0x05050505; // R5
	stack[i][STACKSIZE-16] = 0x04040404; // R4
}


/**
 * @details  Initialize operating system, disable interrupts until OS_Launch.
 * Initialize OS controlled I/O: serial, ADC, systick, LaunchPad I/O and timers.
 * Interrupts not yet enabled.
 * @param  none
 * @return none
 * @brief  Initialize OS
 */
void OS_Init(void){
  // put Lab 2 (and beyond) solution here
	
	OS_ClearMsTime();
}; 

// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, int32_t value){
  // put Lab 2 (and beyond) solution here

	semaPt->Value = value;
	
}; 

// ******** OS_Wait ************
// decrement semaphore 
// Lab2 spinlock
// Lab3 block if less than zero
// input:  pointer to a counting semaphore
// output: none
void OS_Wait(Sema4Type *semaPt){
  // put Lab 2 (and beyond) solution here
	
	DisableInterrupts();
	
	while(semaPt->Value <= 0)	{
		EnableInterrupts();
		DisableInterrupts();
	}
	
	semaPt->Value = semaPt->Value - 1;
	EnableInterrupts();
  
}; 

// ******** OS_Signal ************
// increment semaphore 
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt){
  // put Lab 2 (and beyond) solution here

	long status;
	status = StartCritical();
	semaPt->Value = semaPt->Value + 1;
	EndCritical(status);
}; 

// ******** OS_bWait ************
// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input:  pointer to a binary semaphore
// output: none
void OS_bWait(Sema4Type *semaPt){
  // put Lab 2 (and beyond) solution here
	
	DisableInterrupts();
	
	while(semaPt->Value <= 0)	{
		EnableInterrupts();
		DisableInterrupts();
	}
	
	semaPt->Value = 0;
	EnableInterrupts();

}; 

// ******** OS_bSignal ************
// Lab2 spinlock, set to 1
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a binary semaphore
// output: none
void OS_bSignal(Sema4Type *semaPt){
  // put Lab 2 (and beyond) solution here

	long status;
	status = StartCritical();
	semaPt->Value = 1;
	EndCritical(status);
	
}; 




#define ROUND_ROBIN_SCHEDULER

//******** OS_AddThread *************** 
// add a foregound thread to the scheduler
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields
int OS_AddThread(void(*task)(void), 
   uint32_t stackSize, uint32_t priority){
  // put Lab 2 (and beyond) solution here
		 
		 // Check whether stackSize is divisible by 8 or max thread count not reached, otherwise error
		 if((stackSize % 8 !=0) || (thread_count >= NUM_THREADS))
			 return 0;
		 
		 #ifdef ROUND_ROBIN_SCHEDULER
		 
		 int32_t status;
		 status = StartCritical();
		 
		 if(RunPt == NULL)	{
			 // Add thread to round robin 
			 
			 threadVector[0] = 1;
			 head = 0;
			 tail = 0; 
			 
			 RunPt = &TCBList[0];
			 
			 SetInitialStack(0);
			 stack[0][STACKSIZE - 2] = (int32_t)(task);	// PC;
			 RunPt->sp = &stack[0][STACKSIZE - 16];	// set SP for task stack
			 RunPt->id = 0;
			 
			  
		 }
		 
		 else {
			 
			 
			 
			 int next_thread = free_thread();
			 
			 // If all TCBs full
			 if(next_thread == -1)
				 return 0;
			 
			 
			 tail = next_thread; 	// update tail index
			 RunPt->next = &TCBList[next_thread];	// Set next of current TCB to new TCB
			 threadVector[next_thread] = 1;	// Update vector to indicate TCB occupied
			 
			 RunPt = RunPt->next;	// Go to the new TCB
			 
			 RunPt->next = &TCBList[head];	// Set next of new TCB as first TCB; round-robin scheduling
			 
			 SetInitialStack(next_thread);
			 stack[next_thread][STACKSIZE - 2] = (int32_t)task;
			 RunPt->sp = &stack[next_thread][STACKSIZE - 16];	// set SP for task stack
			 RunPt->id = next_thread;
		 }
		 
		 EndCritical(status);
		 

		 
		 #endif
		 
		 return 1;
		 
  //return 0; // replace this line with solution
};

//******** OS_AddProcess *************** 
// add a process with foregound thread to the scheduler
// Inputs: pointer to a void/void entry point
//         pointer to process text (code) segment
//         pointer to process data segment
//         number of bytes allocated for its stack
//         priority (0 is highest)
// Outputs: 1 if successful, 0 if this process can not be added
// This function will be needed for Lab 5
// In Labs 2-4, this function can be ignored
int OS_AddProcess(void(*entry)(void), void *text, void *data, 
  unsigned long stackSize, unsigned long priority){
  // put Lab 5 solution here

     
  return 0; // replace this line with Lab 5 solution
}


//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
uint32_t OS_Id(void){
  // put Lab 2 (and beyond) solution here
  
  return RunPt->id; // replace this line with solution
};


//******** OS_AddPeriodicThread *************** 
// add a background periodic task
// typically this function receives the highest priority
// Inputs: pointer to a void/void background function
//         period given in system time units (12.5ns)
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// You are free to select the time resolution for this function
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal   OS_AddThread
// This task does not have a Thread ID
// In lab 1, this command will be called 1 time
// In lab 2, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, this command will be called 0 1 or 2 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddPeriodicThread(void(*task)(void), 
   uint32_t period, uint32_t priority){
  // put Lab 2 (and beyond) solution here
  
     
  return 0; // replace this line with solution
};


/*----------------------------------------------------------------------------
  PF1 Interrupt Handler
 *----------------------------------------------------------------------------*/
void GPIOPortF_Handler(void){
 
}

//******** OS_AddSW1Task *************** 
// add a background task to run whenever the SW1 (PF4) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal   OS_AddThread
// This task does not have a Thread ID
// In labs 2 and 3, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSW1Task(void(*task)(void), uint32_t priority){
  // put Lab 2 (and beyond) solution here
 
  return 0; // replace this line with solution
};

//******** OS_AddSW2Task *************** 
// add a background task to run whenever the SW2 (PF0) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is highest, 5 is lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed user task will run to completion and return
// This task can not spin block loop sleep or kill
// This task can call issue OS_Signal, it can call OS_AddThread
// This task does not have a Thread ID
// In lab 2, this function can be ignored
// In lab 3, this command will be called will be called 0 or 1 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSW2Task(void(*task)(void), uint32_t priority){
  // put Lab 2 (and beyond) solution here
    
  return 0; // replace this line with solution
};


// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(uint32_t sleepTime){
  // put Lab 2 (and beyond) solution here
  
	RunPt->sleep = sleepTime;

};  

// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void){
  // put Lab 2 (and beyond) solution here
	
	
	// Update Linked List
	
	struct TCB *ptr1, *ptr2;
	ptr1 = &TCBList[head];
	ptr2 = ptr1->next;
	
	while(ptr2->id != RunPt->id)	{
		ptr1 = ptr2;
		ptr2 = ptr2->next;
	}
	
	ptr1->next = ptr2->next;	// Remove TCB from linked list
	
	threadVector[RunPt->id] = 0;
	SetInitialStack(RunPt->id);
 
	
	
	
  EnableInterrupts();   // end of atomic section 
  for(;;){};        // can not return
    
}; 

void ContextSwitch(void);

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking 
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void){
  // put Lab 2 (and beyond) solution here
  
	
	// Should call Context switch
	NVIC_INT_CTRL_R = 0x10000000; // trigger PendSV
	
	
};
  
// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(uint32_t size){
  // put Lab 2 (and beyond) solution here
  
	PutI = 0;
	GetI = 0;
	OS_InitSemaphore(&CurrentSize, 0);
	LostData = 0;
	
  
};

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int OS_Fifo_Put(uint32_t data){
  // put Lab 2 (and beyond) solution here

		if(CurrentSize.Value == FIFOSIZE)	{
			LostData++;
			return 0;
		}	else {
			Fifo[PutI] = data;
			PutI = (PutI + 1)%FIFOSIZE;
			OS_Signal(&CurrentSize);
			return 1;
			
		}
			
    //return 0; // replace this line with solution
};  

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
uint32_t OS_Fifo_Get(void){
  // put Lab 2 (and beyond) solution here
  
	uint32_t data;
	OS_Wait(&CurrentSize);
	data = Fifo[GetI];
	GetI = (GetI + 1)%FIFOSIZE;
	return data;
	
	
  //return 0; // replace this line with solution
};

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
int32_t OS_Fifo_Size(void){
  // put Lab 2 (and beyond) solution here
   
  return 0; // replace this line with solution
};


// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void){
  // put Lab 2 (and beyond) solution here
  
	OS_InitSemaphore(&Send, 0);
	OS_InitSemaphore(&Ack, 0);

  // put solution here
};

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(uint32_t data){
  // put Lab 2 (and beyond) solution here
  // put solution here
  
	Mail = data;
	
	OS_Signal(&Send);
	
	OS_Wait(&Ack);

};

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
uint32_t OS_MailBox_Recv(void){
  // put Lab 2 (and beyond) solution here
 
	uint32_t recvData;
	
	OS_Wait(&Send);
	
	recvData = Mail;
	
	OS_Signal(&Ack);
	
	return recvData;
  //return 0; // replace this line with solution
};

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
uint32_t OS_Time(void){
  // put Lab 2 (and beyond) solution here

  return 0; // replace this line with solution
};

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
uint32_t OS_TimeDifference(uint32_t start, uint32_t stop){
  // put Lab 2 (and beyond) solution here

  return 0; // replace this line with solution
};



void update_time(void)	{
	systemTime++;
}


// ******** OS_ClearMsTime ************
// sets the system time to zero (solve for Lab 1), and start a periodic interrupt
// Inputs:  none
// Outputs: none
// You are free to change how this works
void OS_ClearMsTime(void){
  // put Lab 1 solution here
	
	systemTime = 0;
	Timer5A_Init(&update_time, 80000000/1000, 0);

};

// ******** OS_MsTime ************
// reads the current time in msec (solve for Lab 1)
// Inputs:  none
// Outputs: time in ms units
// You are free to select the time resolution for this function
// For Labs 2 and beyond, it is ok to make the resolution to match the first call to OS_AddPeriodicThread
uint32_t OS_MsTime(void){
  // put Lab 1 solution here
	return systemTime; // replace this line with solution
};


//******** OS_Launch *************** 
// start the scheduler, enable interrupts
// Inputs: number of 12.5ns clock cycles for each time slice
//         you may select the units of this parameter
// Outputs: none (does not return)
// In Lab 2, you can ignore the theTimeSlice field
// In Lab 3, you should implement the user-defined TimeSlice field
// It is ok to limit the range of theTimeSlice to match the 24-bit SysTick
void OS_Launch(uint32_t theTimeSlice){
  // put Lab 2 (and beyond) solution here
  
	STCTRL = 0; // disable SysTick during setup
	STCURRENT = 0; // any write to current clears it
	SYSPRI3 =(SYSPRI3&0x00FFFFFF)|0xE0000000; // priority 7
	STRELOAD = theTimeSlice - 1; // reload value
	STCTRL = 0x00000007; // enable, core clock and interrupt arm
	StartOS();
    
};

//************** I/O Redirection *************** 
// redirect terminal I/O to UART or file (Lab 4)

int StreamToDevice=0;                // 0=UART, 1=stream to file (Lab 4)

int fputc (int ch, FILE *f) { 
  if(StreamToDevice==1){  // Lab 4
    if(eFile_Write(ch)){          // close file on error
       OS_EndRedirectToFile(); // cannot write to file
       return 1;                  // failure
    }
    return 0; // success writing
  }
  
  // default UART output
  UART_OutChar(ch);
  return ch; 
}

int fgetc (FILE *f){
  char ch = UART_InChar();  // receive from keyboard
  UART_OutChar(ch);         // echo
  return ch;
}

int OS_RedirectToFile(const char *name){  // Lab 4
  eFile_Create(name);              // ignore error if file already exists
  if(eFile_WOpen(name)) return 1;  // cannot open file
  StreamToDevice = 1;
  return 0;
}

int OS_EndRedirectToFile(void){  // Lab 4
  StreamToDevice = 0;
  if(eFile_WClose()) return 1;    // cannot close file
  return 0;
}

int OS_RedirectToUART(void){
  StreamToDevice = 0;
  return 0;
}

int OS_RedirectToST7735(void){
  
  return 1;
}

