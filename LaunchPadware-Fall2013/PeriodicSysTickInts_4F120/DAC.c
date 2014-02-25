#define GPIO_PORTE_DATA_R       (*((volatile unsigned long *)0x400243FC))
#define GPIO_PORTE_DIR_R        (*((volatile unsigned long *)0x40024400))
#define GPIO_PORTE_AFSEL_R      (*((volatile unsigned long *)0x40024420))
#define GPIO_PORTE_DEN_R        (*((volatile unsigned long *)0x4002451C))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC2_GPIOF      0x00000020  // port F Clock Gating Control
#define SYSCTL_RCGC2_GPIOE      0x00000010  // port F Clock Gating Control
#define NVIC_SYS_PRI3_R         (*((volatile unsigned long *)0xE000ED20))  // Sys. Handlers 12 to 15 Priority
#define NVIC_ST_CTRL_R          (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R        (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile unsigned long *)0xE000E018))
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
#define GPIO_PORTF_DATA_R       (*((volatile unsigned long *)0x400253FC))


void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode



int i; 
 const unsigned char wave[32] = {	// initialize the data structure		
   //8,9,11,12,13,14,14,15,15,15,14,14,13,12,11,9,8,7,5,4,3,2,2,1,1,1,2,2,3,4,5,7}; // Sin
	 //6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8 , 8, 8, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10}; // Harmonica
	 //8,10,11,12,13,14,14,14,13,12,11,10,9,8,7,7,6,7,6,6,6,5,5,4,4,4,4,4,5,5,6,7};  // Flute
	 //8,9,9,9,8,6,2,1,6,12,12,9,8,8,8,8,8,8,8,8,8,8,8,9,8,8,8,9,9,8,8,8};   // Trumpet

	 
void DAC_Init(void){
    volatile unsigned long delay; 
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOE;
	delay = 0; 
	GPIO_PORTE_DIR_R |= 0x0F;
	GPIO_PORTE_AFSEL_R &= ~0x0F;
	GPIO_PORTE_DEN_R |= 0x0F;
	
	GPIO_PORTE_DATA_R = 0x0F; 
} 

void SysTick_Init(unsigned long period){long sr;
  sr = StartCritical();
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_RELOAD_R = period-1;// reload value
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000; // priority 2
                              // enable SysTick with core clock and interrupts
  NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC+NVIC_ST_CTRL_INTEN;
  EndCritical(sr);
}

void SysTick_Reload(unsigned long period){
	 NVIC_ST_RELOAD_R = period-1;// reload value
} 

void SysTick_Handler(void){
   GPIO_PORTF_DATA_R ^= 0x04;                // toggle PF2

	GPIO_PORTE_DATA_R &= 0xF0; 
  GPIO_PORTE_DATA_R |= (wave[i]);
  i += 1;
	if (i == 32){i = 0;}
	
	//i=((i+1)&0x7f);   // 0b0111 1111
}
