#include "PWM.h"

#define SYSCTL_RCGC0_PWM0       0x00100000
#define SYSCTL_RCGC2_GPIOB      0x00000002
#define SYSCTL_RCGC2_GPIOF      0x00000020
#define SYSCTL_RCC_USEPWMDIV    0x00100000
#define SYSCTL_RCC_PWMDIV_M     0x000E0000
#define SYSCTL_RCC_PWMDIV_2     0x00000000
#define SYSCTL_DC1_R            (*((volatile unsigned long *)0x400FE010))
#define SYSCTL_RCGC0_R          (*((volatile unsigned long *)0x400FE100))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCC_R            (*((volatile unsigned long *)0x400FE060))
#define SYSCTL_RCGCPWM_R        (*((volatile unsigned long *)0x400FE640))
#define GPIO_PORTF_AFSEL_R      (*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_AMSEL_R      (*((volatile unsigned long *)0x40025528))
#define GPIO_PORTB_AFSEL_R      (*((volatile unsigned long *)0x40005420))
#define GPIO_PORTB_PCTL_R       (*((volatile unsigned long *)0x4000552C))
#define GPIO_PORTB_DEN_R        (*((volatile unsigned long *)0x4000551C))
#define M0PWM0_LOAD_R           (*((volatile unsigned long *)0x40028050))

#define GPIO_PORTF_PCTL_R       (*((volatile unsigned long *)0x4002552C))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define PF1                     (*((volatile unsigned long *)0x40025008))
// M1PWM registers at 0x4002.9000
#define PWM_ENABLE_PWM7EN       0x00000080
#define PWM_ENABLE_PWM6EN       0x00000040
#define PWM_ENABLE_PWM5EN       0x00000020
#define MXPWMX_CTL_ENABLE       0x00000001
#define MXPWMX_GENX_ACTCMPBD_HI 0x00000C00
#define MXPWMX_GENX_ACTCMPAD_HI 0x000000C0
#define MXPWMX_GENX_ACTLOAD_LO  0x00000008
#define M1PWM_ENABLE_R          (*((volatile unsigned long *)0x40029008))
#define M1PWM2_CTL_R            (*((volatile unsigned long *)0x400290C0))
#define M1PWM3_CTL_R            (*((volatile unsigned long *)0x40029100))
#define M1PWM2_LOAD_R           (*((volatile unsigned long *)0x400290D0))
#define M1PWM3_LOAD_R           (*((volatile unsigned long *)0x40029110))
#define M1PWM2_CMPB_R           (*((volatile unsigned long *)0x400290DC))
#define M1PWM2_GENB_R           (*((volatile unsigned long *)0x400290E4))
#define M1PWM3_CMPA_R           (*((volatile unsigned long *)0x40029118))
#define M1PWM3_GENA_R           (*((volatile unsigned long *)0x40029120))
#define M1PWM3_CMPB_R           (*((volatile unsigned long *)0x4002911C))
#define M1PWM3_GENB_R           (*((volatile unsigned long *)0x40029124))
	
#define M0PWM_ENABLE_R          (*((volatile unsigned long *)0x40028008))
#define M0PWM0_CTL_R            (*((volatile unsigned long *)0x40028040))
#define M0PWM0_LOAD_R           (*((volatile unsigned long *)0x40028050))
#define M0PWM0_CMPA_R           (*((volatile unsigned long *)0x40028058))
#define M0PWM0_GENA_R           (*((volatile unsigned long *)0x40028060))

// period is 16-bit number of PWM cycles in one period (3<=period) 
// duty is number of PWM cycles output is high (2<=duty<=period-1) 
// PWM clock rate = processor clock rate/SYSCTL_RCC_PWMDIV 
// = BusClock/2 (in this example) 

// PWM.c
// Runs on TM4C123
// Use PWM0/PB6 to generate pulse-width modulated outputs.
// Daniel Valvano
// September 3, 2013

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013
  Program 6.7, section 6.3.2

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

#define PWM0_ENABLE_R           (*((volatile unsigned long *)0x40028008))
#define PWM_ENABLE_PWM0EN       0x00000001  // PWM0 Output Enable
#define PWM0_0_CTL_R            (*((volatile unsigned long *)0x40028040))
#define PWM_0_CTL_ENABLE        0x00000001  // PWM Block Enable
#define PWM0_0_LOAD_R           (*((volatile unsigned long *)0x40028050))
#define PWM0_0_CMPA_R           (*((volatile unsigned long *)0x40028058))
#define PWM0_0_GENA_R           (*((volatile unsigned long *)0x40028060))
#define PWM_0_GENA_ACTCMPAD_ONE 0x000000C0  // Set the output signal to 1
#define PWM_0_GENA_ACTLOAD_ZERO 0x00000008  // Set the output signal to 0
#define GPIO_PORTB_AFSEL_R      (*((volatile unsigned long *)0x40005420))
#define GPIO_PORTB_DEN_R        (*((volatile unsigned long *)0x4000551C))
#define GPIO_PORTB_AMSEL_R      (*((volatile unsigned long *)0x40005528))
#define GPIO_PORTB_PCTL_R       (*((volatile unsigned long *)0x4000552C))
#define SYSCTL_RCC_R            (*((volatile unsigned long *)0x400FE060))
#define SYSCTL_RCC_USEPWMDIV    0x00100000  // Enable PWM Clock Divisor
#define SYSCTL_RCC_PWMDIV_M     0x000E0000  // PWM Unit Clock Divisor
#define SYSCTL_RCC_PWMDIV_2     0x00000000  // /2
#define SYSCTL_RCGC0_R          (*((volatile unsigned long *)0x400FE100))
#define SYSCTL_RCGC0_PWM0       0x00100000  // PWM Clock Gating Control
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC2_GPIOB      0x00000002  // Port B Clock Gating Control

