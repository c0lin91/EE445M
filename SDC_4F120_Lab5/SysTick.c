// SysTick.h
// Runs on LM4F120
// Provide functions that initialize the SysTick module, wait at least a
// designated number of clock cycles, and wait approximately a multiple
// of 10 milliseconds using busy wait.  After a power-on-reset, the
// LM4F120 gets its clock from the 16 MHz precision internal oscillator,
// which can vary by +/- 1% at room temperature and +/- 3% across all
// temperature ranges.  If you are using this module, you may need more
// precise timing, so it is assumed that you are using the PLL to set
// the system clock to 50 MHz.  This matters for the function
// SysTick_Wait10ms(), which will wait longer than 10 ms if the clock is
// slower.
// Daniel Valvano
// October 25, 2012

/* This example accompanies the books
  "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
  ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2013
  Volume 1, Program 4.7
   
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013
   Program 2.11, Section 2.6

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

#define NVIC_ST_CTRL_R          (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R        (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile unsigned long *)0xE000E018))
#define NVIC_INT_CTRL_R					(*((volatile unsigned long *)0xE000ED04))
#define NVIC_ST_CTRL_COUNT      0x00010000  // Count flag
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
#define NVIC_ST_RELOAD_M        0x00FFFFFF  // Counter load value
#define NVIC_INT_CTRL_PEND_SV   0x10000000  // PendSV Set Pending
#define NVIC_SYS_PRI3_R					(*((volatile unsigned long *)0xE000ED20))
#define PE1  (*((volatile unsigned long *)0x40024008))
#define PE3  (*((volatile unsigned long *)0x40024020))
#include "OS.h"
#define PF2  (*((volatile unsigned long *)0x40025010))
extern tcbType *tempRunPt; 
extern tcbType *RunPt; 
extern tcbType *StartPt;
extern tcbType *SleepPt;
extern int NumSleeping; 

void WakeUp(void); 


// Initialize SysTick with busy wait running at bus clock.
void SysTick_Init(){
  NVIC_ST_CTRL_R = 0;                   // disable SysTick during setup
  NVIC_ST_RELOAD_R = NVIC_ST_RELOAD_M;  // maximum reload value
  NVIC_ST_CURRENT_R = 0;                // any write to current clears it
                                        // enable SysTick with core clock
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R & 0x00FFFFFF) | 0xE0000000;	// Lowest priority (priority 7)
	
  NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC+NVIC_ST_CTRL_INTEN;
	
}
// Time delay using busy wait.
// The delay parameter is in units of the core clock. (units of 20 nsec for 50 MHz clock)
void SysTick_Wait(unsigned long delay){
  volatile unsigned long elapsedTime;
  unsigned long startTime = NVIC_ST_CURRENT_R;
  do{
    elapsedTime = (startTime-NVIC_ST_CURRENT_R)&0x00FFFFFF;
  }
  while(elapsedTime <= delay);
}
// Time delay using busy wait.
// This assumes 50 MHz system clock.
void SysTick_Wait10ms(unsigned long delay){
  unsigned long i;
  for(i=0; i<delay; i++){
    SysTick_Wait(500000);  // wait 10ms (assumes 50 MHz clock)
  }
}
// Time delay using busy wait.
// This assumes 50 MHz system clock.
void SysTick_Wait1ms(unsigned long delay){
  unsigned long i;
  for(i=0; i<delay; i++){
    SysTick_Wait(50000);   // wait 1ms (assumes 50 MHz clock)
  }
}


////Interrupt Handler
void SysTick_Handler (void) {
	//Do MAGIC
	int status;
	PF2 ^= 0x04;  
	status =StartCritical();
	WakeUp(); 
	if((RunPt->nextThread->priority == StartPt->priority)
		  && (RunPt->nextThread->blockState != 1)){
		tempRunPt = RunPt->nextThread; 
	}else{
  	tempRunPt = StartPt;
	 	while(tempRunPt->blockState ==1){
	  	tempRunPt = tempRunPt->nextThread;
	  }
	}
	NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Trigger PendSV
	EndCritical(status);
	PF2 ^= 0x04; 
	
} 

void WakeUp(void){
	int idx, status; tcbType *tempSleep; tcbType *temp; 
	int numWakeUp = 0;
	status = StartCritical();
	for(idx = 0; idx < NumSleeping; idx++){
		SleepPt->sleepState -= 1; 
		if (!(SleepPt->sleepState)) {
			//wake up this thread
			numWakeUp++;
			NumSleeping--; 
			
			// First fix the sleep linked list
			tempSleep = SleepPt->nextThread;
			SleepPt->prevThread->nextThread = SleepPt->nextThread;
			SleepPt->nextThread->prevThread = SleepPt->prevThread;
//			SleepPt->prevThread = RunPt; 
//			SleepPt->nextThread = RunPt->nextThread; 
//			RunPt->nextThread->prevThread = SleepPt; 
//			RunPt->nextThread = SleepPt;
			// Restore the active linked list
			if(StartPt){
		//highest priority, add to front
				if(StartPt->priority > SleepPt->priority){
					SleepPt->nextThread = StartPt;
					SleepPt->prevThread = StartPt->prevThread;
					StartPt->prevThread->nextThread = SleepPt;
					StartPt->prevThread = SleepPt;
					StartPt = SleepPt;
				}else{
					temp = StartPt;
					while(temp->nextThread != StartPt){		// search for spot in middle
						if((temp->priority             <= SleepPt->priority) &&
							 (temp->nextThread->priority > SleepPt->priority)){
							break;
						}
						temp = temp->nextThread;
					}
					SleepPt->nextThread = temp->nextThread;
					SleepPt->prevThread = temp;
					temp->nextThread->prevThread = SleepPt;
					temp->nextThread = SleepPt;
				}
			}else{	// no other link in the list
			StartPt = SleepPt;
			SleepPt->nextThread = SleepPt;
			SleepPt->prevThread = SleepPt;
			}	
			
			//Round Robin Code (this was never 100% correct O_O)
//			tempSleep = SleepPt->nextThread;
//			SleepPt->prevThread = RunPt; 
//			SleepPt->nextThread = RunPt->nextThread; 
//			RunPt->nextThread->prevThread = SleepPt; 
//			RunPt->nextThread = SleepPt;
			
			SleepPt = (NumSleeping) ? tempSleep : 0; 
		}
	}
	EndCritical(status);
	//NumSleeping -= numWakeUp;
}

