// Squarewaves.c
// Runs on LM4F120
// Initialize PF1 and PF2 as outputs with different initial values,
// then toggle them to produce two out of phase square waves.  
// Daniel Valvano
// April 17, 2013
/*
 This example accompanies the book
  "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
  ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2013
  Program 4.4

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



#define GPIO_PORTF_DATA_R       (*((volatile unsigned long *)0x400253FC))
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_AFSEL_R      (*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_PUR_R        (*((volatile unsigned long *)0x40025510))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_LOCK_R       (*((volatile unsigned long *)0x40025520))
#define GPIO_PORTF_CR_R         (*((volatile unsigned long *)0x40025524))
#define GPIO_PORTF_AMSEL_R      (*((volatile unsigned long *)0x40025528))
#define GPIO_PORTF_PCTL_R       (*((volatile unsigned long *)0x4002552C))
#define GPIO_LOCK_KEY           0x4C4F434B  // Unlocks the GPIO_CR register
#define PF0       (*((volatile unsigned long *)0x40025004))
#define PF4       (*((volatile unsigned long *)0x40025040))
#define SWITCHES  (*((volatile unsigned long *)0x40025044))
#define SW1       0x10                      // on the left side of the Launchpad board
#define SW2       0x01                      // on the right side of the Launchpad board
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC2_GPIOF      0x00000020  // port F Clock Gating Control
#define RED       0x02
#define BLUE      0x04
#define GREEN     0x08
#define PF21 (*((volatile unsigned long *)0x40025018))
int main(void){
volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x20;          // 1) Port G clock
  delay = SYSCTL_RCGC2_R;          // 2) no need to unlock 
  GPIO_PORTF_AMSEL_R &= ~0x06;     // 3) disable analog
  GPIO_PORTF_PCTL_R &= ~0x00000FF0;// 4) configure as GPIO
  GPIO_PORTF_DIR_R |= 0x06;        // 5) PF2 PF1 outputs
  GPIO_PORTF_AFSEL_R &= ~0x06;     // 6) normal function
  GPIO_PORTF_DEN_R |= 0x06;        // 7) digital I/O on PF2-1
  PF21 = 0x02;                     // PF2=0; PF1=1
  while(1){
    PF21 ^= 0x06;                  // toggle
  }
}

// Color    LED(s) PortF
// dark     ---    0
// red      R--    0x02
// blue     --B    0x04
// green    -G-    0x08
// yellow   RG-    0x0A
// sky blue -GB    0x0C
// white    RGB    0x0E
// pink     R-B    0x06
