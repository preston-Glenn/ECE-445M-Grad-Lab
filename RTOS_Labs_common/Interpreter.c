// *************Interpreter.c**************
// Students implement this as part of EE445M/EE380L.12 Lab 1,2,3,4 
// High-level OS user interface
// 
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano 1/18/20, valvano@mail.utexas.edu
#include <stdint.h>
#include <stdio.h>
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Labs_common/ADC.h"
#include "../RTOS_Labs_common/ST7735.h"
#include "../inc/ADCT0ATrigger.h"
#include "../inc/ADCSWTrigger.h"
#include "../RTOS_Labs_common/UART0int.h"
#include "../RTOS_Labs_common/eDisk.h"
#include "../RTOS_Labs_common/eFile.h"
#include "../inc/LED.h"


#include <stdio.h>
#include <stdlib.h>
#include "Interpreter.h"
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Lab5_ProcessLoader/loader.h"
#include "../RTOS_Labs_common/heap.h"

extern int32_t MaxJitter; // largest time jitter between interrupts in usec
extern uint32_t const JitterSize;
extern uint32_t JitterHistogram[JITTERSIZE];
extern int32_t MaxJitter2; // largest time jitter between interrupts in usec
extern uint32_t const JitterSize2;
extern uint32_t JitterHistogram2[JITTERSIZE];

void Jitter(int32_t MaxJitter, uint32_t const JitterSize, uint32_t JitterHistogram[]);
void Plot_Histogram(uint32_t const Size, uint32_t DataArray[], int32_t MaxData, int32_t MinData, uint32_t Color, uint8_t ScreenNumber);
char* my_strtok(char* str, const char* delimiters);
int strcmp(const char* str1, const char* str2);


void HELP(){
  UART_OutString("help: lists all commands\n\r");
  UART_OutString("Timer: prints os time\n\r");
	UART_OutString("Clear: clears os time\n\r");
  UART_OutString("LCD: prints out ADC value to LCD. It then asks for (T,B) and then (0-7)\n\r");
  UART_OutString("ADC: prints out ADC value to serial\n\r");
  UART_OutString("LED: Allows the user to toggle LED, asks for (R,G,B)\n\r");
  UART_OutString("\n\r");

}

uint32_t adc;

extern char const string1[], string2[], string3[], string4[];

//char const string1[]="Filename = %s";
//char const string2[]="File size = %lu bytes";
//char const string3[]="Number of Files = %u";
//char const string4[]="Number of Bytes = %lu";

extern void TestDirectory(void);
heap_stats_t stats1;
void heapStats1(void){
  if(Heap_Stats(&stats1))  //heapError("Heap_Stats","",0);
  UART_OutString("\n\rHeap size  ="); UART_OutUDec(stats1.size); 
  UART_OutString("\n\rHeap used  ="); UART_OutUDec(stats1.used);
  UART_OutString("\n\rHeap free  ="); UART_OutUDec(stats1.free);
  UART_OutString("\n\rHeap waste ="); UART_OutUDec(stats1.size - stats1.used - stats1.free);
  UART_OutString("\n\r");
}

static const ELFSymbol_t symtab[] = { 
    { "ST7735_Message", ST7735_Message } // address of ST7735_Message
}; 

