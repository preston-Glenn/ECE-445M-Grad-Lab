


// PCBs

// round robin, no sleep, no priority, no semaphores

// OS_Init


// Start_OS


// Add_Threads
#include "mm.h"
#include "OS.h"
#include "fb.h"
#include "io.h"
#include "irq_numbers.h"
#include "irq.h"
#include "generic_timer.h"

PCB PCB_array[NUM_PROCESSES];

unsigned int Mail; // shared data
Sema4Type BoxFree;
Sema4Type BoxDataValid;

#define NULL 0

#define STACKSIZE 400 // number of 32-bit words in stack
#define NUMTHREADS 10 // maximum number of threads

TCB TCB_array[NUMTHREADS];
TCBptr currThread;


int PREEMPTIVE = 0;

void prep_dead_threads()
{
  int i = 0;

  for (i = 0; i < NUMTHREADS; i++)
  {
    TCB_array[i].status = DEAD;
    TCB_array[i].id = i;
  }
}


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

void switch_to( TCBptr next) 
{
	if (currThread == next) 
		return;
	TCBptr prev = currThread;
	currThread = next;
  set_pgd(next->mm.pgd)
	cpu_switch_to(prev, next);
}

void set_PGD_CURR(){
  set_pgd(currThread->mm.pgd);
}

