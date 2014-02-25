#include "lm4f120h5qr.h"
#include "ADCSWTrigger.h"

unsigned long ADC0_InSeq3(void);
void DisableInterrupts(void);

#define GPIO_PORTF2             (*((volatile unsigned long *)0x40025010))
#include "DAC.h"
int audioInput;

void Timer0A_Init10HzInt(void){
  volatile unsigned long delay;
  DisableInterrupts();
  // **** general initialization ****
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER0;// activate timer0
  delay = SYSCTL_RCGC1_R;          // allow time to finish activating
  TIMER0_CTL_R &= ~TIMER_CTL_TAEN; // disable timer0A during setup
  TIMER0_CFG_R = TIMER_CFG_16_BIT; // configure for 16-bit timer mode
  // **** timer0A initialization ****
                                   // configure for periodic mode
  TIMER0_TAMR_R = TIMER_TAMR_TAMR_PERIOD;
  TIMER0_TAPR_R = 49;             // prescale value for 1us
  TIMER0_TAILR_R = 5;           // start value for 8000 Hz interrupts
  TIMER0_IMR_R |= TIMER_IMR_TATOIM;// enable timeout (rollover) interrupt
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;// clear timer0A timeout flag
  TIMER0_CTL_R |= TIMER_CTL_TAEN;  // enable timer0A 16-b, periodic, interrupts
  // **** interrupt initialization ****
                                   // Timer0A=priority 2
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x40000000; // top 3 bits
  NVIC_EN0_R = NVIC_EN0_INT19;     // enable interrupt 19 in NVIC
}
void Timer0A_Handler(void){
	int interval = 512;
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;    // acknowledge timer0A timeout
  //GPIO_PORTF2 = 0x04;                   // profile
  audioInput = (int) ADC0_InSeq3();
//	if(audioInput[i] < 2000 && i < 50){
//    GPIO_PORTF_DATA_R ^= 0x02;
//}
//	i += 1;
//	if(i == 1024){i = 0;}
//  GPIO_PORTF2 = 0x00;
	
	
	if (audioInput >= 0 && audioInput <= interval) 
	{
		SysTick_Reload (1908); 
	} 
	
	else if (audioInput > interval && audioInput <= interval*2)
	{
			SysTick_Reload (1700); 
	} 
	else if (audioInput > interval*2 && audioInput <= interval*3)
	{
			SysTick_Reload (1515); 
	} 
	else if (audioInput > interval*3 && audioInput <= interval*4)
	{
			SysTick_Reload (1432); 
	} 
	else if (audioInput > interval*4 && audioInput <= interval*5)
	{
			SysTick_Reload (1275); 
	} 
	else if (audioInput > interval*5 && audioInput <= interval*6)
	{
			SysTick_Reload (1136);
	} 
		else if (audioInput > interval*6 && audioInput <= interval*7)
	{
			SysTick_Reload (1012);
	} 
	else 
	{
			SysTick_Reload (956); 
	} 
}