void LoadProgram() { 
  ELFEnv_t env = { symtab, 1 }; // symbol table with one entry
  if (!exec_elf("User.axf", &env)) {
    UART_OutString("Error loading program\n\r");
  } else {
    UART_OutString("Program loaded\n\r");
  }
} 
// *********** Interpreter ************
// Test LED, ADC, LCD, Timer, and UART
void PROCESS_CMD(char* in){

	char* cmd = my_strtok(in, " ");

	
	char *arg = my_strtok(NULL, " ");
	
	char data;


  if(strcmp(in,"help") == 0){
    //UART_OutString("\n\rhelp: lists all commands\n\r");
    HELP();
  } else if(strcmp(in,"LED")==0){
    // ask what color through uart
    UART_OutString("What color? (R,G,B)\n\r");
    char color[100];
    UART_InString(color,100);
    if(strcmp(color,"R")==0){
      // turn on red led
      LED_RedToggle();
    } else if(strcmp(color,"G")==0){
      // turn on green led
      LED_GreenToggle();
    } else if(strcmp(color,"B")==0){
      // turn on blue led
      LED_BlueToggle();
    } else {
      UART_OutString("Invalid color\n\r");
    }


  } else if(strcmp(in,"ADC")==0){
    UART_OutString("ADC: ");
    adc = ADC_In();
    char str[100];
    sprintf(str,"%d",adc);
    UART_OutString(str);
    UART_OutString("\n\r");
  }  else if(strcmp(in,"LCD")==0){
    // ask top or bottom
		UART_OutString("What would you like to print?\n\r");
		char val[100];
		UART_InString(val,100);
		

    UART_OutString("\n\rTop or bottom? (T,B)\n\r");
    char pos[5];
    UART_InString(pos,5);
    // ask wat line
    UART_OutString("\n\rWhat line? (0-7)\n\r");
    char line[5];
    UART_InString(line,5);

    // // print adc to lcd
    // char str[100];
    // sprintf(str,"%d",adc);
    if(strcmp(pos,"T")==0){
      ST7735_Message(0,atoi(line),val,-1);
    } else if(strcmp(pos,"B")==0){
      ST7735_Message(1,atoi(line),val,-1);
    } else {
      UART_OutString("Invalid position\n\r");
    }


   
  } else if(strcmp(in,"Timer")==0){ // OS
    // print to uart os time
    UART_OutString("OS time: ");
    char str[100];
    sprintf(str,"%d",OS_MsTime());
    UART_OutString(str);
    UART_OutString("\n\r");
	} else if(strcmp(in,"Clear")==0){
		OS_ClearMsTime();

  } else if(strcmp(in,"Display")==0){
    DisplayStatus();
  } else if(strcmp(in,"HP")==0){
		UART_OutString("\n\rWhich Jitter? (0-1)\n\r");
    char line[5];
    UART_InString(line,5);
		UART_OutString("\n\rHistogram\n\r");

		if(atoi(line)==0){
				Plot_Histogram(JitterSize, JitterHistogram, MaxJitter, 0, ST7735_CYAN, 1);
		}else{
			Plot_Histogram(JitterSize2, JitterHistogram2, MaxJitter2, 0, ST7735_CYAN, 1);
		}
		
	} else if(strcmp(in,"J")==0){
		UART_OutString("\n\rWhich Jitter? (0-1)\n\r");
    char line[5];
    UART_InString(line,5);
		UART_OutString("\n\rHistogram\n\r");

		if(atoi(line)==0){
      Jitter(MaxJitter, JITTERSIZE, JitterHistogram);
		}else{
      Jitter(MaxJitter2, JITTERSIZE, JitterHistogram2);
		}
		
	} else if(strcmp(in,"\n\r")==0){
		return;
	}else if(strcmp(cmd,"format")==0){
//		eFile_Init();
		eFile_Format();

  } else if(strcmp(cmd,"mount")==0){
		eFile_Init();
    eFile_Mount();


  } else if(strcmp(cmd,"unmount")==0){

    eFile_Unmount();


  } 
	
	else if(strcmp(cmd,"touch")==0){
    if(eFile_Create(arg)==0){
      UART_OutString("\n\rFile created\n\r");
    } else {
      UART_OutString("\n\rFile not created\n\r");
    }


  } else if (strcmp(cmd, "print_dir") == 0){
		TestDirectory();
	}
		else if(strcmp(cmd,"write")==0){
    // ask for data
    UART_OutString("\n\rWhat data would you like to write?\n\r");
    char data[1024];
    UART_InString(data,1024);



    // write to file
    if(eFile_WOpen(arg)==0){
      UART_OutString("\n\rFile opened\n\r");
      int count = 0;
      int bad_write = 0;
      while(count < 1024 && data[count] != '\0'){ 
        if(eFile_Write(data[count])!=0){
          bad_write = 1;
        }
        count++;
      }
      if(bad_write == 1){
        UART_OutString("\n\rWrite error\n\r");
      } else {
        UART_OutString("\n\rWrite successful\n\r");
      }
      // close file
      if(eFile_WClose()==0){
        UART_OutString("\n\rFile closed\n\r");
      } else {
        UART_OutString("\n\rFile not closed\n\r");
      }
    } else {
      UART_OutString("\n\rFile not opened\n\r");
    }



  } else if(strcmp(cmd,"read")==0){
    // read from file
    if(eFile_ROpen(arg)==0){
      UART_OutString("\n\rFile opened\n\r");

     //diskError("eFile_ROpen",0);
    while(eFile_ReadNext(&data) == 0)    UART_OutChar(data);			
  

    } else {
      UART_OutString("\n\rFile not opened\n\r");
    }
    // close file
    if(eFile_RClose() == 0){
      UART_OutString("\n\rFile closed\n\r");
    } else {
      UART_OutString("\n\rFile not closed\n\r");
    }



  } else if(strcmp(cmd,"delete")==0){
    if(eFile_Delete(arg)==0){
      UART_OutString("\n\rFile deleted\n\r");
    } else {
      UART_OutString("\n\rFile not deleted\n\r");
    }
  } else if(strcmp(cmd,"exec")==0){
    LoadProgram();

  } else if(strcmp(cmd,"heap")==0){
    heapStats1();
  } else {
    UART_OutString("\n\rInvalid command\n\r");
  }

}



