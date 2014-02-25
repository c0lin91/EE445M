// PeriodicSysTickInts.c
// Runs on LM4F120
// Use the SysTick timer to request interrupts at a particular period.
// Daniel Valvano
// October 11, 2012

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers"
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2013
   Volume 1, Program 9.6
   
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013
   Volume 2, Program 5.12, section 5.7

 Copyright 2013 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

// oscilloscope or LED connected to PF2 for period measurement

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "DAC.h"
#include "SysTickInts.h"
#include "PLL.h"
#include "fftw3.h"
#include "lm4f120h5qr.h"

#define GPIO_PORTF_DATA_R       (*((volatile unsigned long *)0x400253FC))
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_AFSEL_R      (*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_AMSEL_R      (*((volatile unsigned long *)0x40025528))
#define GPIO_PORTF_PCTL_R       (*((volatile unsigned long *)0x4002552C))
#define PF2                     (*((volatile unsigned long *)0x40025010))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC2_GPIOF      0x00000020  // port F Clock Gating Control
#define SYSCTL_RCGC2_GPIOE			0x00000010
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
void Timer0A_Init10HzInt(void);
void ADC0_InitAllTriggerSeq3(unsigned char channelNum);

volatile unsigned long Counts = 0;




extern int audioInput;
int outputCounter = 0;

int main(void){
	volatile unsigned long sr;
	//int period = 500;
	//float easyTransform[32];
	
	
	DisableInterrupts(); 
	PLL_Init();                 // bus clock at 50 MHz
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF; // activate port F
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOE;
	sr = 0; 
  //Counts = 0;
	
  GPIO_PORTF_DIR_R |= 0x1F;   // make PF2 output (PF2 built-in LED)
  GPIO_PORTF_AFSEL_R &= ~0x1F;// disable alt funct on PF2
  GPIO_PORTF_DEN_R |= 0x1F;   // enable digital I/O on PF2
                              // configure PF2 as GPIO
	GPIO_PORTE_DIR_R &= 0xEF;
	GPIO_PORTE_DIR_R |= 0x0F;
	GPIO_PORTE_AFSEL_R &= ~0x1F;
	GPIO_PORTE_DEN_R |= 0x1F;
	
//	//ADC Ports
	Timer0A_Init10HzInt();
//	i = 0;
//	


//	
//	
//	
//	GPIO_PORTF_DATA_R = 0x02;
//  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFFF0FF)+0x00000000;
//  GPIO_PORTF_AMSEL_R = 0;     // disable analog functionality on PF
//  
	
//	memset( (float*)easyTransform, 0, sizeof(float) * 32);
//	for(i = 0; i < 32; i += 1){
//		easyTransform[i] = (float) wave[i];
//	}
	
	//TransformVoice(32, easyTransform);

//	audioInput = (float*) malloc (sizeof(float) * 1024);
//	memset( (float*) &audioInput[0], 0, (sizeof(float) * 1024));


	ADC0_InitAllTriggerSeq3(9);
	
		DAC_Init(); 
	SysTick_Init(500);        // initialize SysTick timer

	
	EnableInterrupts();

  while(1){                   // interrupts every 1ms, 500 Hz flash
    WaitForInterrupt();
		
  }
}

// Interrupt service routine
// Executed every 20ns*(period)
/*
void SysTick_Handler(void){
  GPIO_PORTF_DATA_R ^= 0x02;
	//audioInput[i] = 42.0;
	//audioInput[i] = 42.0;
	//i += 1;
	//if(i == 10000){i = 0;}
	//GPIO_PORTF_DATA_R = GPIO_PORTF_DATA_R << 1;
	//if(GPIO_PORTF_DATA_R == 0x10){GPIO_PORTF_DATA_R = 0x02;}
	//PF2 ^= 0x06;                // toggle PF2
  Counts = Counts + 1;
}
*/

//void SysTick_Handler(void){
//  //PF2 ^= 0x04;                // toggle PF2
////  Counts = Counts + 1;
//	
//	
//	GPIO_PORTE_DATA_R &= 0xF0; 
//  GPIO_PORTE_DATA_R |= audioInput; //(long) (audioInput[i]);
//  outputCounter=((outputCounter+1)&0x7f);
//}

//void TransformVoice(int n, float* input){
//	fftwf_plan p;
//	//int n = 1024;
//	int j = 0;
//	int k = 0;
//	int lengthOfChord = 500;

//	p = fftwf_plan_r2r_1d(n, (float*) input, (float*) input,
//                                FFTW_DHT, FFTW_ESTIMATE );	
//	//p = fftwf_plan_r2r_1d(n, (float*) audioInput, (float*) audioInput,
//  //                              FFTW_DHT, FFTW_ESTIMATE );
//	
//	//fftwf_execute(p);
//	
//	for(j = 0; j < 1024; j += 1){
//		//audioInput[j] *= ptrToChord[k];
//		if(k == lengthOfChord){k = 0;}
//	}
//	
//	//p = fftwf_plan_r2r_1d(n, (float*) audioInput, (float*) audioInput,
//  //                              FFTW_DHT, FFTW_ESTIMATE );
//	
//	//fftwf_execute(p);
//}


