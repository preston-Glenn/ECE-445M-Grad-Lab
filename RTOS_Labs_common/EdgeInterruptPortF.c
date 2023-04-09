// EdgeInterruptPortF.c
// Runs on LM4F120 or TM4C123
// Request an interrupt on the falling edge of PF4 (when the user
// button is pressed) and increment a counter in the interrupt.  Note
// that button bouncing is not addressed.
// Daniel Valvano
// August 30, 2019

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers"
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2019
   Volume 1, Program 9.4
   
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2019
   Volume 2, Program 5.6, Section 5.5

 Copyright 2019 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

// user button connected to PF4 (increment counter on falling edge)


#include "EdgeInterruptPortF.h"
#include "../inc/LaunchPad.h"

void (*SWTaskONE)(void);
void (*SWTaskTWO) (void);
volatile uint32_t FallingEdges = 0;


// global variable visible in Watch window of debugger
// increments at least once per button press
void EdgeCounterPortF_Init(uint32_t priority, void(*task)(void)){                          
  SYSCTL_RCGCGPIO_R |= 0x00000020; // (a) activate clock for port F
  FallingEdges = 0;             // (b) initialize counter
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0
  GPIO_PORTF_DIR_R |=  0x0E;    // output on PF3,2,1 
  GPIO_PORTF_DIR_R &= ~0x11;    // (c) make PF4,0 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x1F;  //     disable alt funct on PF4,0
  GPIO_PORTF_DEN_R |= 0x1F;     //     enable digital I/O on PF4   
  GPIO_PORTF_PCTL_R &= ~0x000FFFFF; // configure PF4 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       //     disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x11;     //     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x10;     // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x10;    //     PF4 is not both edges
  GPIO_PORTF_IEV_R &= ~0x10;    //     PF4 falling edge event
  GPIO_PORTF_ICR_R = 0x10;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x10;      // (f) arm interrupt on PF4 *** No IME bit as mentioned in Book ***
  
	uint32_t a = (((priority*2) & 0xFF)<<20);
	NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)| a; // (g) priority 5
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
  SWTaskONE = task;
}

void EdgeCounterPortF_Init_SW2(uint32_t priority, void(*task)(void)){                          
  SYSCTL_RCGCGPIO_R |= 0x00000020; // (a) activate clock for port F
  FallingEdges = 0;             // (b) initialize counter
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0
  GPIO_PORTF_DIR_R |=  0x0E;    // output on PF3,2,1 
  GPIO_PORTF_DIR_R &= ~0x11;    // (c) make PF4,0 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x1F;  //     disable alt funct on PF4,0
  GPIO_PORTF_DEN_R |= 0x1F;     //     enable digital I/O on PF4   
  GPIO_PORTF_PCTL_R &= ~0x000FFFFF; // configure PF4 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       //     disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x11;     //     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x01;     // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x01;    //     PF4 is not both edges
  GPIO_PORTF_IEV_R &= ~0x01;    //     PF4 falling edge event
  GPIO_PORTF_ICR_R = 0x01;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x01;      // (f) arm interrupt on PF4 *** No IME bit as mentioned in Book ***
  
	uint32_t a = (((priority*2) & 0xFF)<<20);
	NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)| a; // (g) priority 5
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
  SWTaskTWO = task;
}

void GPIOPortF_Handler(void){
	
	if(GPIO_PORTF_MIS_R & 0x10)	{
	GPIO_PORTF_IM_R &= ~0x10; // disarm interrupt
  FallingEdges = FallingEdges + 1;
  PF1 ^= 0x02; // profile
	LaunchPad_Output(1);
  SWTaskONE();
	GPIO_PORTF_ICR_R |= 0x10;      // acknowledge flag4
         GPIO_PORTF_IM_R |= 0x10; // arm interrupt again 
	}
	else if(GPIO_PORTF_MIS_R & 0x01)	{
		GPIO_PORTF_IM_R &= ~0x01; // disarm interrupt
  FallingEdges = FallingEdges + 1;
  PF1 ^= 0x02; // profile
	LaunchPad_Output(2);
  SWTaskTWO();
	GPIO_PORTF_ICR_R |= 0x01;      // acknowledge flag4
         GPIO_PORTF_IM_R |= 0x01; // arm interrupt again 
	}
	
}