int OS_AddThread(unsigned long task, unsigned long priority)
{
    int status = StartCritical();
    TCBptr newThread = getNextDeadThread(); // get next available TCB


    if (newThread == NULL)
    {
    EndCritical(status);
    return 0; // no TCBs remain available
    }
	
    if (currThread == NULL) currThread = newThread;


	unsigned long stack =  get_free_page(); // allocate stack allocate heap
	if (!stack)
		return 1;

    newThread->status = ALIVE;
    newThread->ticks_run = 0;

    // TODO: user vs os thread R9 (offset)

    newThread->priority = priority;


	newThread->cpu_context.pc = task;
	newThread->cpu_context.sp = stack;
  newThread->mm.pgd = 0;
  newThread->heap = allocate_user_page(newThread, 0);
  newThread->heap_size = PAGE_SIZE;
  newThread->stack = allocate_user_page(newThread, PAGE_SIZE * 2);

  EndCritical(status);


	//preempt_enable();
	return 0;
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

char *NAME = "NEW_THREAD";

int OS_AddThreadwithPCB(void (*task)(void), int stackSize, int priority, PCBptr pcb) {

  int status = StartCritical();

  TCBptr newThread = getNextDeadThread();

  // if no TCBs are available, return 0
  if (newThread == NULL)
  {
    EndCritical(status);
    return 0; // no TCBs remain available
  }

  // if currThread is NULL, set currThread to newThread
  if (currThread == NULL) currThread = newThread;

  unsigned long stack =  get_free_page(); // allocate stack allocate heap
	if (!stack)
		return 1;


  newThread->status = ALIVE;
  newThread->ticks_run = 0;

  newThread->cpu_context.pc = task;
	newThread->cpu_context.sp = stack;
  
  
  newThread->name = NAME;
  newThread->pcb = pcb;
  newThread->pcb->num_threads++;

   EndCritical(status);

  return 1; // replace this line with solution

  EndCritical(status);

}

int OS_AddProcess(void (*entry)(void), void *text, void *data, unsigned long stackSize, unsigned long priority) {
  int status = StartCritical();

  PCBptr new_pcb = getNextPCB();

  if(new_pcb == NULL) return 0;

  new_pcb->text = text;
  new_pcb->data = data;

  int result = OS_AddThreadwithPCB(entry, stackSize, priority, new_pcb);

  if(result == 0) {
    new_pcb->text = 0;
		new_pcb->data = 0;
    new_pcb->num_threads = 0;
    EndCritical(status);
    return 0;
  }

  EndCritical(status);
  return 1;
}

// Kill_Thread

void OS_Kill(void)
{
    // put Lab 2 (and beyond) solution here

    int CriticalSection = StartCritical(); // start of atomic section CHECK IF THIS IS NEEDED
    //PD1 ^= 0x02;       // heartbeat

    // set currThread to DYING state
    // not dead yet, so that getDeadThreads doesnt get it if addThread is called before we can context switch
    currThread->status = DYING;
    //   PCBptr pcb = currThread->pcb;
    //   if(pcb != NULL){
    //     pcb->num_threads--;
    //     if(pcb->num_threads == 0){
    //       // call free for text and data
    //       Heap_Free(pcb->text);
    //       Heap_Free(pcb->data);

    //     }
    //   }

    // If cooperative then we can just kill the thread and context switch to the next thread
    currThread->status = DEAD;
    OS_Scheduler();

}


// Scheduler

int OS_Scheduler()
{
    int status = StartCritical();
    TCBptr nextThread = getNextAliveThread(); // round robin

    if (nextThread != currThread)
    {
        switch_to(nextThread);
    }
    EndCritical(status);
    return;
}




// SysTick

// suspend
void OS_Suspend(){
    OS_Scheduler();
}

// launch

void OS_Launch(unsigned int theTimeSlice)
{
  // put Lab 2 (and beyond) solution here

  timer_init(theTimeSlice);
  //NVIC_ST_RELOAD_R = theTimeSlice - 1; // reload value
 // SysTick_Enable();
  // Check to make sure a thread is available TODO:
  StartOS();
}



void OS_Init(){
    DisableInterrupts();
    prep_dead_threads();
    OS_MailBox_Init();
    uart_init();
    fb_init();

    irq_init_vectors();
    generic_timer_init();
    enable_interrupt_gic(NS_PHYS_TIMER_IRQ, id);

    // other initializations
    // TODO: make sure that we enable interrupts at StartOS
}

// time

// id 

// Semaphores 

void OS_InitSemaphore(Sema4Type *semaPt, int value)
{
    // put Lab 2 (and beyond) solution here
    semaPt->Value = value;
    semaPt->blocked = NULL;
};

void OS_Signal(Sema4Type *semaPt)
{
  long critical = StartCritical();
  semaPt->Value = semaPt->Value + 1;
  EndCritical(critical);

};


void OS_Wait(Sema4Type *semaPt){

  DisableInterrupts();
  while (semaPt->Value <= 0)
  {
    EnableInterrupts();
    OS_Suspend(); // run thread switcher
    DisableInterrupts();
  }
  semaPt->Value = semaPt->Value - 1;
  EnableInterrupts();
	
};

void OS_MailBox_Init(void)
{
  // put Lab 2 (and beyond) solution here

  OS_InitSemaphore(&BoxDataValid, 0);
  OS_InitSemaphore(&BoxFree, 1);

  // put solution here
};


void OS_MailBox_Send(unsigned int data)
{
  // put Lab 2 (and beyond) solution here
  // put solution here
  OS_Wait(&BoxFree);
  Mail = data;
  OS_Signal(&BoxDataValid);
};

unsigned int OS_MailBox_Recv(void)
{
  // put Lab 2 (and beyond) solution here

  unsigned int recvData;

  OS_Wait(&BoxDataValid);

  recvData = Mail;

  OS_Signal(&BoxFree);

  return recvData;
};

OS_Sleep(unsigned long sleepTime)
{
  // put Lab 2 (and beyond) solution here
  // put solution here
  int status = StartCritical();
  if(sleepTime == 0) return;
  currThread->sleep = sleepTime;
  currThread->status = SLEEPING;
  EndCritical(status);
  OS_Suspend();

};


void OS_SysTick_Handler(void)
{

  int status = StartCritical();
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

  if(PREEMPTIVE)
    OS_Scheduler();

  EndCritical(status);


}

// If only cooperative where its always a function call then we dont need to save R0 - R18 since they are volatile
// but if we want to support preemption then we need to save them. This can be done easily with an interrupt handler
// that saves all the registers. Then all we need to do is save the stack pointer and maybe the og LR to the previous thread