void DisplayStatus(void){

	// UART_OutString("Jitter 0.1us===");

//	ST7735_Message(1,0,"NumCreated===",NumCreated); 
	//ST7735_Message(1,3,"Jitter 0.1us===",MaxJitter);

	
  //Jitter(MaxJitter, JITTERSIZE, JitterHistogram);

}


// Print jitter histogram
void Jitter(int32_t MaxJitter, uint32_t const JitterSize, uint32_t JitterHistogram[]){
  // ST7735_Message(0,7,"Jitter 0.1us===",MaxJitter);

	// Plot_Histogram(JitterSize, JitterHistogram, MaxJitter, 0, ST7735_CYAN, 1);
  UART_OutString("Jitter 0.1us = ");
  UART_OutUDec((MaxJitter));
  UART_OutString("\n\r");
  // print histogram using st7735 library
  // Plot_Histogram(JitterSize,JitterHistogram,MaxJitter,0,ST7735_BLUE,1);  
}




void Plot_Histogram(uint32_t const Size, uint32_t DataArray[], int32_t MaxData, int32_t MinData, uint32_t Color, uint8_t ScreenNumber){
  // print histogram using st7735 library

    /*
  uint8_t num_of_blocks = Size/2;
  int i;
  for(i=0;i<Size;i+=2){
    uint16_t x = (ST7735_TFTWIDTH/num_of_blocks)i;
    uint16_t y,w,h;
    int sum = DataArray[i]+DataArray[i+1];
    uint16_t y_offset = ((sum)/MaxData)(ST7735_TFTHEIGHT/2);
    if(ScreenNumber == 0) y = ST7735_TFTHEIGHT/2 - y_offset;
    if(ScreenNumber == 1) y = ST7735_TFTHEIGHT   - y_offset;

    w = x + ST7735_TFTWIDTH/num_of_blocks;
    h = y_offset;
    ST7735_FillRect(x,y,w,h, Color);

  }
    */

    // finding biggest value
    int maxvalue = 0;
    for(int i = 0; i < Size; i++)    {
        if(DataArray[i] > maxvalue)    {
            maxvalue = DataArray[i];
        }
    }

    int scale = 50;
    int j = 0;
    for(int i = 0; i < Size; i++)    {
        UART_OutUDec(i);
        UART_OutString(" ");
				if(i <= 9)        UART_OutString(" ");
        UART_OutString("| ");
				uint8_t blocks = (scale * (DataArray[i]/maxvalue));
        for(int j = 0; j < blocks; j++)    {
            UART_OutChar('*');
        }
				if(blocks == 0)             UART_OutChar('*');
				UART_OutUDec(DataArray[i]);
        UART_OutString("\n\r");
    }


}

//void Plot_Histogram(uint32_t const Size, uint32_t DataArray[], int32_t MaxData, int32_t MinData, uint32_t Color, uint8_t ScreenNumber){
//  // print histogram using st7735 library
///*
//  uint8_t num_of_blocks = Size/2;
//  int i;
//  for(i=0;i<Size;i+=2){
//    uint16_t x = (ST7735_TFTWIDTH/num_of_blocks)*i;
//    uint16_t y,w,h;
//    int sum = DataArray[i]+DataArray[i+1];
//    uint16_t y_offset = ((sum)/MaxData)*(ST7735_TFTHEIGHT/2);
//    if(ScreenNumber == 0) y = ST7735_TFTHEIGHT/2 - y_offset;
//    if(ScreenNumber == 1) y = ST7735_TFTHEIGHT   - y_offset;

//    w = x + ST7735_TFTWIDTH/num_of_blocks;
//    h = y_offset;
//    ST7735_FillRect(x,y,w,h, Color);

//  }
//*/
//  
//}




// *********** Command line interpreter (shell) ************
void Interpreter(void){ 
  // write this  

	UART_OutString("\n\rStarting Interpreter\n\r ");

  while(1){
		char cmd[100];
    UART_OutString("\n\rCMD> ");
    UART_InString(cmd,100);
    UART_OutString("\n\r");
    PROCESS_CMD(cmd);
    DisplayStatus();

  }

 



}

char* my_strtok(char* str, const char* delimiters) {
    static char* token = NULL;
    if (str) {
        token = str;
    } else if (!token) {
        return NULL;
    }

    char* result = token;
    while (*token) {
        const char* d = delimiters;
        while (*d) {
            if (*token == *d) {
                *token++ = '\0';
                return result;
            }
            d++;
        }
        token++;
    }
    token = NULL;
    return result;
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 && *str2) {
        if (*str1 != *str2) {
            return (*str1 > *str2) ? 1 : -1;
        }
        str1++;
        str2++;
    }
    return (*str1 == *str2) ? 0 : ((*str1 > *str2) ? 1 : -1);
}


