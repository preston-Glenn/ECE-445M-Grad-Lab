#ifndef	_UTILS_H
#define	_UTILS_H

#include "types.h"

extern void delay ( unsigned long);
extern void put32 ( unsigned long, unsigned int );
extern unsigned int get32 ( unsigned long );
extern int get_el ( void );
extern u32 get_core();


#endif  /*_UTILS_H */