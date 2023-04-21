#include "fb.h"
#include "io.h"

// The screen
#define WIDTH         1920
#define HEIGHT        1080
#define MARGIN        30
#define VIRTWIDTH     (WIDTH-(2*MARGIN))
#define FONT_BPG      8

// For the bricks
#define ROWS          5
#define COLS          10
unsigned int bricks = ROWS * COLS;

// Gameplay
#define NUM_LIVES     3

// OBJECT TRACKING



int counter(){
    int i = 0;
    int count = 0;
    while(1){
        i+=2;
        if(i % 100000 == 0){
            count++;

            drawString((WIDTH/2)-252, MARGIN-25, "Score: 0     Lives:  ", 0x0f, 3);
            drawChar(count + 0x30, (WIDTH/2)-252 + (8*8*3), MARGIN-25, 0x0f, 3);
            OS_Suspend();
        }
        if(count == 10){
            // print closing thread
            drawString((WIDTH/2)-252, MARGIN-25, "Killing Thread", 0x0f, 3);
            OS_Kill();
            
        }
    }
}

int counter7(){
    int i = 0;
    int count = 0;
    while(1){
        i+=7;
        if(i % 100000 == 0){
            count++;
            // TODO: add semaphores.....
            drawString((WIDTH/2)+252, MARGIN-25, "Score: 0     Lives:  ", 0x0f, 3);
            drawChar(count + 0x30, (WIDTH/2)+252 + (8*8*3), MARGIN-25, 0x0f, 3);
            OS_Suspend();

        }
        if(count == 10){
            // print closing thread
            drawString((WIDTH/2)-252, MARGIN-25, "Killing Thread", 0x0f, 3);
            OS_Kill();
            
        }
    }
}

// never dies
int idle(){
    int i = 0;
    while(1){
        // do nothing
        i++;
        OS_Suspend();
    }
}





void main()
{
    struct Object *foundObject;
    unsigned char ch = 0;

    int lives = NUM_LIVES;
    int points = 0;
    
    int velocity_x = 1;
    int velocity_y = 3;

    OS_Init();



    int zoom = WIDTH/192;
    int strwidth = 10 * FONT_BPG * zoom;
    int strheight = FONT_BPG * zoom;

    // delay
    int counts = 0;
    while(counts < 10000){
        counts++;
    }

    counts = 0;

    //if (bricks == 0) 
    drawString((WIDTH/2)-(strwidth/2), (HEIGHT/2)-(strheight/2), "Well donf!", 0x02, zoom);
    //else drawString((WIDTH/2)-(strwidth/2), (HEIGHT/2)-(strheight/2), "Game over!", 0x04, zoom);

    // add threads
    OS_AddThread(&counter, 1);
    OS_AddThread(&counter7, 1);
    OS_AddThread(&idle, 3);


    // delay
    while(counts < 10000){
        counts++;
    }

    // start threads
    OS_Launch();

    // if we get here, something went wrong
    drawString((WIDTH/2)-(strwidth/2), (HEIGHT/2)-(strheight/2), "FAILED", 0x02, zoom);

    while (1){};
}
