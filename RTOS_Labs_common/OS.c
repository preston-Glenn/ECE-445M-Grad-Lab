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
#include "../inc/Timer1A.h"
#include "../inc/Timer2A.h"
#include "../inc/Timer3A.h"
#include "../inc/Timer4A.h"
#include "../inc/Timer5A.h"
#include "../inc/WTimer0A.h"
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Labs_common/ST7735.h"
#include "../inc/ADCT0ATrigger.h"
#include "../RTOS_Labs_common/UART0int.h"
#include "../RTOS_Labs_common/eFile.h"
#include "../RTOS_Labs_common/eFile.h"
#include "../inc/SysTick.h"
#include "../RTOS_Labs_common/ADC.h"
#include "../RTOS_Labs_common/heap.h"
#include "../inc/LaunchPad.h"
#include "../RTOS_Labs_common/EdgeInterruptPortF.h"

#include <string.h>
#include <stdio.h>

#define PD0  (*((volatile uint32_t *)0x40007004))
#define PD1  (*((volatile uint32_t *)0x40007008))
#define PD2  (*((volatile uint32_t *)0x40007010))
#define PD3  (*((volatile uint32_t *)0x40007020))

#define STACKSIZE 400 // number of 32-bit words in stack
#define NUMTHREADS 10 // maximum number of threads
#define NUM_PROCESSES 8
#define MAX_PRIORITY_PLUS_ONE 6

uint32_t time_slice;


// Set to 1 for preemptive scheduling, 0 for cooperative scheduling
const int PREEMPTIVE = 1;

#define PRIORITY_SCHEDULING 1 // else RoundRobin
//#define SPINLOCK // else BLOCKING
#define FIFOSIZE 32



int32_t STACK[NUMTHREADS][STACKSIZE];
TCB TCB_array[NUMTHREADS];

PCB PCB_array[NUM_PROCESSES];

TCBptr currThread;

int thread_count = 0; // track number of threads in currThread LL
int thread_id = 0;    // used to make TCB id

TCBptr TCB_priority_array[6];

char *NAME = "NEW_THREAD";

// Performance Measurements
int32_t MaxJitter; // largest time jitter between interrupts in usec
#define JITTERSIZE 64
uint32_t const JitterSize = JITTERSIZE;
uint32_t JitterHistogram[JITTERSIZE] = {
    0,
};
int32_t MaxJitter2; // largest time jitter between interrupts in usec
uint32_t const JitterSize2 = JITTERSIZE;
uint32_t JitterHistogram2[JITTERSIZE] = {
    0,
};
uint32_t time;


// OS fifo stuff
uint32_t PutI;
uint32_t GetI;

uint32_t Fifo[FIFOSIZE];
Sema4Type CurrentSize;
uint32_t LostData;

// OS mailbox stuff
uint32_t Mail; // shared data
Sema4Type BoxFree;
Sema4Type BoxDataValid;
#define _DEBUG 1

/**==============================================
 * *                   prepare dead threads
 * 
 * @brief sets all threads to DEAD status
 * 
 * @param void
 * 
 * @return void
 *
 *=============================================**/

void prep_dead_threads()
{
  int i = 0;

  for (i = 0; i < NUMTHREADS; i++)
  {
    TCB_array[i].status = DEAD;
    TCB_array[i].id = i;
  }
}

/**==============================================
 *                getNextDeadThread
 * 
 * @brief returns pointer to next dead thread 
 * 
 * @param void
 * 
 * @return TCBptr
 * 
 * @note returns NULL if no dead threads
 * @note used in OS_AddThread
 * @note  
 *  
 *  
 *=============================================**/

TCBptr getNextDeadThread()
{

  int i;
  for (i = 0; i < NUMTHREADS; i++)
  {
    if (TCB_array[i].status == DEAD)
    {
      return &TCB_array[i];
    }
  }
  return NULL;
}

// getNextAliveThread; start at currthread id and wrap around to it

/**==============================================
 **              getNextAliveThread
 *? returns pointer to next alive thread 
 *@param void    
 *@return TCBptr
 *
 * @note returns NULL if no alive threads
 * @note only used if PREEMPTIVE == 0
 *
 *=============================================**/


TCBptr getNextAliveThread()
{
  int i = currThread->id + 1;
  while (i % NUMTHREADS != currThread->id)
  {
    if (TCB_array[i % NUMTHREADS].status == ALIVE)
    {
      return &TCB_array[i % NUMTHREADS];
    }
    i++;
  }
  return currThread;
}

