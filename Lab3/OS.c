////////// OS.c////////////////

#include "OS.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "inc/tm4c123gh6pm.h"
#include "PLL.h"


//80000  
// edit these depending on your clock        
#define TIME_1MS    80000                 
#define TIME_2MS    (2*TIME_1MS)  
#define TIME_500US  (TIME_1MS/2)  
#define TIME_250US  (TIME_1MS/5)
#define PERIOD TIME_500US


#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC2_GPIOC      0x00000004  // port C Clock Gating Control
#define NVIC_ST_CTRL_R          (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
#define NVIC_ST_RELOAD_R        (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile unsigned long *)0xE000E018))
#define NVIC_INT_CTRL_R         (*((volatile unsigned long *)0xE000ED04))
#define NVIC_EN0_INT21          0x00200000  // Interrupt 21 enable
#define NVIC_INT_CTRL_PENDSTSET 0x04000000  // Set pending SysTick interrupt
#define NVIC_SYS_PRI3_R         (*((volatile unsigned long *)0xE000ED20))  // Sys. Handlers 12 to 15 Priority  
#define NVIC_EN0_INT19          0x00080000  // Interrupt 19 enable
#define PF4 										(*((volatile unsigned long *)0x40025040))
#define PF0 										(*((volatile unsigned long *)0x40025004))
#define NVIC_EN0_INT30					0x40000000
#define PF2  (*((volatile unsigned long *)0x40025010))
#define PE1  (*((volatile unsigned long *)0x40024008))
#define PE2  (*((volatile unsigned long *)0x40024010))

#define EMPTYSTACK -1
#define MAXSTACK NUMTHREADS

#define FIFOSIZE 32
#define FIFOSUCCESS 1
#define FIFOFAIL 0 

typedef  long DataType;
DataType volatile *PutPt; 
DataType volatile *GetPt; 
DataType static Fifo[FIFOSIZE]; 



void (*PF4Task)(void);  // user function
void (*PF0Task)(void);
void (*PeriodicTask1)(void);	// Periodic background task
void (*PeriodicTask2)(void);	// Periodic background task
void (*WakeupTask)(void);	// Periodic background task
void init_openThreads (void); 
void push_OpenThreads (int threadId); 
int pop_OpenThreads (void); 
void SysClock_Init(int period);


extern tcbType tcbs[NUMTHREADS];
extern long Stacks [NUMTHREADS][STACKSIZE]; 
extern unsigned long NumCreated;
extern unsigned long NumSleeping; 
extern tcbType *RunPt; 
extern tcbType *SleepPt;
extern tcbType *StartPt;
unsigned long static LastPF4, LastPF0;
extern tcbType *tempRunPt; 
int open_threads [NUMTHREADS]; 
int idxOpenThreads; 
mailType mail; 
static int SysTime = 0; 
// ******** OS_Init ************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: serial, ADC, systick, LaunchPad I/O and timers 
// input:  none
// output: none
void OS_Init(void){
	OS_DisableInterrupts(); 
	PLL_Init();					// Incompatable with simulator (I think)
	SysClock_Init(100000);		// give the system clock 1 ms ticks
//	SysCtClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_6MHZ | SYSCTL_OSC_MAIN);
	NVIC_ST_CTRL_R = 0; 
	NVIC_ST_RELOAD_R = NVIC_ST_RELOAD_M;
	NVIC_ST_CURRENT_R = 0; 
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R & 0x00FFFFFF) | 0xE0000000; //priority 7 
  //NVIC_SYS_PRI3_R &= ~(7<<21);
	NVIC_SYS_PRI3_R |= (7<<21);
	NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC;
	RunPt = &tcbs[0]; 
	SleepPt = 0;
	StartPt = 0;
	idxOpenThreads = EMPTYSTACK; 
	init_openThreads(); 
	NumSleeping = 0; 
}


// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, long value){
	semaPt->Value = value;
	semaPt->newestThread = 0; 
	semaPt->oldestThread = 0; 
} 

