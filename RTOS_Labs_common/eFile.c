// filename ************** eFile.c *****************************
// High-level routines to implement a solid-state disk 
// Students implement these functions in Lab 4
// Jonathan W. Valvano 1/12/20
#include <stdint.h>
#include <string.h>
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Labs_common/eDisk.h"
#include "../RTOS_Labs_common/eFile.h"
#include <stdio.h>


const uint16_t MAX_SECTOR_DATA = 502;


// Total directory entry size: 16 bytes 

typedef struct {
  char     name[8];         // name of file; 8 bytes
  DWORD     sector;         // pointer to sector; 4 bytes
  uint32_t  size;           // 4 bytes TODO: should be 2 or 4??? 4 is to big for one sector
} directory_entry_t;        //

typedef struct {
  
  DWORD next_sector;       // Pointer to next sector; last sector in file if 0
  DWORD prev_sector;  // Pointer to previous sector; first sector in file if itself
  uint16_t size;      // 2 bytes required for storing size in sector; max (512-4-4-2) = 506 bytes
  BYTE data[MAX_SECTOR_DATA];         // data in sector
} sector_entry_t;



directory_entry_t   directory[32];
directory_entry_t  *current_file;
sector_entry_t     current_sector;
int last_sector_open = -1;
int file_position = 0;

int ReaderCount = 0;
int WriterCount = 0;


//---------- eFile_Init-----------------
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
int eFile_Init(void){ // initialize file system
 
	//OS_Wait(&LCDFree);
	
	
  DSTATUS result = eDisk_Init(0);

  if(result)	{
		//OS_Signal(&LCDFree);
    return 1;
	}
  else	{
		//OS_Signal(&LCDFree);
    return 0;
	}

  //return 1;   // replace
}

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Format(void){ // erase disk, add format

	//OS_Wait(&LCDFree);
	
	//unsigned long lock = OS_LockScheduler();
	
	int status = StartCritical();
	
  // Directory set up

  directory_entry_t initial = {"", 0, 0};
  
  for(int i = 0; i < NUM_FILES - 1; i++) {
    directory[i] = initial;
  }

  directory[NUM_FILES - 1] = (directory_entry_t){"_FREE_", 1, (NUM_SECTORS - 1)};

  // Write directory to sector 0
  if(eDisk_WriteBlock((BYTE *)&directory, 0)) {
		return 1;
	}

  // Complete rest of free-space sectors

  for(int i = 1; i < NUM_SECTORS - 1; i++)  {
    sector_entry_t temp = { i+1, i-1, 0, {0} };

    if(eDisk_WriteBlock((BYTE *)&temp, i)) {
			//OS_Signal(&LCDFree);
			return 1;
		}
  }

  sector_entry_t temp = { 0, NUM_SECTORS - 2, 0, {0} };

  if(eDisk_WriteBlock((BYTE *)&temp, NUM_SECTORS - 1)) return 1;

	//OS_Signal(&LCDFree);
	
	EndCritical(status);
	
	//OS_UnLockScheduler(lock);
	
  return 0;

  //return 1;   // replace
}

//---------- eFile_Mount-----------------
// Mount the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure
int eFile_Mount(void){ // initialize file system
	
	//OS_Wait(&LCDFree);
	

  // Read directory into RAM

  //BYTE *dir;
	
	BYTE dir[512];

  if(eDisk_ReadBlock(dir, (DWORD)0))
    return 1;
  
  directory_entry_t *dirs = (directory_entry_t *) &dir[0];
  memcpy(directory, dirs, sizeof(directory_entry_t) * 32);
	
	//OS_Signal(&LCDFree);
	

  return 0;


  //return 1;   // replace
}


//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create( const char name[]){  // create new file, make it empty 

	//OS_Wait(&LCDFree);
	
  directory_entry_t *new_file = &directory[NUM_FILES-1];

  int i = 0;
  // Check if file already exists
  for(i = 0; i < NUM_FILES-1; i++) {
    if(strcmp(directory[i].name, name) == 0) {
      //OS_Signal(&LCDFree);
      return 1;
    }
  }


  i = 0;
  for(i = 0; i < NUM_FILES-1; i++) {

    // Check for empty space
    if(strcmp(directory[i].name, "") == 0 && directory[i].sector == 0)
      break;
    
  }

  // No empty file place found
  if(i == 30 && directory[30].sector != 0)
    return 1;
  
  // Create new directory entry
  strcpy(directory[i].name, name);
  directory[i].sector = new_file->sector;
  directory[i].size = 0;

  // Check for error reading from sector
  BYTE block[512];
  if(eDisk_ReadBlock(block, new_file->sector))  return 1;

  sector_entry_t* entry = (sector_entry_t *)&block;

  //Update sector metadata
  sector_entry_t temp = {0, new_file->sector, 0, {0} };

  // Write metadata to sector
  if(eDisk_WriteBlock((BYTE *)&temp, new_file->sector)) return 1;

  // Update free-space manager
  new_file->sector = entry->next_sector;  // Update free-space sector
  new_file->size = new_file->size - 1;  // Update size

  // Write free-space manager to disk
  if(eDisk_WriteBlock((BYTE *)&directory, 0)) return 1;

	//OS_Signal(&LCDFree);
	
	
  return 0;

  //return 1;   // replace
}