// getNextPCB
PCBptr getNextPCB()
{
  int i;
  for (i = 0; i < NUM_PROCESSES; i++)
  {
    if (PCB_array[i].num_threads == 0)
    {
      return &PCB_array[i];
    }
  }
  return NULL;
}

/*------------------------------------------------------------------------------
  Systick Interrupt Handler
  SysTick interrupt happens every 10 ms
  used for preemptive thread switch
 *------------------------------------------------------------------------------*/
void SysTick_Handler(void)
{

  // decrement sleep counter for sleeping threads
  // this may need to be its own timer :: CHECK
  DisableInterrupts();


  // here we decrement the sleep counter for sleeping threads
  //todo  make this its own timer or do something where we dont have to loop through all threads 
  int i = 0;
  for (i = 0; i < NUMTHREADS; i++)
  {
    if (TCB_array[i].sleep > 0 && TCB_array[i].status == SLEEPING)
    {
      TCB_array[i].sleep--;
    }
    if (TCB_array[i].sleep == 0 && TCB_array[i].status == SLEEPING)
    {
      TCB_array[i].status = ALIVE;
    }
  }

  EnableInterrupts();
  if (PREEMPTIVE)
    NVIC_INT_CTRL_R = 0x10000000; // trigger pendsv
} // end SysTick_Handler

unsigned long OS_LockScheduler(void)
{
  // lab 4 might need this for disk formating
  return 0; // replace with solution
}
void OS_UnLockScheduler(unsigned long previous)
{
  // lab 4 might need this for disk formating
}

// leaves systick disabled
void SysTick_Init_Period(unsigned long period)
{
  NVIC_ST_CTRL_R = 0;                            // disable SysTick during setup
  NVIC_ST_CURRENT_R = 0;                         // any write to current clears it
  SYSPRI3 = (SYSPRI3 & 0x00FFFFFF) | 0xD0000000; // priority 6
}

void SysTick_Enable()
{
  NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE + NVIC_ST_CTRL_CLK_SRC + NVIC_ST_CTRL_INTEN;
}

void SysTick_Disable()
{
  NVIC_ST_CTRL_R = 0; // disable SysTick during setup
}

/**
 * @details  Initialize operating system, disable interrupts until OS_Launch.
 * Initialize OS controlled I/O: serial, ADC, systick, LaunchPad I/O and timers.
 * Interrupts not yet enabled.
 * @param  none
 * @return none
 * @brief  Initialize OS
 */
void OS_Init(void)
{
  // put Lab 2 (and beyond) solution here

  // disable interrupts
  DisableInterrupts();

  ST7735_InitR(INITR_REDTAB); // LCD initialization
  UART_Init();

  // init systick
  PLL_Init(Bus80MHz);
	
	LaunchPad_Init();

  // init timer
  OS_ClearMsTime();

  // init Launchpad IO
  // ST7735_InitR(INITR_REDTAB); // LCD initialization
  SysTick_Init_Period(80000000 / 1000);
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R & 0x00FFFFFF) | 0xD0000000; // priority 7

  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R & 0xFF00FFFF) | 0x00E00000; // priority 6

  Heap_Init();

  SysTick_Disable();
  prep_dead_threads();
}

// ******** OS_InitSemaphore ************
// initialize semaphore
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, int32_t value)
{
  // put Lab 2 (and beyond) solution here
  semaPt->Value = value;
	semaPt->blocked = NULL;
};

