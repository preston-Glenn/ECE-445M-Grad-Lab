
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

// use for debugging profile
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))

//volatile uint32_t FallingEdges = 0;
void EdgeCounterPortF_Init(uint32_t priority, void(*task)(void));
void EdgeCounterPortF_Init_SW2(uint32_t priority, void(*task)(void));

extern void (*SWTaskONE)(void);
extern void (*SWTaskTWO)(void);
extern volatile uint32_t FallingEdges;