//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is an ASCII string up to seven characters
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen( const char name[]){      // open a file for writing 


	//OS_Wait(&LCDFree);
  // TODO: Check whether other files open

  // Find the file in directory
	
	if(WriterCount == 1 || ReaderCount == 1)
		return 1;

  int i = 0;

  for(i = 0; i < NUM_FILES-1; i++) {
    if(strcmp(directory[i].name, name) == 0)
      break;
  }

  // If file not found
  if(i == NUM_FILES - 1 )  return 1;

  current_file = &directory[i];

  // Find the last sector of the file

  BYTE block[512];

  if(eDisk_ReadBlock(block, current_file->sector)) return 1;
	
	memcpy(&current_sector, block, 512);
	//current_sector = (sector_entry_t)&block;
	last_sector_open = (int)current_file->sector;
	

  while(current_sector.next_sector != 0) {

    last_sector_open = (int)current_sector.next_sector;

    if(eDisk_ReadBlock(block, current_sector.next_sector))  {
      last_sector_open = -1;
      return 1;
    }
		memcpy(&current_sector, block, 512);
    //current_sector = (sector_entry_t *)block;
  }

  // Update writer count
  WriterCount = 1;

	//OS_Signal(&LCDFree);
	
  return 0;

  //return 1;   // replace  
}

//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write( const char data){
  
		//OS_Wait(&LCDFree);
	
	
    if(current_sector.size < MAX_SECTOR_DATA)  {
      current_sector.data[current_sector.size] = data;
      current_sector.size = current_sector.size + 1;
      
      current_file->size = current_file->size + 1;
			
			//if(current_file->size == 492)	{
			//	int test = 1;
			//	printf("Test");
				
			//}

      //BYTE block[512];
			//BYTE block1[512];
			//BYTE *block_ptr;
			
			//block_ptr = block;
      //block = (BYTE *)&current_sector;
			
			//sector_entry_t temp;
			
			//temp.next_sector = current_sector.next_sector;
			//temp.prev_sector = current_sector.prev_sector;
			//temp.size = current_sector.size;
			//for(int i = 0; i < MAX_SECTOR_DATA; i++)
				//temp.data[i] = current_sector.data[i];
			
			//block_ptr = (BYTE *)current_sector;
			
			//memcpy(block, &temp, 512);
			//memcpy(block, current_sector, 512);

      //if(eDisk_WriteBlock(block, last_sector_open)) return 1;

			//if(eDisk_ReadBlock(block1, last_sector_open)) return 1;
    }
    else  {

            BYTE block[512];
			      BYTE block1[512];

            // Next sector

            directory_entry_t* free = &directory[NUM_FILES-1];

            current_sector.next_sector = free->sector;

            // Write back the current sector
            if(eDisk_WriteBlock((BYTE *)&current_sector, last_sector_open)) return 1;

            // Fetch the new sector
            
            if(eDisk_ReadBlock(block, free->sector))  return 1;

            int new_last_sector = free->sector;
            sector_entry_t* new_temp = (sector_entry_t*)&block;

            sector_entry_t temp = {0, last_sector_open, 0, {0}};

            free->sector = new_temp->next_sector;    // Update free-space
            free->size = free->size - 1;      // Update size
						
						// Write back the updated directory
						if(eDisk_WriteBlock((BYTE *)&directory, 0)) return 1;

            temp.data[0] = data;
            temp.size = temp.size + 1;
            current_file->size = current_file->size + 1;

            last_sector_open = new_last_sector;

						current_sector = temp;
						
            if(eDisk_WriteBlock((BYTE *)&current_sector, last_sector_open)) return 1;

						//if(eDisk_ReadBlock(block1, last_sector_open)) return 1;
            
    }

		//OS_Signal(&LCDFree);
		
    return 0;

    //return 1;   // replace
}

//---------- eFile_WClose-----------------
// close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(void){ // close the file for writing
  

  // No file open in writing mode
  if(WriterCount == 0)
    return 1;
	
	if(eDisk_WriteBlock((BYTE *)&current_sector, last_sector_open)) return 1;
  
  current_file = NULL;
  current_sector = (sector_entry_t){ 0, 0, 0, {0} };
  WriterCount = 0;
  last_sector_open = -1;

	
  return 0;   // replace
}


