#include "lm4f120h5qr.h"
#include "SSD2119.h"
#include "PLL.h"
#include "systick.h"
#include "adc.h"
#include "random.h"
#include "FPU.h"
#include "LCDTests.h"
#include "images.h"

int tickCounter = 0;
int n = 0;

int main(void){unsigned long i=0;
    // Set the clocking to run at 80MHz from the PLL.
    PLL_Init();

    // Initialize LCD
    LCD_Init();
    
    // Initialize RNG
    Random_Init(121213);

    // Initialize touchscreen GPIO
    Touch_Init();
    
    // Initialize ADC for use with touchscreen
    ADC_Init();    
    
    // Intiialize SysTick
    SysTick_Init();
    
    // Set SysTick interrupts to happen every millisecond
    SysTick_SetPeriod(80000);
    SysTick_EnableInterrupts();
    
    //Touch_BeginWaitForTouch();
    
    for(;;) {
        
        // TO PLAY WITH THESE, EMPTY THE SYSTICK OF TIMED
        // STUFF AND UNCOMMENT ONE TEST AT A TIME
        // - SOME MAY BE BROKEN NOW
        
        //This file was modified to make a short animated
        //demonstration of the LCD driver.  Each test is
        //shown at least briefly, then the best footage
        //will be selected in post-production.
        //NOTE: To return this file to its previous
        //condition, un-comment the following lines in the
        //SysTick_Handler:
        //touchDebug();
        //LCD_DrawBMP(testbmp, 64,64);
        //Beginning of animated demonstration
        for(i=0; i<10000; i=i+1){
          RandomRectangles();
        }
        LCD_ColorFill(convertColor(0, 0, 255));
        for(i=0; i<1250000; i=i+1){
          MovingColorBars();
        }
        for(i=0; i<15; i=i+1){
          LineSpin();
        }
        LCD_ColorFill(convertColor(0, 0, 0));
        for(i=0; i<2000; i=i+1){
          printfTest();
        }
        for(i=0; i<2500; i=i+1){
          RandomCircle();
        }
        for(i=0; i<6500; i=i+1){
          Random4BPPTestSprite();
        }
        //End of animated demonstration
        //PrintAsciiChars();
        //BlastChars();
        //TestStringOutput();
        //RandomRectangles();
        //MovingColorBars();
        //RandomColorFill();
        //BWWaitFill();
        //LineSpin();
        //printfTest();
        //charTest();
        //RandomCircle();
        //Random4BPPTestSprite();

    }
}

// ************** SysTick_Handler  ************************
// - Default SysTick interrupt handler
// ********************************************************
// Input: none
// Output: none
// ********************************************************
void SysTick_Handler(void){
    // ticks every millisecond
    tickCounter++;

    // TIMED STUFF HERE!
    if (tickCounter >= 20) { 
        tickCounter = 0;

        // NOTE: i realize the systick handler is not a great place
        // for the next couple functions.  they're just hanging out
        // here right now.
        
        // retreive/print touch debug info
//        touchDebug();
        
        // draw a test image 
//         LCD_DrawBMP(testbmp, 64,64);
    }
}

// unused at the moment, previously used to implement touch sensing on edge (not yet working)
void GPIOPortA_Handler(void){
  GPIO_PORTA_ICR_R = 0x08;      // acknowledge flag4
  Touched = 1;
}
