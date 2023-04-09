// FIFO.h
// Runs on any LM3Sxxx
// Provide functions that initialize a FIFO, put data in, get data out,
// and return the current size.  The file includes a transmit FIFO
// using index implementation and a receive FIFO using pointer
// implementation.  Other index or pointer implementation FIFOs can be
// created using the macros supplied at the end of the file.
// Daniel Valvano
// June 16, 2011

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
      Programs 3.7, 3.8., 3.9 and 3.10 in Section 3.7

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

#include "OS.h"
#include "RxTxFifo.h"

long StartCritical(void);  // previous I bit, disable interrupts
void EndCritical(long sr); // restore I bit to previous value

Sema4Type RxDataAvailable;
Sema4Type TxRoom;


char volatile *RxPutPt;
char volatile *RxGetPt;
char static RxFifo[RxTxFIFOSIZE];
void RxFifo_Init(void)
{
  long sr;
  sr = StartCritical();
  RxPutPt = RxGetPt = &RxFifo[0];
  EndCritical(sr);
}
int RxFifo_Put(char data)
{
  char volatile *nextPutPt;
  nextPutPt = RxPutPt + 1;
  if (nextPutPt == &RxFifo[RxTxFIFOSIZE])
  {
    nextPutPt = &RxFifo[0];
  }
  if (nextPutPt == RxGetPt)
  {
    return (RxTxFIFOFAIL);
  }
  else
  {
    *(RxPutPt) = data;
    RxPutPt = nextPutPt;
    OS_Signal(&RxDataAvailable);
    return (RxTxFIFOSUCCESS);
  }
}
int RxFifo_Get(char *datapt)
{
  OS_Wait(&RxDataAvailable);
  if (RxPutPt == RxGetPt)
  {
    return (RxTxFIFOFAIL);
  }
  *datapt = *(RxGetPt++);
  if (RxGetPt == &RxFifo[RxTxFIFOSIZE])
  {
    RxGetPt = &RxFifo[0];
  }
  return (RxTxFIFOSUCCESS);
}
unsigned short RxFifo_Size(void)
{
  if (RxPutPt < RxGetPt)
  {
    return ((unsigned short)(RxPutPt - RxGetPt + (RxTxFIFOSIZE * sizeof(char))) / sizeof(char));
  }
  return ((unsigned short)(RxPutPt - RxGetPt) / sizeof(char));
}



char volatile *TxPutPt;
char volatile *TxGetPt;
char static TxFifo[RxTxFIFOSIZE];
void TxFifo_Init(void)
{
  long sr;
  sr = StartCritical();
  TxPutPt = TxGetPt = &TxFifo[0];
  OS_InitSemaphore(&TxRoom, RxTxFIFOSIZE);
  EndCritical(sr);
}
int TxFifo_Put(char data)
{
  OS_Wait(&TxRoom);
  char volatile *nextPutPt;
  nextPutPt = TxPutPt + 1;
  if (nextPutPt == &TxFifo[RxTxFIFOSIZE])
  {
    nextPutPt = &TxFifo[0];
  }
  if (nextPutPt == TxGetPt)
  {
    return (RxTxFIFOFAIL);
  }
  else
  {
    *(TxPutPt) = data;
    TxPutPt = nextPutPt;
    return (RxTxFIFOSUCCESS);
  }
}
int TxFifo_Get(char *datapt)
{
  if (TxPutPt == TxGetPt)
  {
    return (RxTxFIFOFAIL);
  }
  *datapt = *(TxGetPt++);
  if (TxGetPt == &TxFifo[RxTxFIFOSIZE])
  {
    TxGetPt = &TxFifo[0];
  }
  OS_Signal(&TxRoom);
  return (RxTxFIFOSUCCESS);
}
unsigned short TxFifo_Size(void)
{
  if (TxPutPt < TxGetPt)
  {
    return ((unsigned short)(TxPutPt - TxGetPt + (RxTxFIFOSIZE * sizeof(char))) / sizeof(char));
  }
  return ((unsigned short)(TxPutPt - TxGetPt) / sizeof(char));
}
// e.g.,
// AddPointerFifo(Tx,32,unsigned char, 1,0)
// RxTxFIFOSIZE can be any size
// creates TxFifo_Init() TxFifo_Get() and TxFifo_Put()
