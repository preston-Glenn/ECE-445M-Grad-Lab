// filename *************************heap.c ************************
// Implements memory heap for dynamic memory allocation.
// Follows standard malloc/calloc/realloc/free interface
// for allocating/unallocating memory.

// Jacob Egner 2008-07-31
// modified 8/31/08 Jonathan Valvano for style
// modified 12/16/11 Jonathan Valvano for 32-bit machine
// modified August 10, 2014 for C99 syntax

/* This example accompanies the book
   "Embedded Systems: Real Time Operating Systems for ARM Cortex M Microcontrollers",
   ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2015

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


#include "heap.h"


#define HEAP_SIZE (1024 * 2 ) // 4KB heap


// TODO: remove heap and use the currentThread's heap in its PCB
static int heap[HEAP_SIZE]; // static so it is not on the stack

static heap_stats_t heap_stats;

// extern currentThread
extern TCBptr currentThread;

//******** Heap_Init *************** 
// Initialize the Heap
// input: none
// output: always 0
// notes: Initializes/resets the heap to a clean state where no memory
//  is allocated.
int Heap_Init(void){

  // Initialize the heap to a clean state where no memory is allocated
  // Initialize the PCB list of allocated blocks to be empty

  // clear the heap
  for(int i = 0; i < HEAP_SIZE; i++) {
    heap[i] = 0;
  }

  // set index to indicate that the heap is empty/ free / size of heap
  heap[0] = -1 * (HEAP_SIZE - 2); // size of heap
  heap[HEAP_SIZE-1] = -1 * (HEAP_SIZE - 2); // size of heap

  heap_stats.size = (HEAP_SIZE) * sizeof(int);
  heap_stats.used = 0;
  heap_stats.free = (HEAP_SIZE - 2) * sizeof(int);; 
  return 0;   // replace
}


//******** Heap_Malloc *************** 
// Allocate memory, data not initialized
// input: 
//   desiredBytes: desired number of bytes to allocate
// output: void* pointing to the allocated memory or will return NULL
//   if there isn't sufficient space to satisfy allocation request
void* Heap_Malloc(int desiredBytes){
  if(desiredBytes <= 0) {
    return NULL;
  }

  int numBlocks = (desiredBytes + 3) / 4; // number of 4 byte blocks needed
  //make numBlocks even
  if(numBlocks % 2 != 0) {
    numBlocks++;
  }
  // Find a free memory block that is large enough to satisfy the request
  // If no free block is large enough, return NULL
  // If a free block is large enough, allocate it and return a pointer to it

  // loop through the heap to find a free block
  // if the block is free, check if it is large enough
  // if the block is not large enough, continue to the next block
  // if the block is large enough, allocate it and return a pointer to it

  // If no free block is large enough, return NULL

  // get first value of heap
  // if the value is negative, the block is free
  // if the value is positive, the block is allocated
  unsigned int *heapPtr = &heap[0];
  int index = 0;
  int value = heapPtr[index];
	int a;

  while(value + numBlocks > 0 && index < HEAP_SIZE) {
  
    index += abs(value) + 2;
    value = heapPtr[index];
	if(value == 0)  // and index == 2046; TODO: how to handle edge cases of not enough room.
		return NULL;

  }

    if(index >= HEAP_SIZE)return NULL;   // NULL
  
  value *= -1; // switch value to positive

  // Allocate the block
  // If the free block is larger than the requested size, split the block
  // If the free block is not larger than the requested size, allocate the entire block
  if(value == numBlocks){
    // allocate the entire block
    heapPtr[index] = numBlocks;
    heapPtr[index + numBlocks + 1] = numBlocks;
    heap_stats.free -= numBlocks * sizeof(int);
    heap_stats.used += numBlocks * sizeof(int);


  } else if(value > numBlocks) {
    // split the block
    heapPtr[index] = numBlocks;
    heapPtr[index + numBlocks + 1] = numBlocks;
    heapPtr[index + numBlocks + 2] = (value - numBlocks - 2) * -1;
    heapPtr[index + value + 1] = (value - numBlocks - 2) * -1;
    
    heap_stats.free -= numBlocks * sizeof(int) + 8;
    heap_stats.used += numBlocks * sizeof(int);

  }




  return &heapPtr[index + 1];   // NULL
}


//******** Heap_Calloc *************** 
// Allocate memory, data are initialized to 0
// input:
//   desiredBytes: desired number of bytes to allocate
// output: void* pointing to the allocated memory block or will return NULL
//   if there isn't sufficient space to satisfy allocation request
//notes: the allocated memory block will be zeroed out
void* Heap_Calloc(int desiredBytes){  

  int numBlocks = (desiredBytes + 3) / 4; // number of 4 byte blocks needed

  // Call Heap_Malloc to allocate the memory
  // If the allocation is successful, zero out the memory block
  void *ptr = Heap_Malloc(desiredBytes);
  if(ptr != NULL) {
    unsigned int *heapPtr = ptr;
    for(int i = 1; i < numBlocks; i++) {
      heapPtr[i] = 0;
    }
  }


 
  return ptr;   // NULL
}


//******** Heap_Realloc *************** 
// Reallocate buffer to a new size
//input: 
//  oldBlock: pointer to a block
//  desiredBytes: a desired number of bytes for a new block
// output: void* pointing to the new block or will return NULL
//   if there is any reason the reallocation can't be completed
// notes: the given block may be unallocated and its contents
//   are copied to a new block if growing/shrinking not possible
void* Heap_Realloc(void* oldBlock, int desiredBytes){

  int numBlocks = (desiredBytes + 3) / 4; // number of 4 byte blocks needed
  
  // If the oldBlock is NULL, call Heap_Malloc to allocate a new block
  if(oldBlock == NULL) {
    return Heap_Malloc(desiredBytes);
  }

  // If the desiredBytes is 0, call Heap_Free to unallocate the oldBlock
  if(desiredBytes == 0) {
    Heap_Free(oldBlock);
    return NULL;
  }

  // If the oldBlock is not NULL and the desiredBytes is not 0,
  //   check if the oldBlock is large enough to satisfy the request
  // If the oldBlock is larger than the desiredBytes, split the block
  
  // Get the size of the oldBlock
  unsigned int *heapPtr = &heap[0];

	
  int index = (((unsigned int)oldBlock - (unsigned int)heap) / 4) - 1;
  int oldBlockSize = heap[index];
  
  if(oldBlockSize == numBlocks) {
    // do nothing
    return oldBlock;
  } else {
    // If the oldBlock is not large enough, call Heap_Malloc to allocate a new block
    Heap_Free(oldBlock);
    return Heap_Malloc(desiredBytes);
  }

  return 0;   // NULL
}


//******** Heap_Free *************** 
// return a block to the heap
// input: pointer to memory to unallocate
// output: 0 if everything is ok, non-zero in case of error (e.g. invalid pointer
//     or trying to unallocate memory that has already been unallocated
int Heap_Free(void* pointer){

  // If the pointer is NULL, return 0
  if(pointer == NULL) {
    return 0;
  }




  // Get the index of the block
  int index = (((unsigned int)pointer - (unsigned int)heap) / 4) - 1;
  // Get the size of the block
  int blockSize = heap[index];



  if(blockSize < 0) return 1;
  

  // Set the block to free
  heap[index] *= -1;
  heap[index + blockSize + 1] *= -1;

  // update stats
  heap_stats.free += blockSize * 4 ;
  heap_stats.used -= blockSize * 4;

  // check if the previous block is free, merge the blocks if it is
  if((index - 1 >= 0) && heap[index - 1] < 0) {
    // merge the blocks
    int newSize = heap[index - 1] + heap[index] - 2;
    heap[index - 1 + heap[index - 1] - 1] = newSize; // update previous blocks first value
    heap[index - heap[index] + 1] = newSize;

    int newindex = index + heap[index - 1] - 2;
    heap[index] = 0;
    heap[index-1] = 0;

    index = newindex;
    blockSize = -1 * newSize;

    // update stats
    heap_stats.free += 8;
    // heap_stats.used -= 8;

  }
  if(index + blockSize + 1 >= HEAP_SIZE) return 0; // skip the next check if we are at the end of the heap
  // check if the next block is free, merge the blocks if it is
  int bottomValue = heap[index + blockSize + 1];
  int topSize = heap[index + blockSize + 2];
  if(topSize < 0) {
    // merge the blocks
    int newSize = topSize + bottomValue - 2;

    heap[index] = newSize;
    heap[index - newSize + 1] = newSize;

    heap[index + blockSize + 1] = 0;
    heap[index + blockSize + 2] = 0;

    // update stats
    heap_stats.free += 8;
    // heap_stats.used -= 8;
		
  }

 
  return 0;   // replace
}


//******** Heap_Stats *************** 
// return the current status of the heap
// input: reference to a heap_stats_t that returns the current usage of the heap
// output: 0 in case of success, non-zeror in case of error (e.g. corrupted heap)
int Heap_Stats(heap_stats_t *stats){
  
  // copy the heap_stats to the stats
  stats->free = heap_stats.free;
  stats->used = heap_stats.used;
  stats->size = heap_stats.size;
	
	return 0;
  
}





// TODO: CHeck what happens on merge if value  is 0.