#include "mm.h"
#include "types.h"
// #include "sched.h"
#include "utils.h"
#include "mmu.h"
static unsigned short mem_map [ PAGING_PAGES ] = {0,};

/* returns a pa of a free page, does not return if it fails */
u64 get_free_page()
{
    for (int i = 0; i < MALLOC_PAGES; i++) {
        if (mem_map[i] == 0) {
            mem_map[i] = 1;
            u64 ret = MALLOC_START + i * PAGE_SIZE;
            memzero(PA_TO_KVA(ret), PAGE_SIZE);
            return ret;
        }
    }
    // panic("Out of physical memory!");
    /* doesn't comeback */
    return 0;
}

void free_page(unsigned long p){
	mem_map[(p - LOW_MEMORY) / PAGE_SIZE] = 0;
}



// NEW









/* utility function to get used count of user pages array */
int task_up_count(struct task_struct *task)
{
    for (int i = 0; i < MAX_PAGE_COUNT; i++) {
        if (task->mm.user_pages[i].pa == 0)
            return i;
    }
    return MAX_PAGE_COUNT;
}

/* returns a kva of a free page */
u64 get_kernel_page()
{
    u64 phys_page = get_free_page();
    return PA_TO_KVA(phys_page);
}

void free_kernel_page(u64 kp)
{
    free_page(KVA_TO_PA(kp));
}


int do_data_abort(u64 far, u64 esr)
{
    u64 dfsc = esr & 0x3f;
    /* 0x7: level 3 translation fault */
    if (dfsc == 0x7) {
        u64 page = get_free_page();
        if (map_page(current, far & PAGE_MASK, page) < 0)
            return -1;
        return 0;
    }
    return -1;
}