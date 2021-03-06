//OS_PING.c

#include "OS.h"
//#include "inc/tm4c123gh6pm.h"
//#include "tm4cAddresses.h"



//PING.c


/*
PING CONNECTIONS:
GND ----- GND
5V ------ VBUS
SIG ----- PB6
*/
// external signal connected to PB6 (T0CCP0) (trigger on rising edge)

#define NVIC_EN0_INT19          0x00080000  // Interrupt 19 enable
#define NVIC_EN0_R              (*((volatile unsigned long *)0xE000E100))  // IRQ 0 to 31 Set Enable Register
#define NVIC_PRI4_R             (*((volatile unsigned long *)0xE000E410))  // IRQ 16 to 19 Priority Register
#define GPIO_PORTB_DIR_R        (*((volatile unsigned long *)0x40005400))
#define GPIO_PORTB_DATA_R       (*((volatile unsigned long *)0x400053FC))
#define GPIO_PORTB_AFSEL_R      (*((volatile unsigned long *)0x40005420))
#define GPIO_PORTB_DEN_R        (*((volatile unsigned long *)0x4000551C))
#define GPIO_PORTB_AMSEL_R      (*((volatile unsigned long *)0x40005528))
#define GPIO_PORTB_PCTL_R       (*((volatile unsigned long *)0x4000552C))
#define TIMER0_CFG_R            (*((volatile unsigned long *)0x40030000))
#define TIMER0_TAMR_R           (*((volatile unsigned long *)0x40030004))
#define TIMER0_CTL_R            (*((volatile unsigned long *)0x4003000C))
#define TIMER0_IMR_R            (*((volatile unsigned long *)0x40030018))
#define TIMER0_ICR_R            (*((volatile unsigned long *)0x40030024))
#define TIMER0_TAILR_R          (*((volatile unsigned long *)0x40030028))
#define TIMER0_TAPR_R           (*((volatile unsigned long *)0x40030038))
#define TIMER0_TAR_R            (*((volatile unsigned long *)0x40030048))
#define TIMER_CFG_16_BIT        0x00000004  // 16-bit timer configuration,
                                            // function is controlled by bits
                                            // 1:0 of GPTMTAMR and GPTMTBMR
#define TIMER_CFG_32_BIT_TIMER  0x00000000  // 32-bit timer configuration
#define TIMER_TAMR_TACMR        0x00000004  // GPTM TimerA Capture Mode
#define TIMER_TAMR_TAMR_CAP     0x00000003  // Capture mode
#define TIMER_CTL_TAEN          0x00000001  // GPTM TimerA Enable
#define TIMER_CTL_TAEVENT_POS   0x00000000  // Positive edge
#define TIMER_CTL_TAEVENT_BOTH  0x0000000C  // Both edges
#define TIMER_IMR_CAEIM         0x00000004  // GPTM CaptureA Event Interrupt
                                            // Mask
#define TIMER_ICR_CAECINT       0x00000004  // GPTM CaptureA Event Interrupt
                                            // Clear
#define TIMER_TAILR_M           0xFFFFFFFF  // GPTM Timer A Interval Load
                                            // Register
#define SYSCTL_RCGC1_R          (*((volatile unsigned long *)0x400FE104))
#define SYSCTL_RCGC1_TIMER0     0x00010000  // timer 0 Clock Gating Control
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC2_GPIOB      0x00000002  // port B Clock Gating Control



#define TIMER2_TAV_R            (*((volatile unsigned long *)0x40032050))





#define GPIO_PORTB_IS_R         (*((volatile unsigned long *)0x40005404))
#define GPIO_PORTB_IBE_R        (*((volatile unsigned long *)0x40005408))
#define GPIO_PORTB_IEV_R        (*((volatile unsigned long *)0x4000540C))
#define GPIO_PORTB_IM_R         (*((volatile unsigned long *)0x40005410))
#define GPIO_PORTB_RIS_R        (*((volatile unsigned long *)0x40005414))
#define GPIO_PORTB_MIS_R        (*((volatile unsigned long *)0x40005418))
#define GPIO_PORTB_ICR_R        (*((volatile unsigned long *)0x4000541C))
#define GPIO_PORTB_AFSEL_R      (*((volatile unsigned long *)0x40005420))

#define NVIC_EN0_INT17          0x00020000  // Interrupt 17 enable
#define NVIC_EN0_INT1           0x00000002  // Interrupt 1 enable

