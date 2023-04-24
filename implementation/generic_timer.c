#include "generic_timer.h"

void generic_timer_init()
{
    setup_CNTP_CTL();
    set_CNTP_TVAL(SYS_FREQ);
}

void handle_generic_timer()
{
    set_CNTP_TVAL(SYS_FREQ);
}

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
