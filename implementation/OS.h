#ifndef _OS_H
#define _OS_H

#define THREAD_CPU_CONTEXT			0 		// offset of cpu_context in task_struct 

#ifndef __ASSEMBLER__

#define THREAD_SIZE				4096

#define NR_TASKS				64 

#define FIRST_TASK task[0]
#define LAST_TASK task[NR_TASKS-1]

#define TASK_RUNNING				0

#define NUM_PROCESSES 8
#define MAX_PAGE_COUNT    16


extern struct task_struct *current;
extern struct task_struct * task[NR_TASKS];
extern int nr_tasks;

struct cpu_context {
	unsigned long x19;
	unsigned long x20;
	unsigned long x21;
	unsigned long x22;
	unsigned long x23;
	unsigned long x24;
	unsigned long x25;
	unsigned long x26;
	unsigned long x27;
	unsigned long x28;
	unsigned long fp;
	unsigned long sp;
	unsigned long pc; // lr 
};

struct PCB {
 int ID;
 int priority;
 int num_threads;
 void* text;
 void* data;
   
};
typedef struct PCB PCB;
typedef struct PCB* PCBptr;


// struct task_struct {
// 	struct cpu_context cpu_context;
// 	long state;	
// 	long counter;
// 	long priority;
// 	long preempt_count;
// };

typedef struct tcb TCB;
typedef TCB *TCBptr;


extern PCB PCB_array[NUM_PROCESSES];

struct user_page
{
    u64 pa;
    u64 uva;
};

struct mm_struct
{
    u64 pgd;
    struct user_page user_pages[MAX_PAGE_COUNT];
    u64 kernel_pages[MAX_PAGE_COUNT];
};



struct tcb
{
  struct cpu_context cpu_context;  
  TCBptr next; // linked-list pointer
  TCBptr prev; // linked-list pointer

  int id;            // thread id
  unsigned long *blocked;  // nonzero if blocked on this semaphore
  unsigned long sleep;    // nonzero if this thread is sleeping
  unsigned long priority; // 0 is highest
  // stack
  // max stack size is 500 32-bit words
  unsigned long *sp_max;   // highest stack address
  unsigned long ticks_run; // number of ticks thread has run
  char *name;        // name of thread
  unsigned long status;
  PCBptr pcb;
  u64 flags;
  u64 heap;
  u64 heap_size = PAGE_SIZE;
  u64 stack;
  struct mm_struct mm;
};

#define INIT_TASK \
/* core_context */	{ {0,0,0,0,0,0,0,0,0,0,0,0,0}, \
/* next */		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
/* PCBprt*/   0, 0\  

/* mm */ {0, {{0}}, {0} } \
}




// add status enum
enum status
{
  ALIVE,
  DYING,
  DEAD,
  BLOCKED,
  SLEEPING,
  FATAL
};



extern void sched_init(void);
extern void schedule(void);
extern void timer_tick(void);
extern void preempt_disable(void);
extern void preempt_enable(void);
extern void switch_to(TCBptr next);
extern void cpu_switch_to(TCBptr prev, TCBptr next);
void set_PGD_CURR();

void OS_SysTick_Handler(void);


struct  Sema4{
  int Value;   // >0 means free, otherwise means busy      
	TCBptr blocked;
  TCBptr running;
// add other components here, if necessary to implement blocking
};
typedef struct Sema4 Sema4Type;

extern TCBptr currThread;



#endif
#endif