//---------- eFile_ROpen-----------------
// Open the file, read first block into RAM 
// Input: file name is an ASCII string up to seven characters
// Output: 0 if successful and 1 on failure (e.g., trouble read to flash)
int eFile_ROpen( const char name[]){      // open a file for reading 

	
	//OS_Wait(&LCDFree);
	
	if(WriterCount == 1)
		return 1;
	
  BYTE block[512];

  int i = 0;

  for(i = 0; i < NUM_FILES-1; i++) {
    if(strcmp(directory[i].name, name) == 0)
      break;
  }

  // File not found
  if(i == NUM_FILES-1)  return 1;

  // Read first sector into RAM

  current_file = &directory[i];
  last_sector_open = current_file->sector;

  if(eDisk_ReadBlock(block, current_file->sector))  return 1;

	
  //current_sector = (sector_entry_t *)block;
		memcpy(&current_sector, block, 512);
	
  ReaderCount = 1;

	//OS_Signal(&LCDFree);
	
	
  return 0;

  //return 1;   // replace   
}
 
//---------- eFile_ReadNext-----------------
// retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext( char *pt){       // get next byte 
  
	//OS_Wait(&LCDFree);
	

  if(file_position == MAX_SECTOR_DATA)  {
    file_position = 0;

    // Update to next sector
    BYTE block[512];

    if(eDisk_ReadBlock(block, current_sector.next_sector)) return 1;
    //current_sector = (sector_entry_t *)&block;
		//memcpy(&current_sector, block, 512);
		sector_entry_t* new_temp = (sector_entry_t*)&block;
		current_sector = *new_temp;

		
  }


		*pt = current_sector.data[file_position];
  file_position++;
	
	//OS_Signal(&LCDFree);

	
  return 0;

  //return 1;   // replace
}
    
//---------- eFile_RClose-----------------
// close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(void){ // close the file for writing
  

  if(ReaderCount == 0)
    return 1;

  current_sector = (sector_entry_t){0, 0, 0, {0}};
  current_file = NULL;
  ReaderCount = 0;
  last_sector_open = -1;
	file_position = 0;

  return 0;

  //return 1;   // replace
}


//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete( const char name[]){  // remove this file

	//OS_Wait(&LCDFree);
  int i = 0;

  for(i = 0; i < NUM_FILES-1; i++) {
    if(strcmp(directory[i].name, name) == 0)
      break;
  }

  // File not found
  if(i == NUM_FILES-1)
    return 1;

  BYTE block[512];

  // Memory leak implementation

  strcpy(directory[i].name, "");
  directory[i].size = 0;
  directory[i].sector = 0;

  // To-Do: Update the sectors and add them to free-space


	//OS_Signal(&LCDFree);
	
	
	return 0;

  //return 1;   // replace
}                             


//---------- eFile_DOpen-----------------
// Open a (sub)directory, read into RAM
// Input: directory name is an ASCII string up to seven characters
//        (empty/NULL for root directory)
// Output: 0 if successful and 1 on failure (e.g., trouble reading from flash)
int eFile_DOpen( const char name[]){ // open directory
   
  // Only root directory implementation
  return 0;

  //return 1;   // replace
}
  
//---------- eFile_DirNext-----------------
// Retreive directory entry from open directory
// Input: none
// Output: return file name and size by reference
//         0 if successful and 1 on failure (e.g., end of directory)
int eFile_DirNext( char *name[], unsigned long *size){  // get next entry 
   
  // Only root directory implementation
	
	//OS_Wait(&LCDFree);
	
	
	static int i = 0;
	
	if(directory[i].size == 0 && strcmp(directory[i].name, "") == 0)	{
		i = 0;
		*name = NULL;
		*size = 0;
		return 1;
	}
	
	*name = directory[i].name;
	*size = directory[i].size;
	
	i++;
	
	//OS_Signal(&LCDFree);
	
	
  return 0;

  //return 1;   // replace
}

//---------- eFile_DClose-----------------
// Close the directory
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_DClose(void){ // close the directory
   
   // Only root directory implementation
   return 0;

  //return 1;   // replace
}


//---------- eFile_Unmount-----------------
// Unmount and deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently mounted)
int eFile_Unmount(void){ 
 
	//OS_Wait(&LCDFree);
	if(WriterCount == 1)	{
		if(eFile_WClose()) return 1;
	}
	
	if(ReaderCount == 1)	{
		if(eFile_RClose()) return 1;
	}
	
	if(eDisk_WriteBlock((BYTE *)&directory, 0)) return 1;
	
	//OS_Signal(&LCDFree);
  return 0;   // replace
}


int eFile_Size(){
	return current_file->size;
}