// period is 16-bit number of PWM clock cycles in one period (3<=period)
// duty is number of PWM clock cycles output is high  (2<=duty<=period-1)
// PWM clock rate = processor clock rate/SYSCTL_RCC_PWMDIV
//                = BusClock/2 
//                = 50 MHz/2 = 25 MHz (in this example)
void PWM0_Init(unsigned short period, unsigned short duty){
  volatile unsigned long delay;
  SYSCTL_RCGC0_R |= SYSCTL_RCGC0_PWM0;  // 1) activate PWM0
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB; // 2) activate port B
  delay = SYSCTL_RCGC2_R;               // allow time to finish activating
  GPIO_PORTB_AFSEL_R |= 0x40;           // enable alt funct on PB6
  GPIO_PORTB_PCTL_R &= ~0x0F000000;     // configure PB6 as PWM0
  GPIO_PORTB_PCTL_R |= 0x04000000;
  GPIO_PORTB_AMSEL_R &= ~0x40;          // disable analog functionality on PB6
  GPIO_PORTB_DEN_R |= 0x40;             // enable digital I/O on PB6
  SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV; // 3) use PWM divider
  SYSCTL_RCC_R &= ~SYSCTL_RCC_PWMDIV_M; //    clear PWM divider field
  SYSCTL_RCC_R += SYSCTL_RCC_PWMDIV_2;  //    configure for /2 divider
  PWM0_0_CTL_R = 0;                     // 4) re-loading mode
  PWM0_0_GENA_R = (PWM_0_GENA_ACTCMPAD_ONE|PWM_0_GENA_ACTLOAD_ZERO);
  PWM0_0_LOAD_R = period - 1;           // 5) cycles needed to count down to 0
  PWM0_0_CMPA_R = duty - 1;             // 6) count value when output rises
  PWM0_0_CTL_R |= PWM_0_CTL_ENABLE;     // 7) start PWM0
  PWM0_ENABLE_R |= PWM_ENABLE_PWM0EN;   // enable PWM0
}
// change duty cycle
// duty is number of PWM clock cycles output is high  (2<=duty<=period-1)
void PWM0_Duty(unsigned short duty){
  PWM0_0_CMPA_R = duty - 1;             // 6) count value when output rises
}