// ******** OS_Wait ************
// decrement semaphore 
// Lab2 spinlock
// Lab3 block if less than zero
// input:  pointer to a counting semaphore
// output: none
void OS_Wait(Sema4Type *semaPt){
	long status; 
	tcbType* temp; 
	status = StartCritical(); 
	semaPt->Value = semaPt->Value - 1;
	if(semaPt->Value < 0){
		//save the next pt --- figure something out because this is a prioirty scheduler
		 if(RunPt->nextThread->priority == StartPt->priority){
			tempRunPt = RunPt->nextThread; 
		 }
		 else{
			tempRunPt = StartPt;
			}
		if(RunPt == StartPt){
		 StartPt = (StartPt->nextThread != StartPt) ? StartPt->nextThread : 0;
	  }
		//unlink it from the active list
			RunPt->prevThread->nextThread = RunPt->nextThread; 
			RunPt->nextThread->prevThread = RunPt->prevThread; 
			
		
		//add it at the end of the semaphore's blocked threads
			if (semaPt->Value == -1) {
				semaPt->newestThread = RunPt; 
				semaPt->oldestThread = RunPt; 
				semaPt->newestThread->nextThread = 0; 
				semaPt->newestThread->prevThread = 0; 
			} 
			else {
				temp = semaPt->newestThread; 
				semaPt->newestThread->nextThread = RunPt; 
				semaPt->newestThread = RunPt; 
				semaPt->newestThread->nextThread = 0; 
				semaPt->newestThread->prevThread = temp; 
				
				
			} 
			// Suspend current thread
		NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Trigger PendSV
		EndCritical(status);
	}
	else{
		EndCritical(status); 
	}
}

// ******** OS_Signal ************
// increment semaphore 
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt){
	long status;
	tcbType* unblock;
	tcbType* temp;
	
	status = StartCritical();
	semaPt->Value = semaPt->Value + 1;
	if (semaPt->Value <= 0) {
		//Wake-up one thread
		//First, figure out which one to unblock -- this implements bounded waiting
		unblock = semaPt->oldestThread; 
		semaPt->oldestThread = unblock->nextThread; 
		/**********************************************************/
		//add it to the active list
		if(StartPt){
		//highest priority, add to front
		if(StartPt->priority > unblock->priority){
			unblock->nextThread = StartPt;
			unblock->prevThread = StartPt->prevThread;
			StartPt->prevThread->nextThread = unblock;
			StartPt->prevThread = unblock;
			StartPt = unblock;
		}else{
			temp = StartPt;
			while(temp->nextThread != StartPt){		// search for spot in middle
				if((temp->priority             <= unblock->priority) &&
					 (temp->nextThread->priority > unblock->priority)){
					break;
				}
				temp = temp->nextThread;
			}
			unblock->nextThread = temp->nextThread;
			unblock->prevThread = temp;
			temp->nextThread->prevThread = unblock;
			temp->nextThread = unblock;
		}
	}else{	// no other link in the list
		StartPt = unblock;
		unblock->nextThread = unblock;
		unblock->prevThread = unblock;
	}	
		
		//if the thread is higher priority , then suspend this one
		if(unblock->priority > RunPt->priority) {
			EndCritical(status); 
			tempRunPt = unblock;
			OS_Suspend(); 
		}
		else {
			EndCritical(status); 
		}
	}
	else {
		EndCritical(status); 
	}
} 

// ******** OS_bWait ************
// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input:  pointer to a binary semaphore
// output: none
void OS_bWait(Sema4Type *semaPt){
		OS_DisableInterrupts();
	while(semaPt->Value <= 0){
		OS_EnableInterrupts();
		OS_Suspend();
		OS_DisableInterrupts();
	}
	semaPt->Value = 0;
	OS_EnableInterrupts();
}

// ******** OS_bSignal ************
// Lab2, set to 1
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a binary semaphore
// output: none
void OS_bSignal(Sema4Type *semaPt){
	OS_DisableInterrupts();
	semaPt->Value =1;
	OS_EnableInterrupts();
}


