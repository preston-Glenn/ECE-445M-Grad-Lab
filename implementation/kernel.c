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
        if(i % 3 == 0){
            count++;

            drawString((WIDTH/2)+252, MARGIN-25, "Score: 0     Lives:  ", 0x0f, 3);
            drawChar(count + 0x30, (WIDTH/2)-252 + (8*8*3), MARGIN-25, 0x0f, 3);
            // OS_Sleep(1);
            // OS_Suspend();
        } else {
            drawString((WIDTH/2)+252, MARGIN-25, "Nope: 0     Dead:  ", 0x0f, 3);
            drawChar(count + 0x30, (WIDTH/2)-252 + (8*8*3), MARGIN-25, 0x0f, 3);
        }
        // if(count == 10){
        //     // print closing thread
        //     drawString((WIDTH/2)-252, MARGIN-25, "Killing Thread", 0x0f, 3);
        //     OS_Kill();
            
        // }
    }
}

int counter7(){
    int i = 0;
    int count = 0;
    while(1){
        i+=7;
        if(i % 2 == 0){
            count++;
            // TODO: add semaphores.....
            drawString((WIDTH/2)+252, MARGIN+25, "Game: 0     Done:  ", 0x0f, 3);
            drawChar(count + 0x30, (WIDTH/2)+252 + (8*8*3), MARGIN+25, 0x0f, 3);
            // OS_Suspend();

        }
        else {
            count++;
            // TODO: add semaphores.....
            drawString((WIDTH/2)+252, MARGIN+25, "Test: 0     Gers:  ", 0x0f, 3);
            drawChar(count + 0x30, (WIDTH/2)+252 + (8*8*3), MARGIN+25, 0x0f, 3);
            // OS_Suspend();

        }

    }
    // TODO: TEST THIS FIRST :: demos kill thread works!
    OS_Kill(); 
    drawString((WIDTH/2)+252, MARGIN+25, "Thread Failed to close!!", 0x0f, 3);

}

// never dies
int idle(){
    int i = 0;
    char *string = "0123456789";
    int count = 0;
    while(1){
        if((i % 100001) == 0)  {
        drawString((WIDTH/2)+252, MARGIN+60, "Idle", 0x0f, 3);
        //drawChar(count + 0x30, (WIDTH/2)+252, MARGIN+90, 0x0f, 3);

        i++;

        count++;

        // drawChar(i%10 + '0', (WIDTH/2)+252 + (8*8*3), MARGIN-25, 0x0f, 3);
        }
        // OS_Suspend();
    }
}

int true_idle(){
    while(1){
        OS_Suspend();
    }
}

int producer(){
    int i = 0;
    while( i < 100){
        OS_MailBox_Send(i);
        i++;
        OS_Suspend(); // get rid of if we get preemption working
    }

    OS_Kill();
    // print to screen thread failed to close
    drawString((WIDTH/2)+252, MARGIN-25, "Thread Failed to close!!", 0x0f, 3);

}

int consumer(){
    int i = 0;
    while( i < 100){
        unsigned int input = OS_MailBox_Recv();
        i++;
        drawChar(input % 100 + 0x30, (WIDTH/8)+252, MARGIN+100, 0x0f, 3);
        drawChar(input % 10 + 0x30,  (WIDTH/8)+300, MARGIN+100, 0x0f, 3);

        OS_Suspend(); // get rid of if we get preemption working
    }

    OS_Kill();

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
    drawString((WIDTH/2)-(strwidth/2), (HEIGHT/2)-(strheight/2), "New text!", 0x02, zoom);
    //else drawString((WIDTH/2)-(strwidth/2), (HEIGHT/2)-(strheight/2), "Game over!", 0x04, zoom);

    // add threads
   OS_AddThread(&counter, 1);
    OS_AddThread(&counter7, 1);
    OS_AddThread(&idle, 3);
    //OS_AddProcess(&counter7, 0, 0, 1, 128, 1);


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