void PWM_Init(unsigned short period, unsigned short duty){ 
  volatile unsigned long delay; 
  SYSCTL_RCGC0_R |= SYSCTL_RCGC0_PWM0;   // 1)activate PWM clock
	SYSCTL_RCGCPWM_R |= 0x00000002;
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;  // 2)activate port F clocck 
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB;  // 2)activate port F clocck 
  delay = SYSCTL_RCGC2_R;                // allow time to finish activating 
  GPIO_PORTF_DEN_R = 0x02;
	GPIO_PORTF_DIR_R = 0x02;
  GPIO_PORTF_AFSEL_R |= 0x0C;            // enable alt funct on PF1-3
  GPIO_PORTB_AFSEL_R |= 0x40;            // enable alt funct on PF1-3
	
	               // allow time to finish activating
  GPIO_PORTB_AFSEL_R |= 0x40;           // enable alt funct on PB6
  GPIO_PORTB_PCTL_R &= ~0x0F000000;     // configure PB6 as PWM0
  GPIO_PORTB_PCTL_R |= 0x04000000;
  GPIO_PORTB_AMSEL_R &= ~0x40;          // disable analog functionality on PB6
  GPIO_PORTB_DEN_R |= 0x40;             // enable digital I/O on PB6
	//will check if we need this.
	GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R & 0xFFFF0FFF) | 0x00005000;
	GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & 0xF0FFFFFF) | 0x04000000;
	GPIO_PORTB_DEN_R = 0x40;
	
  SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV;  // 3) use PWM divider 
  SYSCTL_RCC_R &= ~SYSCTL_RCC_PWMDIV_M;  // clear PWM divider field 
  SYSCTL_RCC_R += 7;   // configure for /2 divider		
	
	//PF1 = 0x02;
	M0PWM0_CTL_R = 0;
  M1PWM2_CTL_R = 0;                      // 4) re-loading mode 
  M1PWM3_CTL_R = 0;
	M1PWM2_GENB_R = (MXPWMX_GENX_ACTCMPBD_HI|MXPWMX_GENX_ACTLOAD_LO); 
	M1PWM3_GENA_R = (0x0000008C);
	M0PWM0_GENA_R = (0x0000008C);
	M1PWM3_GENB_R = (0x0000080C);
  M1PWM2_LOAD_R = 0x0000018F;            // 5) cycles needed to count down to 0
	M1PWM3_LOAD_R = 0x0000018F;
  M1PWM2_CMPB_R = 0x00000063;              // 6) count value when output rises 
	M1PWM3_CMPA_R = 0x00000063;
	M1PWM3_CMPB_R = 0x00000063;
	
	M0PWM0_LOAD_R = 40000;
  M0PWM0_CMPA_R = 39999;  
	/*
	M1PWM2_CTL_R |= MXPWMX_CTL_ENABLE;      // 7) start PWM
  M1PWM3_CTL_R |= MXPWMX_CTL_ENABLE;
  M1PWM_ENABLE_R |= PWM_ENABLE_PWM7EN |
										PWM_ENABLE_PWM6EN |
										PWM_ENABLE_PWM5EN;     // enable M1PWM5-7
	*/
	M1PWM3_CTL_R = 1;
	M0PWM0_CTL_R = 1;
	M0PWM_ENABLE_R = 0x00000001;
	M1PWM_ENABLE_R = 0x00000004;
	
	//PF1 = 0x02;
	
	
  PWM0_0_CTL_R = 0;                     // 4) re-loading mode
  PWM0_0_GENA_R = (PWM_0_GENA_ACTCMPAD_ONE|PWM_0_GENA_ACTLOAD_ZERO);
  PWM0_0_LOAD_R = 10000;           // 5) cycles needed to count down to 0
  PWM0_0_CMPA_R = 5000;             // 6) count value when output rises
  PWM0_0_CTL_R |= PWM_0_CTL_ENABLE;     // 7) start PWM0
  PWM0_ENABLE_R |= PWM_ENABLE_PWM0EN;   // enable PWM0
								
}
/*
void PWM0_Duty(unsigned short duty){ 
 PWM_0_CMPA_R = duty - 1; // 6) count value when output rises 
} 
*/
/*	Enables the PWM MmPWMn on the microcontroller.
	The PWM will be enabled with a Duty of 0%

void PWM_Enable(int m, int n);*/
#define PWM1_ENABLE_R           (*((volatile unsigned long *)0x40029008))
#define PWM_ENABLE_PWM0EN       0x00000001  // PWM0 Output Enable
#define PWM_ENABLE_PWM1EN       0x00000002  // PWM1 Output Enable
#define PWM_ENABLE_PWM2EN       0x00000004  // PWM2 Output Enable
#define PWM_ENABLE_PWM3EN       0x00000008  // PWM2 Output Enable
#define PWM1_0_CTL_R            (*((volatile unsigned long *)0x40029040))
#define PWM1_1_CTL_R            (*((volatile unsigned long *)0x40029040))
#define PWM_1_CTL_ENABLE        0x00000001  // PWM Block Enable
#define PWM1_0_LOAD_R           (*((volatile unsigned long *)0x40029050))
#define PWM1_0_CMPA_R           (*((volatile unsigned long *)0x40029058))
#define PWM1_0_GENA_R           (*((volatile unsigned long *)0x40029060))
#define PWM_0_GENA_ACTCMPAD_ONE 0x000000C0  // Set the output signal to 1
#define PWM_0_GENA_ACTLOAD_ZERO 0x00000008  // Set the output signal to 0
void PWM_Test(void){
  volatile unsigned long delay;
  SYSCTL_RCGC0_R |= SYSCTL_RCGC0_PWM0;  // 1) activate PWM0
	SYSCTL_RCGCPWM_R |= 2;
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF; // 2) activate port B
  delay = SYSCTL_RCGC2_R;               // allow time to finish activating
  GPIO_PORTF_AFSEL_R |= 0xFF;           // enable alt funct on PB6
  GPIO_PORTF_PCTL_R &= ~0xFFFFFFFF;     // configure PB6 as PWM0
  GPIO_PORTF_PCTL_R |= 0x55555555;
  GPIO_PORTF_AMSEL_R &= ~0xFF;          // disable analog functionality on PB6
  GPIO_PORTF_DEN_R |= 0xFF;             // enable digital I/O on PB6
  SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV; // 3) use PWM divider
  SYSCTL_RCC_R &= ~SYSCTL_RCC_PWMDIV_M; //    clear PWM divider field
  SYSCTL_RCC_R += SYSCTL_RCC_PWMDIV_2;  //    configure for /2 divider
  M1PWM2_CTL_R = 0;                     // 4) re-loading mode
  M1PWM2_GENB_R = (PWM_0_GENA_ACTCMPAD_ONE|PWM_0_GENA_ACTLOAD_ZERO);
  M1PWM2_LOAD_R = 10;           // 5) cycles needed to count down to 0
  M1PWM2_CMPB_R = 1;             // 6) count value when output rises
  M1PWM2_CTL_R |= 1;     // 7) start PWM0
  PWM1_ENABLE_R |= PWM_ENABLE_PWM0EN;   // enable PWM0
}