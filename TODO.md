

The main objective of this graduate lab is to implement a working base OS that utilizes virtual memory. 

The initial goal will be to allow each process to have its own memory space allocation.  ( I dont know if would be sufficient or not. How do we allocate Code and Data Segments for it???) Once that is completed we will investigate a secondary bootloader.


To that end we will be using a RaspberryPi since it has an MMU. The first part of the project will simply be porting over our Lab3 OS to the RP. The next part will be implementing the Virtual Memory Management.


# Steps.
1. Get hello world bootup working RP.
2. Implement .s code that manages bootup (This may be part of step 1.)
3. Get UART/ print statements working
3. Port over OS to RP.
- get priority scheduler working.
**************************
4. Design VM management with thread(consider with process)
5. Start implementation (for threads)
**************************
6. Add 2ndary bootloader to code
7. Update VMM to support proccesses.

8 IMPORTANT ** RESEARCH PAPERS about it.








1. Get serial connection working
2. Get systick and pensv working
3. Get add thread working