// Implement Peterson's algorithm to solve the mutual exclusion problem instead of disabling interrupts
// ******** OS_Wait ************
// decrement semaphore
// Lab2 spinlock
// Lab3 block if less than zero
// input:  pointer to a counting semaphore
// output: none
void OS_Wait(Sema4Type *semaPt)
{
  // put Lab 2 (and beyond) solution here
	 //Lab 2 Implementation
	#ifdef SPINLOCK
  DisableInterrupts();
  while (semaPt->Value <= 0)
  {
    EnableInterrupts();
    OS_Suspend(); // run thread switcher
    DisableInterrupts();
  }
  semaPt->Value = semaPt->Value - 1;
  EnableInterrupts();
	
	#else
	// Lab 3 Implementation
	int status = StartCritical();
		
	semaPt->Value = semaPt->Value - 1;
	
  // If semaphore is less than 0, block the current thread :: do this by 
  // inserting the current thread into the blocked linked list of the semaphore
  // and setting the status of the current thread to BLOCKED
	if(semaPt->Value < 0)	{
		
		currThread->status = BLOCKED;
		removeThreadPriority(currThread);
		

    // if none blocked, insert at the start of the blocked linked list
		if(semaPt->blocked == NULL)	{
			semaPt->blocked = currThread;
			currThread->next = NULL;
			currThread->prev = NULL;
			// Check what to do with previous
		}
		else	{
			
			// If highest priority, insert at the start of the blocked linked list
			if(currThread->priority < semaPt->blocked->priority)	{
				currThread->next = semaPt->blocked;
				semaPt->blocked->prev = currThread;
				semaPt->blocked = currThread;
				semaPt->blocked->prev = NULL; // Check whether correct
			}	else	{ // Not highest priority, insert in the correct position in the blocked linked list based on priority
				TCBptr temp;
				temp = semaPt->blocked;
				int inserted = 0;
      
				while(temp->next != NULL)	{
          // If the priority of the current thread is less than the priority of the next thread in the blocked linked list
          // and the priority of the current thread is greater than the priority of the previous thread in the blocked linked list
					if(temp->priority <= currThread->priority && temp->next->priority > currThread->priority)	{
						TCBptr save;
						
						save = temp->next;
						temp->next = currThread;
						
						currThread->next = save;
						currThread->prev = temp;
						
						save->prev = currThread;
						
						inserted = 1;
						break; // Break out of the while loop
						
					}
					temp = temp->next;
				}
				
        // If the current thread has not been inserted, insert it at the end of the blocked linked list
				if(!inserted)	{
					temp->next = currThread;
					currThread->next = NULL;
					currThread->prev = temp;
				}
				
			}
			
		}
		
		EndCritical(status);
		OS_Suspend();
	}

  semaPt->running = currThread;
	
	
	EndCritical(status);
	
	#endif
};

// ******** OS_Signal ************
// increment semaphore
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt)
{
  // put Lab 2 (and beyond) solution here
	#ifdef SPINLOCK
  long critical = StartCritical();
  semaPt->Value = semaPt->Value + 1;
  EndCritical(critical);
	
	#else
	long status;

	status = StartCritical();

	semaPt->Value = semaPt->Value + 1;
  TCBptr b_thread = semaPt->blocked;

	
	if(semaPt->Value <= 0)	{
		// Wake up the highest priority thread
		
		uint32_t highest_priority;

    if(b_thread == NULL && _DEBUG)	{
      while(1){
        int a = 1;
      }
      return;
    }

    b_thread->status = ALIVE;               // Set the status of the thread to ALIVE
		highest_priority = b_thread->priority;  // Priority of thread to be woken up
    
    semaPt->blocked = b_thread->next;       // update the blocked linked list of the semaphore, to point to the next thread in the blocked linked list
    semaPt->blocked->prev = NULL;           // list is not circular, so the previous pointer of the first thread in the blocked linked list is NULL


		
		addThreadPriority(b_thread);            // Add the thread back to the priority linked list

		// If the priority of the thread to be woken up is greater than the priority of the current thread, suspend the current thread
		if(currThread->priority > highest_priority)	{
			EndCritical(status);
			OS_Suspend();			// Suspend if waking up a higher priority thread
		}
	}
	
	
	EndCritical(status);
	
	#endif
};

