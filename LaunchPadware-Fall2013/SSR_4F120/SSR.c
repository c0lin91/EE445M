// SSR.c
// Runs on LM4F120
// Provide functions that initialize a GPIO pin and turn it on and off.
// Use bit-banded I/O.
// Daniel Valvano
// May 18, 2013

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers"
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2013
   Volume 1 Program 4.3, Figure 4.14

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

// solid state relay connected to PF2

#define GPIO_PORTF_DATA_R       (*((volatile unsigned long *)0x400253FC))
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_AFSEL_R      (*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_AMSEL_R      (*((volatile unsigned long *)0x40025528))
#define GPIO_PORTF_PCTL_R       (*((volatile unsigned long *)0x4002552C))
#define PF2                     (*((volatile unsigned long *)0x40025010))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC2_GPIOF      0x00000020  // port F Clock Gating Control

// Make PF2 an output, enable digital I/O, ensure alt. functions off
SSR_Init(void){ volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x20;           // 1) activate clock for Port F
  delay = SYSCTL_RCGC2_R;           // allow time for clock to start
                                    // 2) no need to unlock PF2
  GPIO_PORTF_PCTL_R &= ~0x00000F00; // 3) regular GPIO
  GPIO_PORTF_AMSEL_R &= ~0x04;      // 4) disable analog function on PF2
  GPIO_PORTF_DIR_R |= 0x04;         // 5) set direction to output
  GPIO_PORTF_AFSEL_R &= ~0x04;      // 6) regular port function
  GPIO_PORTF_DEN_R |= 0x04;         // 7) enable digital port
}

// Make PF2 high
void SSR_On(void){
  PF2 = 0x04;
//  GPIO_PORTF_DATA_R |= 0x04;
}
// Make PF2 low
void SSR_Off(void){
  PF2 = 0x00;
//  GPIO_PORTF_DATA_R &= ~0x04;
}

//debug code
// This is an extremely simple test program to demonstrate that the SSR
// can turn on and off.  Button de-bouncing is not addressed.
// built-in negative logic switch connected to PF4
#define GPIO_PORTF_PUR_R        (*((volatile unsigned long *)0x40025510))
int main(void){
  SSR_Init();               // initialize PF2 and make it output
  GPIO_PORTF_DIR_R &= ~0x10;// make PF4 in (PF4 built-in button #1)
  GPIO_PORTF_AFSEL_R &= ~0x10;// disable alt funct on PF4
  GPIO_PORTF_PUR_R |= 0x10; // enable pull-up on PF4
  GPIO_PORTF_DEN_R |= 0x10; // enable digital I/O on PF4
                            // configure PF4 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFF0FFFF)+0x00000000;
  while(1){
    SSR_On();
    while(GPIO_PORTF_DATA_R&0x10);        // wait for button press
    while((GPIO_PORTF_DATA_R&0x10) == 0); // wait for button release
    SSR_Off();
    while(GPIO_PORTF_DATA_R&0x10);        // wait for button press
    while((GPIO_PORTF_DATA_R&0x10) == 0); // wait for button release
  }
}
