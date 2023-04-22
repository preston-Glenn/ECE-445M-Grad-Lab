#include "utils.h"
#include "printf.h"
#include "peripherals/timer.h"
#include "OS.h"

unsigned int interval = 200000;
unsigned int curVal = 0;

void timer_init ( unsigned int ts )
{
    if(ts > 0) interval = ts;
	curVal = get32(TIMER_CLO);
	curVal += interval;
	put32(TIMER_C1, curVal);
}

void handle_timer_irq( void ) 
{
	curVal += interval;
	put32(TIMER_C1, curVal);
	put32(TIMER_CS, TIMER_CS_M1);
	// printf("Timer interrupt received\n\r");
    OS_SysTick_Handler();
}