void SetInitialStack(int i){
  tcbs[i].stackPt = &Stacks[i][STACKSIZE-16]; // thread stack pointer
  Stacks[i][STACKSIZE-1] = 0x01000000;   // thumb bit
  Stacks[i][STACKSIZE-3] = 0x14141414;   // R14
  Stacks[i][STACKSIZE-4] = 0x12121212;   // R12
  Stacks[i][STACKSIZE-5] = 0x03030303;   // R3
  Stacks[i][STACKSIZE-6] = 0x02020202;   // R2
  Stacks[i][STACKSIZE-7] = 0x01010101;   // R1
  Stacks[i][STACKSIZE-8] = 0x00000000;   // R0
  Stacks[i][STACKSIZE-9] = 0x11111111;   // R11
  Stacks[i][STACKSIZE-10] = 0x10101010;  // R10
  Stacks[i][STACKSIZE-11] = 0x09090909;  // R9
  Stacks[i][STACKSIZE-12] = 0x08080808;  // R8
  Stacks[i][STACKSIZE-13] = 0x07070707;  // R7
  Stacks[i][STACKSIZE-14] = 0x06060606;  // R6
  Stacks[i][STACKSIZE-15] = 0x05050505;  // R5
  Stacks[i][STACKSIZE-16] = 0x04040404;  // R4
}

//******** OS_AddThread *************** 
// add a foregound thread to the scheduler
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields

int OS_AddThread (void (*threadName) (void),int sleepState, int priority)  {
	int threadId; 
	tcbType *temp;
	threadId = pop_OpenThreads(); 
	
	if(threadId == -1){ 			// Put this here to ease debugging
		OS_DisableInterrupts(); // Should probably print error message instead
		while(1){}
	}
	
	//call the function that initializes stack
	SetInitialStack(threadId); Stacks[threadId][STACKSIZE-2] = (long)threadName;   // initialize stack and pc on the stack
	tcbs[threadId].prevThread = &tcbs[threadId]; 
	tcbs[threadId].nextThread = &tcbs[threadId]; // Point to itself by default
	tcbs[threadId].threadId   = threadId; 
	tcbs[threadId].sleepState = sleepState; 
	tcbs[threadId].priority   = priority; 
	
	if(StartPt){
		//highest priority, add to front
		if(StartPt->priority > tcbs[threadId].priority){
			tcbs[threadId].nextThread = StartPt;
			tcbs[threadId].prevThread = StartPt->prevThread;
			StartPt->prevThread->nextThread = &tcbs[threadId];
			StartPt->prevThread = &tcbs[threadId];
			StartPt = &tcbs[threadId];
		}else{
			temp = StartPt;
			while(temp->nextThread != StartPt){		// search for spot in middle
				if((temp->priority             <= tcbs[threadId].priority) &&
					 (temp->nextThread->priority > tcbs[threadId].priority)){
					break;
				}
				temp = temp->nextThread;
			}
			tcbs[threadId].nextThread = temp->nextThread;
			tcbs[threadId].prevThread = temp;
			temp->nextThread->prevThread = &tcbs[threadId];
			temp->nextThread = &tcbs[threadId];
		}
	}else{	// no other link in the list
		StartPt = &tcbs[threadId];
		tcbs[threadId].nextThread = &tcbs[threadId];
		tcbs[threadId].prevThread = &tcbs[threadId];
	}	
			
	// Code from round robin scheduler
//	if(threadId >0){
//		tcbs[threadId].nextThread = tcbs[threadId-1].nextThread; 
//		tcbs[threadId].prevThread = &tcbs[threadId-1];
//		tcbs[threadId -1].nextThread = &tcbs[threadId];
//		tcbs[0].prevThread = &tcbs[threadId]; 
//	} 
	
	return 1; 
} 	




//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
unsigned long OS_Id(void){
	return RunPt->threadId;
}