// ******** OS_bWait ************
// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input:  pointer to a binary semaphore
// output: none
void OS_bWait(Sema4Type *semaPt)
{
  // put Lab 2 (and beyond) solution here
  DisableInterrupts();
  while (semaPt->Value == 0)
  {
    EnableInterrupts();
    OS_Suspend(); // run thread switcher
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
void OS_bSignal(Sema4Type *semaPt)
{
  // put Lab 2 (and beyond) solution here
  int critical = StartCritical();
  semaPt->Value = 1;
  EndCritical(critical);
};

void SetInitialStack(int i, void* R9);

//******** OS_AddThread ***************
// add a foregound thread to the scheduler
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields
int OS_AddThread(void (*task)(void),
                 uint32_t stackSize, uint32_t priority)
{
  // put Lab 2 (and beyond) solution here

  PD0 ^= 0x01;

  int status = StartCritical();

  TCBptr newThread = getNextDeadThread(); // get next available TCB

  // if no TCBs are available, return 0
  if (newThread == NULL)
  {
    EndCritical(status);
    return 0; // no TCBs remain available
  }

  // if currThread is NULL, set currThread to newThread
  if (currThread == NULL) currThread = newThread;
  // TODO: Set in OS_Init

  // else  at least one active thread

  newThread->status = ALIVE;
  newThread->ticks_run = 0;

  void* R9;

  if(currThread->pcb != NULL) { // User thread
    newThread->pcb = currThread->pcb;
    currThread->pcb->num_threads++;
    R9 = currThread->pcb->data;
  }
  else { // OS thread
    newThread->pcb = NULL;
    R9 = 0;
  }

  // code gotten from textbook
  SetInitialStack(newThread->id, R9);
  STACK[newThread->id][STACKSIZE - 2] = (int32_t)task; // set the program counter for task stack

  newThread->sp = &STACK[newThread->id][STACKSIZE - 16]; // set the stack pointer for task stack
  newThread->sp_max = newThread->sp;
  newThread->name = NAME;



  // if priority scheduling, then we need to add the thread to the priority linked list
  // otherwise the Round Robin scheduler will handle it from the tcb array.
  #ifdef PRIORITY_SCHEDULING
  newThread->priority = priority;
  addThreadPriority(newThread);
  #endif

  PD0 ^= 0x01;

  EndCritical(status);

  return 1; // replace this line with solution
};

//******** OS_AddThread ***************
// add a foregound thread to the scheduler
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields
int OS_AddThreadWithPCB(void (*task)(void),
                 uint32_t stackSize, uint32_t priority, PCBptr pcb)
{
  // put Lab 2 (and beyond) solution here


  int status = StartCritical();

  PD0 ^= 0x01;


  TCBptr newThread = getNextDeadThread(); // get next available TCB

  // if no TCBs are available, return 0
  if (newThread == NULL)
  {
    EndCritical(status);
    return 0; // no TCBs remain available
  }




  // if currThread is NULL, set currThread to newThread
  if (currThread == NULL) currThread = newThread;
  // TODO: Set in OS_Init

  // else  at least one active thread

  newThread->status = ALIVE;
  newThread->ticks_run = 0;

  void* R9 = pcb->data;

  // code gotten from textbook
  SetInitialStack(newThread->id, R9);
  STACK[newThread->id][STACKSIZE - 2] = (int32_t)task; // set the program counter for task stack

  newThread->sp = &STACK[newThread->id][STACKSIZE - 16]; // set the stack pointer for task stack
  newThread->sp_max = newThread->sp;
  newThread->name = NAME;
  newThread->pcb = pcb;
  newThread->pcb->num_threads++;

  // if priority scheduling, then we need to add the thread to the priority linked list
  // otherwise the Round Robin scheduler will handle it from the tcb array.
  #ifdef PRIORITY_SCHEDULING
  newThread->priority = priority;
  addThreadPriority(newThread);
  #endif

  PD0 ^= 0x01;
  EndCritical(status);

  return 1; // replace this line with solution
};

int OS_AddThread_Name(void (*task)(void),
                      uint32_t stackSize, uint32_t priority, char *name)
{
  // put Lab 2 (and beyond) solution here
  int status = StartCritical();
  NAME = name;
  int a = OS_AddThread(task, stackSize, priority);
  NAME = "NEW_THREAD";

  EndCritical(status);
  return a;
}

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
int OS_AddProcess(void (*entry)(void), void *text, void *data,
                  unsigned long stackSize, unsigned long priority)
{
  // Try to add a thread to the scheduler

  int status = StartCritical();
  PCBptr new_pcb = getNextPCB();
  if(new_pcb == NULL)  return 0;
	
	new_pcb->text = text;
  new_pcb->data = data;

  int result = OS_AddThreadWithPCB(entry, stackSize, priority, new_pcb);
  
  if(result == 0) {
		new_pcb->text = 0;
		new_pcb->data = 0;
    new_pcb->num_threads = 0;
    EndCritical(status);
    return 0;
  }
  

  EndCritical(status);
  return 1; // replace this line with Lab 5 solution
}

//******** OS_Id ***************
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero
uint32_t OS_Id(void)
{
  // put Lab 2 (and beyond) solution here

  return currThread->id; // replace this line with solution
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

int periodicThreadCount = 0;
int OS_AddPeriodicThread(void (*task)(void),
                         uint32_t period, uint32_t priority)
{
  // put Lab 2 (and beyond) solution here
  if(priority > 5){
    priority = 5;
  }
  //
  if(periodicThreadCount == 0){
    Timer4A_Init(task, period, priority); 
  } else if (periodicThreadCount == 1){
    Timer2A_Init(task, period, priority); 
  } else if (periodicThreadCount == 2){
    Timer3A_Init(task, period, priority); 
  } else {
    return 0;
  }
  periodicThreadCount++;


  return 1; // replace this line with solution
}

/*----------------------------------------------------------------------------
  PF1 Interrupt Handler
 *----------------------------------------------------------------------------*/
// void GPIOPortF_Handler(void){
//
// }

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

int OS_AddSW1Task(void (*task)(void), uint32_t priority)
{
  // put Lab 2 (and beyond) solution here

  // add interrupt handler for button press
  if(priority > 5){
    priority = 5;
  }

  EdgeCounterPortF_Init(priority, task);

  return 0; // replace this line with solution
}

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
int OS_AddSW2Task(void (*task)(void), uint32_t priority)
{
  // put Lab 2 (and beyond) solution here
	
	EdgeCounterPortF_Init_SW2(priority, task);

  return 0; // replace this line with solution
}

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(uint32_t sleepTime)
{
  // put Lab 2 (and beyond) solution here
  int status = StartCritical();
  if (sleepTime > 0)
  {
    currThread->status = SLEEPING;
    currThread->sleep = sleepTime;
  }

  EndCritical(status);
  OS_Suspend(); // Check Tejas didn't have this line
};

// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void)
{
  // put Lab 2 (and beyond) solution here

  int CriticalSection = StartCritical(); // start of atomic section CHECK IF THIS IS NEEDED
  PD1 ^= 0x02;       // heartbeat

  // set currThread to DYING state
  // not dead yet, so that getDeadThreads doesnt get it if addThread is called before we can context switch
  currThread->status = DYING;
  PCBptr pcb = currThread->pcb;
  if(pcb != NULL){
    pcb->num_threads--;
    if(pcb->num_threads == 0){
      // call free for text and data
      Heap_Free(pcb->text);
      Heap_Free(pcb->data);
      
    }
  }



  #ifdef PRIORITY_SCHEDULING

  // remove thread from priority queue
  int r = removeThreadPriority(currThread);
  if (r == -1 && _DEBUG)
  {
    // ERROR :: add NEXT??
    currThread->status = FATAL;
    while (1)
    {
    }; // infinite loop
  }
  #endif
  PD1 ^= 0x02;       // heartbeat


  EndCritical(CriticalSection); // end of atomic section
  // HERE was the issue: we finished killing the thread, but before scheduler could be called, we were interrupted by the button press.
  // the button press would then call OS_AddThread, and mess up all the points since the tail had been moved

  // reset SP to the top of the stack :: not necessary since addthread will do this
  // currThread->sp = (int32_t *)&STACK[currThread->id][stackSize-1]
  OS_Suspend();
  return;


  for (;;)
  {
		int a = 0;
		a++;
  }; // can not return
}

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void)
{
  // put Lab 2 (and beyond) solution here
  NVIC_INT_CTRL_R = 0x10000000; // trigger PendSV
  // INTCTRL = 0x04000000; // trigger SysTick
}

// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(uint32_t size)
{
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
int OS_Fifo_Put(uint32_t data)
{
  // put Lab 2 (and beyond) solution here
  // TODO: ADD SEMAPHORES

  if (CurrentSize.Value == FIFOSIZE)
  {
    LostData++;
    return 0;
  }
  else
  {
    Fifo[PutI] = data;
    PutI = (PutI + 1) % FIFOSIZE;
    OS_Signal(&CurrentSize);
    return 1;
  }

  // return 0; // replace this line with solution
};

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data
uint32_t OS_Fifo_Get(void)
{
  // put Lab 2 (and beyond) solution here

  uint32_t data;
  OS_Wait(&CurrentSize);
  data = Fifo[GetI];
  GetI = (GetI + 1) % FIFOSIZE;
  return data;

  // return 0; // replace this line with solution
};

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
int32_t OS_Fifo_Size(void)
{
  // put Lab 2 (and beyond) solution here

  return PutI - GetI; // replace this line with solution
};

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void)
{
  // put Lab 2 (and beyond) solution here

  OS_InitSemaphore(&BoxDataValid, 0);
  OS_InitSemaphore(&BoxFree, 1);

  // put solution here
};

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received
void OS_MailBox_Send(uint32_t data)
{
  // put Lab 2 (and beyond) solution here
  // put solution here
  OS_Wait(&BoxFree);
  Mail = data;
  OS_Signal(&BoxDataValid);
};

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty
uint32_t OS_MailBox_Recv(void)
{
  // put Lab 2 (and beyond) solution here

  uint32_t recvData;

  OS_Wait(&BoxDataValid);

  recvData = Mail;

  OS_Signal(&BoxFree);

  return recvData;
};

// ******** OS_Time ************
// return the system time
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as
//   this function and OS_TimeDifference have the same resolution and precision
uint32_t OS_Time(void)
{
  // put Lab 2 (and beyond) solution here

  return NVIC_ST_CURRENT_R; // replace this line with solution
};

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as
//   this function and OS_Time have the same resolution and precision
uint32_t OS_TimeDifference(uint32_t start, uint32_t stop)
{
  // put Lab 2 (and beyond) solution here

  if (stop > start)
  {
    uint32_t a = time_slice - stop;
    return start + a; // replace this line with solution
  }
  else
  {
    return start - stop;
  }
};

void update_time(void)
{
  time += 1;
}

// ******** OS_ClearMsTime ************
// sets the system time to zero (solve for Lab 1), and start a periodic interrupt
// Inputs:  none
// Outputs: none
// You are free to change how this works
void OS_ClearMsTime(void)
{
  // put Lab 1 solution here
  time = 0;
  Timer5A_Init(&update_time, TIME_1MS, 0);
}

// ******** OS_MsTime ************
// reads the current time in msec (solve for Lab 1)
// Inputs:  none
// Outputs: time in ms units
// You are free to select the time resolution for this function
// For Labs 2 and beyond, it is ok to make the resolution to match the first call to OS_AddPeriodicThread
uint32_t OS_MsTime(void)
{
  // put Lab 1 solution here
  return time;
}

//******** OS_Launch ***************
// start the scheduler, enable interrupts
// Inputs: number of 12.5ns clock cycles for each time slice
//         you may select the units of this parameter
// Outputs: none (does not return)
// In Lab 2, you can ignore the theTimeSlice field
// In Lab 3, you should implement the user-defined TimeSlice field
// It is ok to limit the range of theTimeSlice to match the 24-bit SysTick
void OS_Launch(uint32_t theTimeSlice)
{
  // put Lab 2 (and beyond) solution here

  time_slice = theTimeSlice;
  NVIC_ST_RELOAD_R = theTimeSlice - 1; // reload value
  SysTick_Enable();
  // Check to make sure a thread is available TODO:
  StartOS();
}

//************** I/O Redirection ***************
// redirect terminal I/O to UART or file (Lab 4)

int StreamToDevice = 0; // 0=UART, 1=stream to file (Lab 4)

int fputc(int ch, FILE *f)
{
  if (StreamToDevice == 1)
  { // Lab 4
    if (eFile_Write(ch))
    {                         // close file on error
      OS_EndRedirectToFile(); // cannot write to file
      return 1;               // failure
    }
    return 0; // success writing
  }

  // default UART output
  UART_OutChar(ch);
  return ch;
}

int fgetc(FILE *f)
{
  char ch = UART_InChar(); // receive from keyboard
  UART_OutChar(ch);        // echo
  return ch;
}

int OS_RedirectToFile(const char *name)
{                     // Lab 4
  eFile_Create(name); // ignore error if file already exists
  if (eFile_WOpen(name))
    return 1; // cannot open file
  StreamToDevice = 1;
  return 0;
}

int OS_EndRedirectToFile(void)
{ // Lab 4
  StreamToDevice = 0;
  if (eFile_WClose())
    return 1; // cannot close file
  return 0;
}

int OS_RedirectToUART(void)
{
  StreamToDevice = 0;
  return 0;
}

int OS_RedirectToST7735(void)
{
  return 1;
}

void SetInitialStack(int i, void* R9)
{
  STACK[i][STACKSIZE - 1] = 0x01000000; // thumb bit
  // STACK[i][STACKSIZE-2] is PC
  STACK[i][STACKSIZE - 3] = 0x14141414;  // R14
  STACK[i][STACKSIZE - 4] = 0x12121212;  // R12
  STACK[i][STACKSIZE - 5] = 0x03030303;  // R3
  STACK[i][STACKSIZE - 6] = 0x02020202;  // R2
  STACK[i][STACKSIZE - 7] = 0x01010101;  // R1
  STACK[i][STACKSIZE - 8] = 0x00000000;  // R0
  STACK[i][STACKSIZE - 9] = 0x11111111;  // R11
  STACK[i][STACKSIZE - 10] = 0x10101010; // R10
  STACK[i][STACKSIZE - 11] = (int32_t)R9; // R9
  STACK[i][STACKSIZE - 12] = 0x08080808; // R8
  STACK[i][STACKSIZE - 13] = 0x07070707; // R7
  STACK[i][STACKSIZE - 14] = 0x06060606; // R6
  STACK[i][STACKSIZE - 15] = 0x05050505; // R5
  STACK[i][STACKSIZE - 16] = 0x04040404; // R4
}

// xPSR, PC, LR, R12, R0-R3


void Scheduler(void)
{

  int status = StartCritical();
  PD2 ^= 0x04;

  // Update the current thread's ticks_run
  currThread->ticks_run++;

  if (currThread->status == DYING)
  {
    currThread->status = DEAD;
  }

  #ifdef PRIORITY_SCHEDULING 
  currThread = getNextPriorityThreadAndUpdateList(); // priority scheduler :: gets the next thread with the highest priority
  #else
    currThread = getNextAliveThread(); // round robin
  #endif

  PD2 ^= 0x04;
  
  EndCritical(status);
}
// Adds a thread to the priority array.
// Assumes that the thread is not already in the array and that the array is not full.

void addThreadPriority(TCBptr thread)
{
  int priority = thread->priority;

  thread->next = NULL;
  thread->prev = NULL;

  //get the head of the list for the priority level
  TCBptr ptr = TCB_priority_array[priority]; 
  if (ptr == NULL) // if the array is empty, update the head
  {
    TCB_priority_array[priority] = thread;
    thread->next = thread;
    thread->prev = thread;
    return;
  }
  else // if the array is not empty, add to the end
  {
    do{
      ptr = ptr->next;
    }while(ptr->next != TCB_priority_array[priority]);

    // add to the end
    ptr->next = thread;
    thread->prev = ptr;


    // make the list circular
    thread->next = TCB_priority_array[priority];
    TCB_priority_array[priority]->prev = thread;
  }
}


// Removes a thread from the priority array.
// Assumes that the thread is in the array and that the array is not empty.
int removeThreadPriority(TCBptr thread)
{
  int priority = thread->priority;

  // if there is only one thread in the array
  if (thread->next == thread)
  {
    TCB_priority_array[priority] = NULL;
    return 0;
  } else{
    //  if the thread is the first thread in the array
    if (TCB_priority_array[priority] == thread)
    {
      TCB_priority_array[priority] = thread->next;
      // TCB_priority_array[priority]->prev = thread->prev;
    }

    thread->prev->next = thread->next;
    thread->next->prev = thread->prev;
    

    thread->next = NULL;
    thread->prev = NULL;
    return 0;

  }


  // if there are multiple threads in the array
  return -1;

}

// Must be called in Critical Section
// dying threads have already been removed from the priority array
//
// moves the thread to the end of the list

TCBptr getNextPriorityThreadAndUpdateList()
{
  int i;
  for (i = 0; i < MAX_PRIORITY_PLUS_ONE; i++)
  {
    if (TCB_priority_array[i] != NULL) // if there is a thread in the list at i priority
    {
      // get first thread not sleeping
      TCBptr thread = TCB_priority_array[i];

      // if all threads are sleeping go to the next priority
      do{
        if (thread->status == ALIVE)
        {
          break;
        }
        thread = thread->next;
      }while(thread != TCB_priority_array[i]);

      // if all threads are sleeping go to the next priority
      if (thread->status != ALIVE)
      {
        continue;
      }
      
      // only one thread in the list -> dont move
      if(thread->next == thread){
        return thread;
      }

      // next one to be considered
      TCB_priority_array[i] = thread->next;

      return thread;
    }
  }
  return NULL;
}

// loop through TCB_array and count number of threads that arent DEAD
int OS_ThreadCount(){
  int i;
  int count = 0;
  for (i = 0; i < NUMTHREADS; i++)
  {
    if (TCB_array[i].status != DEAD)
    {
      count++;
    }
  }
  return count;
}