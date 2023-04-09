
#include <stdint.h>
#include "../RTOS_Labs_common/fifo.h"
#include "../RTOS_Labs_common/OS.h"

uint32_t volatile RxPutI;
uint32_t volatile RxGetI;
uint32_t volatile TxPutI;
uint32_t volatile TxGetI;

char static RxFifo [FIFOSIZE];
char static TxFifo [FIFOSIZE];

Sema4Type RxDataAvailable;
Sema4Type TxRoomLeft;




void RxFifo_Init(void){           
  RxPutI = RxGetI = 0;   
  OS_InitSemaphore(&RxDataAvailable, 0);   
}                                       

int RxFifo_Put (char data){       
  if(( RxPutI - RxGetI ) & ~(FIFOSIZE-1)){  
    return(FIFOFAIL);      
  }                    
  RxFifo[ RxPutI &(FIFOSIZE-1)] = data; 
  RxPutI++;  
  OS_Signal(&RxDataAvailable);
  return(FIFOSUCCESS);     
}                      

int RxFifo_Get (char *datapt){  
  OS_Wait(&RxDataAvailable);
  if( RxPutI == RxGetI ){ 
    return(FIFOFAIL);      
  }                    
  *datapt = RxFifo[ RxGetI &(FIFOSIZE-1)];  
  RxGetI++; 
  return(FIFOSUCCESS);     
}                      
unsigned short RxFifo_Size (void){  
 return ((unsigned short)( RxPutI - RxGetI ));  
}


void TxFifo_Init(void){           
  TxPutI = TxGetI = 0;   
  OS_InitSemaphore(&TxRoomLeft, FIFOSIZE);   
}                                       

int TxFifo_Put (char data){   
  OS_Wait(&TxRoomLeft);    
  if(( TxPutI - TxGetI ) & ~(FIFOSIZE-1)){  
    return(FIFOFAIL);      
  }                    
  TxFifo[ TxPutI &(FIFOSIZE-1)] = data; 
  TxPutI++;  
  return(FIFOSUCCESS);     
}                      

int TxFifo_Get (char *pt){  
  if( TxPutI == TxGetI ){ 
    return(FIFOFAIL);      
  }                    
  *pt = TxFifo[ TxGetI &(FIFOSIZE-1)];  
  TxGetI++; 
  OS_Signal(&TxRoomLeft);
  return(FIFOSUCCESS);     
}                      
uint32_t TxFifo_Size (void){  
 return  TxPutI - TxGetI;  
}