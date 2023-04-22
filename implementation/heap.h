/**
 * @file      heap.h
 * @brief     heap memory manager
 * @details   Dynamic memory management on a heap
 * @version   V1.0
 * @author    Valvano (originally by Jacob Egner)
 * @copyright Copyright 2020 by Jonathan W. Valvano, valvano@mail.utexas.edu,
 * @warning   AS-IS
 * @note      For more information see  http://users.ece.utexas.edu/~valvano/
 * @date      Jan 12, 2020
 ******************************************************************************/

#ifndef HEAP_H
#define HEAP_H


// struct for holding statistics on the state of the heap
typedef struct heap_stats {
  unsigned int size;   // heap size (in bytes)
  unsigned int used;   // number of bytes used/allocated
  unsigned int free;   // number of bytes available to allocate
  unsigned int* text;
  unsigned int* data;
} heap_stats_t;

/*struct PCB {
 int ID;
 int priority;
 int num_threads;
 void* text;              ********* Defined in OS.h *************
 void* data;
   
};
typedef struct PCB PCB;
typedef struct PCB* PCBptr;
*/

typedef enum {CHILD, PARENT} ThreadType;

/**
 * @details Initialize the Heap
 * @param  none
 * @return always 0 (success)
 * @brief  Initializes/resets the heap to a clean state where no memory
 *         is allocated.
 */
int Heap_Init(void);


/**
 * @details Allocate memory, data not initialized
 * @param  desiredBytes: desired number of bytes to allocate
 * @return void* pointing to the allocated memory or will return NULL
 *         if there isn't sufficient space to satisfy allocation request
 * @brief  Allocate memory
 */
void* Heap_Malloc(int desiredBytes);


/**
 * @details Allocate memory, allocated memory is initialized to 0 (zeroed out)
 * @param  desiredBytes: desired number of bytes to allocate
 * @return void* pointing to the allocated memory block or will return NULL
 *         if there isn't sufficient space to satisfy allocation request
 * @brief Zero-allocate memory
 */
void* Heap_Calloc(int desiredBytes);


/**
 * @details Reallocate buffer to a new size. The given block may be 
 *          unallocated and its contents copied to a new block
 * @param  oldBlock: pointer to a block
 * @param  desiredBytes: a desired number of bytes for a new block
 * @return void* pointing to the new block or will return NULL
 *         if there is any reason the reallocation can't be completed
 * @brief  Grow/shrink memory
 */
void* Heap_Realloc(void* oldBlock, int desiredBytes);


/**
 * @details Return a block to the heap
 * @param  pointer to memory to unallocate
 * @return 0 if everything is ok, non-zero in case of error (e.g. invalid pointer
 *         or trying to unallocate memory that has already been unallocated)
 * @brief  Free memory
 */
int Heap_Free(void* pointer);


/**
 * @details Return the current usage status of the heap
 * @param  reference to a heap_stats_t that returns the current usage of the heap
 * @return 0 in case of success, non-zeror in case of error (e.g. corrupted heap)
 * @brief  Get heap usage
 */
int Heap_Stats(heap_stats_t *stats);


#endif //#ifndef HEAP_H