#define NVIC_PRI0_R             (*((volatile unsigned long *)0xE000E400))
#define GPIO_PORTB_PDR_R        (*((volatile unsigned long *)0x40005514))
	#define GPIO_PORTB_PUR_R        (*((volatile unsigned long *)0x40005510))
void (*PING_PeriodicTask)(void);

//------------TimerCapture_Init------------
// Initialize Timer0A in edge time mode to request interrupts on
// the rising edge of PB0 (CCP0).  The interrupt service routine
// acknowledges the interrupt and calls a user function.
// Input: task is a pointer to a user function
// Output: none
void TimerCapture_Init(void(*task)(void)){long sr;
/* 
	sr = StartCritical();
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER0;// activate timer0
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB; // activate port B
  PING_PeriodicTask = task;             // user function 
  GPIO_PORTB_DIR_R &= ~0x40;       // make PB6 in
  GPIO_PORTB_AFSEL_R |= 0x40;      // enable alt funct on PB6
  GPIO_PORTB_DEN_R |= 0x40;        // enable digital I/O on PB6
                                   // configure PB6 as T0CCP0
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xF0FFFFFF)+0x07000000;
  GPIO_PORTB_AMSEL_R &= ~0x40;     // disable analog functionality on PB6
  TIMER0_CTL_R &= ~TIMER_CTL_TAEN; // disable timer0A during setup
  TIMER0_CFG_R = TIMER_CFG_16_BIT; // configure for 16-bit timer mode
	//TIMER0_CFG_R = TIMER_CFG_32_BIT_TIMER; // configure for 32-bit timer mode
                                   // configure for capture mode, default down-count settings
  TIMER0_TAMR_R = (TIMER_TAMR_TACMR|TIMER_TAMR_TAMR_CAP);
                                   // configure for rising edge event
  //TIMER0_CTL_R &= ~(TIMER_CTL_TAEVENT_POS|0xC);
	TIMER0_CTL_R &= ~(TIMER_CTL_TAEVENT_BOTH|0xC);
  TIMER0_TAILR_R = TIMER_TAILR_M;  // max start value
  TIMER0_IMR_R |= TIMER_IMR_CAEIM; // enable capture match interrupt
  TIMER0_ICR_R = TIMER_ICR_CAECINT;// clear timer0A capture match flag
  TIMER0_CTL_R |= TIMER_CTL_TAEN;  // enable timer0A 16-b, +edge timing, interrupts
                                   // Timer0A=priority 2
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x40000000; // top 3 bits
  NVIC_EN0_R = NVIC_EN0_INT19;     // enable interrupt 19 in NVIC
  EndCritical(sr);
*/


sr = StartCritical();
SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB; //activate port B
PING_PeriodicTask = task;             //user function 
GPIO_PORTB_DIR_R |= 0x40;    					//PB6 out
GPIO_PORTB_AFSEL_R &= ~0x40;  				//disable alt funct on PB64
GPIO_PORTB_DEN_R |= 0x40;     				//enable digital I/O on PB6
																			//configure PF4 as GPIO
GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xF0FFFFFF)+0x00000000;
GPIO_PORTB_AMSEL_R = 0;       				//disable analog functionality on PB
//	GPIO_PORTB_PUR_R |= 0x40;    		//     enable weak pull-up on PB6
GPIO_PORTB_PDR_R |= 0x40;    		//     enable weak pull-down on PB6
GPIO_PORTB_IS_R &= ~0x40;     			// (d) PB64 is edge-sensitive
	//GPIO_PORTB_IBE_R &= ~0x40;    			//     PB6 is not both edges
GPIO_PORTB_IBE_R |= 0x40;    			//     PB6 is both edges
	//GPIO_PORTB_IEV_R &= ~0x40;    			//     PB6 falling edge event
GPIO_PORTB_ICR_R = 0x40;      			// (e) clear flag6
GPIO_PORTB_IM_R |= 0x40;      // (f) arm interrupt on PB6

//NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x40000000; // top 3 bits
NVIC_PRI0_R = (NVIC_PRI0_R&0xFFFF0FFF)|0x00004000; // top 3 bits
//NVIC_EN0_R = NVIC_EN0_INT19;     // enable interrupt 19 in NVIC
NVIC_EN0_R = NVIC_EN0_INT1;     // enable interrupt 1 in NVIC
EndCritical(sr);
/*



Edge interrupt prototype

void EdgeCounter_Init(void){
  DisableInterrupts();
                                // (a) activate clock for port F
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;
  FallingEdges = 0;             // (b) initialize counter
  GPIO_PORTF_DIR_R &= ~0x10;    // (c) make PF4 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x10;  //     disable alt funct on PF4
  GPIO_PORTF_DEN_R |= 0x10;     //     enable digital I/O on PF4
                                //     configure PF4 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFF0FFFF)+0x00000000;
  GPIO_PORTF_AMSEL_R = 0;       //     disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x10;     //     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x10;     // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x10;    //     PF4 is not both edges
  GPIO_PORTF_IEV_R &= ~0x10;    //     PF4 falling edge event
  GPIO_PORTF_ICR_R = 0x10;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x10;      // (f) arm interrupt on PF4
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // (g) priority 5
  NVIC_EN0_R = NVIC_EN0_INT30;  // (h) enable interrupt 30 in NVIC
  EnableInterrupts();           // (i) Program 5.3
}
void GPIOPortF_Handler(void){
  GPIO_PORTF_ICR_R = 0x10;      // acknowledge flag4
  FallingEdges = FallingEdges + 1;

*/

}



unsigned long PING_Time;

//---------------------------- PING_Init -----------------------------
//called before OS_Launch
void PING_Init(void (*task)(void)){
	
	TimerCapture_Init(task);
	//setup edge interrupt (with interrupts not enabled for PB6; )

}

void PING_Signal(void){
		unsigned long sr, startTime, finishTime;
		int i;
		sr = StartCritical();	//diable interrupts
   
		GPIO_PORTB_IM_R &= ~0x40;
		GPIO_PORTB_DIR_R |= 0x40; //make PB6 out
		GPIO_PORTB_DATA_R |= 0x40; //set PB6 high
	 
			for(i = 0; i < 100; i++){}
		//startTime = OS_Time();
		//while(OS_TimeDifference(startTime, OS_Time()) < 400){};  //wait for 5us
	
		GPIO_PORTB_DATA_R &= ~0x40; //set PB6 low
		GPIO_PORTB_DIR_R &= ~0x40; //make PB6 in
		GPIO_PORTB_DATA_R &= ~0x40; //set PB6 low
		GPIO_PORTB_ICR_R = 0x40;      // acknowledge flag6
		GPIO_PORTB_IM_R |= 0x40;
		
		
		EndCritical(sr);	//enable interrupts
		OS_Kill();
}



//----------------- PING_Start -----------------------
//this is called from background from PeriodicThreads
void PING_Start (void){
		OS_AddThread(&PING_Signal,128,1);
}
/*
// timer0A interrupt handler
void Timer0A_Handler(void){
	static unsigned long startTime, diff;
	TIMER0_ICR_R = TIMER_ICR_CAECINT;// acknowledge timer0A capture match

    if((GPIO_PORTB_DATA_R & 0x40) == 0x40){ //PB6 high
        startTime = OS_Time();
    }
    else if((GPIO_PORTB_DATA_R & 0x40) == 0){ //PB6 low
        //disarm the interrupt for PB6?
			diff = OS_Time() - startTime;
				(*PING_PeriodicTask)(OS_Time() - startTime);
    }
}
*/

//----------------- PortB_Handler -----------------------
//occurs after PING_Signal arms ther interrupt
void GPIOPortB_Handler(void){
    static unsigned long startTime, data, time;
		//TIMER0_ICR_R = TIMER_ICR_CAECINT;// acknowledge timer0A capture match
   // GPIO_PORTB_ICR_R = 0x40;      // acknowledge flag6
	time = OS_Time(); 	
	data = GPIO_PORTB_DATA_R;
    if((GPIO_PORTB_DATA_R & 0x40) == 0x40){ //PB6 high
        startTime = time;
    }
   else if((GPIO_PORTB_DATA_R & 0x40) == 0){ //PB6 low
        //disarm the interrupt for PB6?
				//(*PING_PeriodicTask)(OS_Time() - startTime);
				PING_Time = OS_TimeDifference(startTime, time);
				OS_AddThread(PING_PeriodicTask,128,1);
				//(*PING_PeriodicTask)(OS_TimeDifference(startTime, OS_Time()));
	 }
GPIO_PORTB_ICR_R = 0x40;      // acknowledge flag6
}

