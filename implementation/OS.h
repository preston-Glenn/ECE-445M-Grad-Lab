#ifndef _OS_H
#define _OS_H

#define THREAD_CPU_CONTEXT			0 		// offset of cpu_context in task_struct 

#ifndef __ASSEMBLER__

#define THREAD_SIZE				4096

#define NR_TASKS				64 

#define FIRST_TASK task[0]
#define LAST_TASK task[NR_TASKS-1]

#define TASK_RUNNING				0

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

// struct task_struct {
// 	struct cpu_context cpu_context;
// 	long state;	
// 	long counter;
// 	long priority;
// 	long preempt_count;
// };


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
//   PCBptr pcb;
};

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

typedef struct tcb TCB;
typedef TCB *TCBptr;

extern void sched_init(void);
extern void schedule(void);
extern void timer_tick(void);
extern void preempt_disable(void);
extern void preempt_enable(void);
extern void switch_to(TCBptr next);
extern void cpu_switch_to(TCBptr prev, TCBptr next);

#define INIT_TASK \
/*cpu_context*/	{ {0,0,0,0,0,0,0,0,0,0,0,0,0}, \
/* state etc */	0,0,1, 0 \
}

#endif
#endif