//******** OS_AddPeriodicThread *************** 
// add a background periodic task
// typically this function receives the highest priority
// Inputs: pointer to a void/void background function
//         period given in system time units (12.5ns)
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// You are free to select the time resolution for this function
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In lab 2, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, this command will be called 0 1 or 2 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddPeriodicThread(void(*task)(void), 		
   unsigned long period, unsigned long priority){
 
  static int bgThreadNum = 0;
	switch (bgThreadNum){
		case 0:
			SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER1; // 0) activate timer0
			PeriodicTask1 = task;             // user function 
			TIMER1_CTL_R &= ~0x00000001;     // 1) disable timer1A during setup
			TIMER1_CFG_R = 0x00000000;       // 2) configure for 32-bit timer mode
			TIMER1_TAMR_R = 0x00000002;      // 3) configure for periodic mode
			TIMER1_TAILR_R = period;         // 4) reload value
			TIMER1_TAPR_R = 49;              // 5) 1us timer0A 49
			TIMER1_ICR_R = 0x00000001;       // 6) clear timer1A timeout flag
			TIMER1_IMR_R |= 0x00000001;      // 7) arm timeout interrupt
			NVIC_PRI5_R &= ~(7<<13); 
			NVIC_PRI5_R |= (priority<<13);; // 8) priority 2
			NVIC_EN0_R |= NVIC_EN0_INT21;    // 9) enable interrupt 21 in NVIC
			TIMER1_CTL_R |= 0x00000001;      // 10) enable timer1A
			break;
		
		case 1:
			SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER2; // 0) activate timer0
			PeriodicTask2 = task;             // user function 
			TIMER2_CTL_R &= ~0x00000001;     // 1) disable timer1A during setup
			TIMER2_CFG_R = 0x00000000;       // 2) configure for 32-bit timer mode
			TIMER2_TAMR_R = 0x00000002;      // 3) configure for periodic mode
			TIMER2_TAILR_R = period;         // 4) reload value
			TIMER2_TAPR_R = 49;              // 5) 1us timer0A 49
			TIMER2_ICR_R = 0x00000001;       // 6) clear timer1A timeout flag
			TIMER2_IMR_R |= 0x00000001;      // 7) arm timeout interrupt
			NVIC_PRI5_R &= ~(7<<29); 
			NVIC_PRI5_R |= (priority<<29);; // 8) priority 2
			NVIC_EN0_R |= (1<<23);    // 9) enable interrupt 21 in NVIC
			TIMER2_CTL_R |= 0x00000001;      // 10) enable timer1A
			break;
		default:
			return 0;
		
	}
	bgThreadNum ++;
	return 1;
}

void Timer1A_Handler(void){
  TIMER1_ICR_R = TIMER_ICR_TATOCINT;	// acknowledge timer1A timeout
  (*PeriodicTask1)();                // execute user task
}

void Timer2A_Handler(void){
  TIMER2_ICR_R = TIMER_ICR_TATOCINT;	// acknowledge timer1A timeout
  (*PeriodicTask2)();                // execute user task
}




	 

//******** OS_AddSW1Task *************** 
// add a background task to run whenever the SW1 (PF4) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In labs 2 and 3, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSW1Task(void(*task)(void), unsigned long priority){unsigned long volatile delay;
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;
	PF4Task = task;
	GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
	GPIO_PORTF_CR_R |= 0x10;
  GPIO_PORTF_DIR_R &= ~0x10;    // (c) make PF4 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x10;  //     disable alt funct on PF4
  GPIO_PORTF_DEN_R |= 0x10;     //     enable digital I/O on PF4   
  GPIO_PORTF_PCTL_R &= ~0x000F000F; // configure PF4 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       //     disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x10;     //     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x10;     // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R |= 0x10;    //     PF4 is not both edges
//  GPIO_PORTF_IEV_R &= ~0x10;    //     PF4 falling edge event
  GPIO_PORTF_ICR_R = 0x10;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x10;      // (f) arm interrupt on PF4
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // (g) priority 5
  NVIC_EN0_R = NVIC_EN0_INT30;  // (h) enable interrupt 30 in NVIC
	LastPF4 = PF4;
	return 1;
}

void static DebounceTaskPF0 () {
			OS_Sleep(10); 
			LastPF0 = PF0; 
			GPIO_PORTF_ICR_R = 0x01;
			GPIO_PORTF_IM_R |= 0x01; 
			OS_Kill(); 
}
void static DebounceTaskPF4 () {
			OS_Sleep(10); 
			LastPF4 = PF4; 
			GPIO_PORTF_ICR_R = 0x10;
			GPIO_PORTF_IM_R |= 0x10; 
			OS_Kill(); 
	}
//void GPIOPortF_Handler(void){
//	PE2 ^= 0x04;	
//  if(LastPF4){
//		(*PF4Task)();                // execute user task
//	}
//	GPIO_PORTF_IM_R &= ~0x10; 
//	
//	NumCreated += OS_AddThread (&DebounceTask, 0, 0, 0); 
//	PE2 ^= 0x04;
//}

void GPIOPortF_Handler(void){
	if (GPIO_PORTF_RIS_R & 0x10) { 
		if(LastPF4){
		(*PF4Task)();                // execute user task
		}
		GPIO_PORTF_IM_R &= ~0x10; 
		NumCreated += OS_AddThread (&DebounceTaskPF4, 0, 0); 
	} 
	
	else if (GPIO_PORTF_RIS_R & 0x01) {
		if(LastPF0){
		(*PF0Task)();                // execute user task
		}
		GPIO_PORTF_IM_R &= ~0x01; 
	
		NumCreated += OS_AddThread (&DebounceTaskPF0,0, 0); 
	} 
}

