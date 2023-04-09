// Timer4A.c
// Runs on LM4F120/TM4C123
// Use Timer4 in 32-bit periodic mode to request interrupts at a periodic rate
// Daniel Valvano
// Jan 1, 2020

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2020
  Program 7.5, example 7.6

 Copyright 2020 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../RTOS_Labs_common/OS.h"


void (*PeriodicTask4)(void);   // user function

// ***************** Timer4A_Init ****************
// Activate Timer4 interrupts to run user task periodically
// Inputs:  task is a pointer to a user function
//          period in units (1/clockfreq)
//          priority 0 (highest) to 7 (lowest)
// Outputs: none
void Timer4A_Init(void(*task)(void), uint32_t period, uint32_t priority){
  SYSCTL_RCGCTIMER_R |= 0x10;   // 0) activate TIMER4
  PeriodicTask4 = task;         // user function
  TIMER4_CTL_R = 0x00000000;    // 1) disable TIMER4A during setup
  TIMER4_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER4_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER4_TAILR_R = period-1;    // 4) reload value
  TIMER4_TAPR_R = 0;            // 5) bus clock resolution
  TIMER4_ICR_R = 0x00000001;    // 6) clear TIMER3A timeout flag
  TIMER4_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI17_R = (NVIC_PRI17_R&0xFF00FFFF)|(priority<<21); // priority
// interrupts enabled in the main program after all devices initialized
// vector number 86, interrupt number 70
  NVIC_EN2_R = 0x00000040;      // 9) enable interrupt 70 in NVIC
  TIMER4_CTL_R = 0x00000001;    // 10) enable TIMER4A
}


uint8_t timer4A_work = 0;
extern int32_t MaxJitter;             // largest time jitter between interrupts in usec
extern uint32_t const JitterSize;
extern uint32_t JitterHistogram[];


void Timer4A_Handler(void){
  TIMER4_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER4A timeout
  long jitter;                    // time between measured and expected, in us
  unsigned static long LastTime;  // time at previous ADC sample

  if(timer4A_work==0) {
    timer4A_work++;
    LastTime = OS_Time();       // current time, 12.5 ns
    (*PeriodicTask4)();               // execute user task
    return;
  }

  uint32_t thisTime;              // time at current ADC sample

  thisTime = OS_Time();       // current time, 12.5 ns
  (*PeriodicTask4)();               // execute user task
  uint32_t diff = OS_TimeDifference(LastTime,thisTime);
  if(diff>(TIMER4_TAILR_R + 1 ) ){
    jitter = (diff-(TIMER4_TAILR_R + 1 ) +4)/8;  // in 0.1 usec
  }else{
    if(TIMER4_TAILR_R == NVIC_ST_RELOAD_R){
      jitter = (diff + 4 + TIMER4_TAILR_R + 1) / 8;
    } else{
      jitter = ((TIMER4_TAILR_R + 1 ) -diff+4)/8;  // in 0.1 usec
    }
  }
  if(jitter > MaxJitter){
    MaxJitter = jitter; // in usec
  }       // jitter should be 0
  if(jitter >= JitterSize){
    jitter = JitterSize-1;
  }
  JitterHistogram[jitter]++; 
  LastTime = thisTime;

}

void Timer4A_Stop(void){
  NVIC_DIS2_R = 0x00000040;        // 9) disable interrupt 70 in NVIC
  TIMER4_CTL_R = 0x00000000;       // 10) disable timer4A
}
