#include "utils.h"
#include "types.h"
#include "OS.h"
#include "mm.h"
#include "sched.h"
#include "sys.h"

void * sys_call_table[] = {sys_write, sys_fork, sys_exit};

void sys_write(char * buf)
{
    // main_output(MU, buf);
    int a = 0;
}

int sys_fork(u64 stack)
{
    // return copy_process(UTHREAD, 0, 0);
    // return OS_AddThread
    int a = 1;
}

void sys_exit()
{
    OS_Kill();
}

void sys_call_table_relocate()
{
    for (int i = 0; i < NR_SYSCALLS; i++) {
        sys_call_table[i] = (void *)((u64)sys_call_table[i] + (u64)KERNEL_START);
    }
}