//******** OS_AddSW2Task *************** 
// add a background task to run whenever the SW2 (PF0) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is highest, 5 is lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed user task will run to completion and return
// This task can not spin block loop sleep or kill
// This task can call issue OS_Signal, it can call OS_AddThread
// This task does not have a Thread ID
// In lab 2, this function can be ignored
// In lab 3, this command will be called will be called 0 or 1 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSW2Task(void(*task)(void), unsigned long priority){
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;
	PF0Task = task;
	GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
	GPIO_PORTF_CR_R |= 0x01;
  GPIO_PORTF_DIR_R &= ~0x01;    // (c) make PF0 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x01;  //     disable alt funct on PF4
  GPIO_PORTF_DEN_R |= 0x01;     //     enable digital I/O on PF4   
  GPIO_PORTF_PCTL_R &= ~0x0000000F; // configure PF4 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       //     disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x01;     //     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x01;     // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R |= 0x01;    //     PF4 is not both edges
//  GPIO_PORTF_IEV_R &= ~0x01;    //     PF4 falling edge event
  GPIO_PORTF_ICR_R = 0x01;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x01;      // (f) arm interrupt on PF4
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // (g) priority 5
  NVIC_EN0_R = NVIC_EN0_INT30;  // (h) enable interrupt 30 in NVIC
	LastPF0 = PF0;
	return 1;
}

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(unsigned long sleepTime){
// Be sure to reset SleepPt to zero (when neccessary) in the function that wakes up functions
	tcbType *tempRun;
	OS_DisableInterrupts();
	
	if(RunPt->nextThread->priority == StartPt->priority){
		tempRun = RunPt->nextThread; 
	}else{
		tempRun = StartPt;
	}
	RunPt->sleepState = sleepTime;
	if(RunPt == StartPt){
		StartPt = (StartPt->nextThread != StartPt) ? StartPt->nextThread : 0;
	}
//	*SleepPt = RunPt; 
//	SleepPt += sizeof(tcbType); 

	// let's unlink first
	RunPt->nextThread->prevThread = RunPt->prevThread;
	RunPt->prevThread->nextThread = RunPt->nextThread;
	
	// Insert into Sleep List
	if(SleepPt != 0){
		RunPt->nextThread = SleepPt; 
		RunPt->prevThread = SleepPt->prevThread;
		SleepPt->prevThread->nextThread = RunPt;
		SleepPt->prevThread = RunPt;
		
	}else{
		RunPt->prevThread = RunPt;
		RunPt->nextThread = RunPt;
	}
	SleepPt = RunPt; 
	tempRunPt = tempRun; 
	NumSleeping++; 
	NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Trigger PendSV
	OS_EnableInterrupts();
}


// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void){
	OS_DisableInterrupts();
	if(RunPt == StartPt){ StartPt = RunPt->nextThread;}
	RunPt->nextThread->prevThread = RunPt->prevThread;
	RunPt->prevThread->nextThread = RunPt->nextThread;
	NumCreated--;
	push_OpenThreads(RunPt->threadId); 	// Need to make it apparent that this space is now availible in tcbs
	OS_EnableInterrupts();
	OS_Suspend();
} 

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking 
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void){
	NVIC_ST_CURRENT_R =0; 
	NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PENDSTSET; 
}
 
// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(unsigned long size){
	PutPt = GetPt = &Fifo[0]; 

}
// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int OS_Fifo_Put(unsigned long data){
	DataType volatile *nextPutPt; 
	nextPutPt = PutPt + 1; 
	if (nextPutPt == &Fifo[FIFOSIZE]){
		nextPutPt = &Fifo[0]; 
	}
		if (nextPutPt == GetPt) {
			return (FIFOFAIL); 
		}
		else {
			*(PutPt) = data; 
			PutPt = nextPutPt; 
			return (FIFOSUCCESS); 
		} 
} 

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long OS_Fifo_Get(void){
	int data; 
	if (PutPt == GetPt) {
		return (FIFOFAIL); 
	}
	data = *(GetPt++); 
	if(GetPt == &Fifo[FIFOSIZE]) {
		GetPt = &Fifo[0]; 
	}
	return (data);
}

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long OS_Fifo_Size(void){return FIFOSIZE;}

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void){
	OS_InitSemaphore(&mail.boxFree, 1); //1 means FREE
	mail.value	= -1; 
	OS_InitSemaphore(&mail.DataValid, 0); //Since mailbox is free, data is not valid
}

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(unsigned long data){
	OS_bWait(&mail.boxFree); 
	mail.value = data; 
	OS_bSignal(&mail.DataValid); 

}

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
unsigned long OS_MailBox_Recv(void){
	int tempValue; 
	OS_bWait (&mail.DataValid); 
	tempValue = mail.value; 
	OS_bSignal(&mail.boxFree); 
	return tempValue;
}

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
unsigned long OS_Time(void){return (SysTime*PERIOD);} //need to fix this to ns

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
unsigned long OS_TimeDifference(unsigned long start, unsigned long stop){
	return (stop-start);
}

// ******** OS_ClearMsTime ************
// sets the system time to zero (from Lab 1)
// Inputs:  none
// Outputs: none
// You are free to change how this works
void OS_ClearMsTime(void){
	SysTime = 0; 
}

// ******** OS_MsTime ************
// reads the current time in msec (from Lab 1)
// Inputs:  none
// Outputs: time in ms units
// You are free to select the time resolution for this function
// It is ok to make the resolution to match the first call to OS_AddPeriodicThread
unsigned long OS_MsTime(void){return SysTime;}

//******** OS_Launch *************** 
// start the scheduler, enable interrupts
// Inputs: number of 12.5ns clock cycles for each time slice
//         you may select the units of this parameter
// Outputs: none (does not return)
// In Lab 2, you can ignore the theTimeSlice field
// In Lab 3, you should implement the user-defined TimeSlice field
// It is ok to limit the range of theTimeSlice to match the 24-bit SysTick
void OS_Launch(unsigned long theTimeSlice){
	RunPt = StartPt;
	NVIC_ST_RELOAD_R = theTimeSlice - 1; 
	NVIC_ST_CTRL_R = 0x00000007; 
	StartOS(); 

}

/*********************PRIVATE FUNCTIONS*****************************/
void init_openThreads () {
	int i; 
	for (i = 0; i < NUMTHREADS; i++){
		open_threads[i] = MAXSTACK-1-i; 
	}
	idxOpenThreads = MAXSTACK-1; 
} 
void push_OpenThreads (int threadId) {
	if (idxOpenThreads != MAXSTACK) { 
   open_threads[++idxOpenThreads] = threadId;
	} 
} 

int pop_OpenThreads () {
	return  ((idxOpenThreads != EMPTYSTACK) ? open_threads[idxOpenThreads--] : -1);
} 

void Wakeup(void){
	int idx; tcbType *tempSleep; tcbType *temp; 
	int numWakeUp = 0;
	SysTime++;
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
	//NumSleeping -= numWakeUp;
}

void SysClock_Init(int period){
			SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER3; // 0) activate timer0
			WakeupTask = &Wakeup;             // user function 
			TIMER3_CTL_R &= ~0x00000001;     // 1) disable timer1A during setup
			TIMER3_CFG_R = 0x00000000;       // 2) configure for 32-bit timer mode
			TIMER3_TAMR_R = 0x00000002;      // 3) configure for periodic mode
			TIMER3_TAILR_R = period;         // 4) reload value
			TIMER3_TAPR_R = 49;              // 5) 1us timer0A 49
			TIMER3_ICR_R = 0x00000001;       // 6) clear timer1A timeout flag
			TIMER3_IMR_R |= 0x00000001;      // 7) arm timeout interrupt
			NVIC_PRI8_R &= ~(7<<29); // 8) priority 0
			NVIC_EN1_R |= 0X08;    // 9) enable interrupt 21 in NVIC
			TIMER3_CTL_R |= 0x00000001;      // 10) enable timer3A
}


void Timer3A_Handler(void){
	static int counter = 1000; 
	TIMER3_ICR_R = TIMER_ICR_TATOCINT;	// acknowledge timer1A timeout
	
	if (counter == 0) {
		PF2 ^= 0x04; 
		counter = 1000; 
	} 
	else {
		counter--; 
	} 
	(*WakeupTask)();
}
