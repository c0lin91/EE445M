// Lab2.c
// Runs on LM4F120/TM4C123
// Real Time Operating System for Labs 2 and 3
// Lab2 Part 1: Testmain1 and Testmain2
// Lab2 Part 2: Testmain3 Testmain4  and main
// Lab3: Testmain5 Testmain6, Testmain7, and main (with SW2)

// Jonathan W. Valvano 1/31/14, valvano@mail.utexas.edu
// EE445M/EE380L.6 
// You may use, edit, run or distribute this file 
// You are free to change the syntax/organization of this file

// LED outputs to logic analyzer for OS profile 
// PF1 is preemptive thread switch
// PF2 is periodic task, samples PD3
// PF3 is SW1 task (touch PF4 button)

// Button inputs
// PF0 is SW2 task (Lab3)
// PF4 is SW1 button input

// Analog inputs
// PD3 Ain3 sampled at 2k, sequencer 3, by DAS software start in ISR
// PD2 Ain5 sampled at 250Hz, sequencer 0, by Producer, timer tigger

#include "OS.h"
#include "inc/tm4c123gh6pm.h"
#include "ST7735.h"
#include "ADC.h"
#include "UART2.h"
#include <string.h> 
#include "Interpreter.h"
#include <stdio.h>
#include "Filter.h"
#include "FIFO.h"
#include "math.h" 

//*********Prototype for FFT in cr4_fft_64_stm32.s, STMicroelectronics
void cr4_fft_64_stm32(void *pssOUT, void *pssIN, unsigned short Nbin);
//*********Prototype for PID in PID_stm32.s, STMicroelectronics
short PID_stm32(short Error, short *Coeff);
void WaitForInterrupt (void); 
void ButtonPush1(void);
void switchWork1(void);

unsigned long NumCreated;   // number of foreground threads created
unsigned long NumSleeping;  // number of sleeping threads
unsigned long PIDWork;      // current number of PID calculations finished
unsigned long FilterWork;   // number of digital filter calculations finished
unsigned long NumSamples;   // incremented every ADC sample, in Producer
extern char filterFlag;
char switchFlag = 0;
extern char fftActive;

#define FS 400            // producer/consumer sampling
#define RUNLENGTH (FS * 10) //(2000 * FS) display results and quit when NumSamples==RUNLENGTH
// 20-sec finite time experiment duration 

#define PERIOD TIME_500US // DAS 2kHz sampling period in system time units
long x[128],y[128];         // input and output arrays for FFT


#define ADCFIFOSIZE 64 // must be a power of 2
#define ADCFIFOSUCCESS 1
#define ADCFIFOFAIL    0

AddIndexFifo(ADC,ADCFIFOSIZE,long, ADCFIFOSUCCESS,ADCFIFOFAIL)

tcbType tcbs[NUMTHREADS];
long Stacks [NUMTHREADS][STACKSIZE];
tcbType *RunPt; 
tcbType *SleepPt;
tcbType *tempRunPt; 
tcbType *StartPt;
Sema4Type toDisplay;


//---------------------User debugging-----------------------
unsigned long DataLost;     // data sent by Producer, but not received by Consumer
long MaxJitter;             // largest time jitter between interrupts in usec
#define JITTERSIZE 64
unsigned long const JitterSize=JITTERSIZE;
unsigned long JitterHistogram[JITTERSIZE]={0,};
#define PE0  (*((volatile unsigned long *)0x40024004))
#define PE1  (*((volatile unsigned long *)0x40024008))
#define PE2  (*((volatile unsigned long *)0x40024010))
#define PE3  (*((volatile unsigned long *)0x40024020))
#define PF1 										(*((volatile unsigned long *)0x40025008))
#define PF2  (*((volatile unsigned long *)0x40025010))

void PortE_Init(void){ unsigned long volatile delay;
  SYSCTL_RCGC2_R |= 0x10;       // activate port E
  
	delay = SYSCTL_RCGC2_R;        
  delay = SYSCTL_RCGC2_R;
	delay = SYSCTL_RCGC2_R;
	delay = SYSCTL_RCGC2_R;
	delay = SYSCTL_RCGC2_R;
	
  GPIO_PORTE_DIR_R |= 0x0F;    // make PE3-0 output heartbeats
  GPIO_PORTE_AFSEL_R &= ~0x0F;   // disable alt funct on PE3-0
  GPIO_PORTE_DEN_R |= 0x0F;     // enable digital I/O on PE3-0
  GPIO_PORTE_PCTL_R = ~0x0000FFFF;
  GPIO_PORTE_AMSEL_R &= ~0x0F;;      // disable analog functionality on PF
	PE0 = 0;
	PE1 = 0;
	PE2 = 0;
	PE3 = 0; 
}

void PortF_Init(void){ unsigned long volatile delay;
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;
  GPIO_PORTF_DIR_R |= 0x06;    // (c) make PF1-2 out
  GPIO_PORTF_AFSEL_R &= ~0x06;  //     disable alt funct on PF1-2
  GPIO_PORTF_DEN_R |= 0x06;     //     enable digital I/O on PF1-2   
  GPIO_PORTF_PCTL_R &= ~0x00000FF0; // configure PF1-2 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       //     disable analog functionality on PF
	PF1 = 0;
	PF2 = 0;
	
}

	

	
//------------------Task 1--------------------------------
// 2 kHz sampling ADC channel 1, using software start trigger
// background thread executed at 2 kHz
// 60-Hz notch high-Q, IIR filter, assuming fs=2000 Hz
// y(n) = (256x(n) -503x(n-1) + 256x(n-2) + 498y(n-1)-251y(n-2))/256 (2k sampling)
// y(n) = (256x(n) -476x(n-1) + 256x(n-2) + 471y(n-1)-251y(n-2))/256 (1k sampling)
long Filter(long data){
static long x[6]; // this MACQ needs twice
static long y[6];
static unsigned long n=3;   // 3, 4, or 5
  n++;
  if(n==6) n=3;     
  x[n] = x[n-3] = data;  // two copies of new data
  y[n] = (256*(x[n]+x[n-2])-503*x[n-1]+498*y[n-1]-251*y[n-2]+128)/256;
  y[n-3] = y[n];         // two copies of filter outputs too
  return y[n];
} 
//******** DAS *************** 
// background thread, calculates 60Hz notch filter
// runs 2000 times/sec
// samples channel 4, PD3,
// inputs:  none
// outputs: none
unsigned long DASoutput;
void DAS(void){ 
unsigned long input;  
unsigned static long LastTime;  // time at previous ADC sample
unsigned long thisTime;         // time at current ADC sample
long jitter;                    // time between measured and expected, in us
	//PE1 ^= 0x02;								// debugging tool
  if(NumSamples < RUNLENGTH){   // finite time run
    PE0 ^= 0x01;
    input = ADC_In();           // channel set when calling ADC_Init
    PE0 ^= 0x01;
    thisTime = OS_Time();       // current time, 12.5 ns
    DASoutput = Filter(input);
    FilterWork++;        // calculation finished
    if(FilterWork>1){    // ignore timing of first interrupt
      unsigned long diff = OS_TimeDifference(LastTime,thisTime);
      if(diff>PERIOD){
        jitter = (diff-PERIOD+4)/8;  // in 0.1 usec
      }else{
        jitter = (PERIOD-diff+4)/8;  // in 0.1 usec
      }
      if(jitter > MaxJitter){
        MaxJitter = jitter; // in usec
      }       // jitter should be 0
      if(jitter >= JitterSize){
        jitter = JITTERSIZE-1;
      }
      JitterHistogram[jitter]++; 
    }
    LastTime = thisTime;
    PE0 ^= 0x01;
  }
	else{
		NumSamples ++;
	}
}
//--------------end of Task 1-----------------------------

//------------------Task 2--------------------------------
// background thread executes with SW1 button
// one foreground task created with button push
// foreground treads run for 2 sec and die
// ***********ButtonWork*************
void ButtonWork(void){
unsigned long myId = OS_Id(); 
	//PE1 ^= 0x02;
	OS_bWait(&toDisplay); 
  ST7735_Message(1,0,"NumCreated =",NumCreated); 
	//OS_bSignal(&toDisplay);
  OS_Sleep(50);     // set this to sleep for 50msec
	//OS_bWait(&toDisplay);
  ST7735_Message(1,1,"PIDWork     =",PIDWork);
  ST7735_Message(1,2,"DataLost    =",DataLost);
  ST7735_Message(1,3,"Jitter 0.1us=",MaxJitter);
	OS_bSignal(&toDisplay); 
 // PE1 ^= 0x02;
  OS_Kill();  // done, OS does not return from a Kill
} 

//************SW1Push*************
// Called when SW1 Button pushed
// Adds another foreground task
// background threads execute once and return
void SW1Push(void){
	int i, pixelPos;
	switchFlag = 0;
	PF1 ^= 0x02;
	for(i=0; i<64;i++){
		pixelPos = 148 - ((x[i]* 366)/10000) ;
		ST7735_PlotBar(127);
		ST7735_PlotPoint(pixelPos); // called N times
		ST7735_PlotNext();
	}
	
	//PE1 ^= 0x02;
}
//************SW2Push*************
// Called when SW2 Button pushed, Lab 3 only
// Adds another foreground task
// background threads execute once and return
void SW2Push(void){
	switchFlag = 1;
	PF1 ^= 0x02;
}
//--------------end of Task 2-----------------------------

//------------------Task 3--------------------------------
// hardware timer-triggered ADC sampling at 400Hz
// Producer runs as part of ADC ISR
// Producer uses fifo to transmit 400 samples/sec to Consumer
// every 64 samples, Consumer calculates FFT
// every 2.5ms*64 = 160 ms (6.25 Hz), consumer sends data to Display via mailbox
// Display thread updates LCD with measurement

//******** Producer *************** 
// The Producer in this lab will be called from your ADC ISR
// A timer runs at 400Hz, started by your ADC_Collect
// The timer triggers the ADC, creating the 400Hz sampling
// Your ADC ISR runs when ADC data is ready
// Your ADC ISR calls this function with a 12-bit sample 
// sends data to the consumer, runs periodically at 400Hz
// inputs:  none
// outputs: none
void Producer(unsigned long data){  
	static int i =0;
	// the i counter is hacky way of slowing down sampling time, 
	//initialization was being weird so i had to add this to slow it down more. 
	// We should fix this when we can -CH
	if(i==0){										
		if(NumSamples < RUNLENGTH){   // finite time run
			NumSamples++;               // number of samples
			if(OS_Fifo_Put(data) == 0){ // send to consumer
				DataLost++;
			} 
		}
	}	
	i = (i+1) % 5;
}
void Display(void); 


//******** plotV *************** 
// Creates a plot of Voltage vs. Time
// inputs:  none
// outputs: none
void plotV(void){
	int i, pixelPos, status;
	status = StartCritical(); 
//	OS_bWait(&toDisplay); 
	ST7735_FillScreen(0);
	ST7735_DrawStr(55,152, "Time", 0xFFFF, 0x0000);
	ST7735_DrawChar(3, 40, 'V', 0xFFFF, 0x0000, 1);
	ST7735_DrawChar(3, 48, 'o', 0xFFFF, 0x0000, 1);
	ST7735_DrawChar(3, 56, 'l', 0xFFFF, 0x0000, 1);
	ST7735_DrawChar(3, 64, 't', 0xFFFF, 0x0000, 1);
	ST7735_DrawChar(3, 72, 'a', 0xFFFF, 0x0000, 1);
	ST7735_DrawChar(3, 80, 'g', 0xFFFF, 0x0000, 1);
	ST7735_DrawChar(3, 88, 'e', 0xFFFF, 0x0000, 1);
	ST7735_DrawFastVLine(10, 0, 150, 0xFFFF);
	ST7735_DrawFastHLine(10,150,128,0xFFFF);
	
	for(i=0; i<128; i++){
		pixelPos = 148 - ((x[i]* 366)/10000)            ;
		ST7735_DrawPixel(11 + ((i* 117)/100), pixelPos, 0xFFFF); 
	}		
	//OS_bSignal(&toDisplay); 
	EndCritical(status);
}

//******** Consumer *************** 
// foreground thread, accepts data from producer
// calculates FFT, sends DC component to Display
// inputs:  none
// outputs: none
void Consumer(void){ 
unsigned long data,DCcomponent;   // 12-bit raw ADC sample, 0 to 4095
unsigned long t;                  // time in 2.5 ms
unsigned long myId = OS_Id(); 
  ADC_Collect(5, FS, &Producer); // start ADC sampling, channel 5, PD2, 400 Hz
  NumCreated += OS_AddThread(&Display, 0,1); 
  while(NumSamples < RUNLENGTH) { 
    PE2 = 0x04;
    for(t = 0; t < 64; t++){   // collect 64 ADC samples
      //data = ADC_In(); 			// is giving actual accurate data, but has high data loss
			data = OS_Fifo_Get();					// keeps returning 0
      x[t] = data;             // real part is 0 to 4095, imaginary part is 0
    }
    PE2 = 0x00;
    cr4_fft_64_stm32(y,x,64);  // complex FFT of last 64 ADC values
    DCcomponent = y[0]&0xFFFF; // Real part at frequency 0, imaginary part should be zero
    OS_MailBox_Send(DCcomponent); // called every 2.5ms*64 = 160ms
  }
	plotV();
  OS_Kill();  // done
}
//******** Display *************** 
// foreground thread, accepts data from consumer
// displays calculated results on the LCD
// inputs:  none                            
// outputs: none
void Display(void){ 
unsigned long data,voltage;
	OS_bWait(&toDisplay); 
  ST7735_Message(0,1,"Run length = ",(RUNLENGTH)/FS);   // top half used for Display
  while(NumSamples <= RUNLENGTH) { 
    data = OS_MailBox_Recv();
    voltage = 3000*data/4095;               // calibrate your device so voltage is in mV
    PE3 = 0x08;
    ST7735_Message(0,2,"v(mV) =",voltage);  
    PE3 = 0x00;
  } 
	OS_bSignal(&toDisplay); 
  OS_Kill();  // done
} 

//--------------end of Task 3-----------------------------

//------------------Task 4--------------------------------
// foreground thread that runs without waiting or sleeping
// it executes a digital controller 
//******** PID *************** 
// foreground thread, runs a PID controller
// never blocks, never sleeps, never dies
// inputs:  none
// outputs: none
short IntTerm;     // accumulated error, RPM-sec
short PrevError;   // previous error, RPM
short Coeff[3];    // PID coefficients
short Actuator;
void PID(void){ 
short err;  // speed error, range -100 to 100 RPM
unsigned long myId = OS_Id(); 
  PIDWork = 0;
  IntTerm = 0;
  PrevError = 0;
  Coeff[0] = 384;   // 1.5 = 384/256 proportional coefficient
  Coeff[1] = 128;   // 0.5 = 128/256 integral coefficient
  Coeff[2] = 64;    // 0.25 = 64/256 derivative coefficient*
  while(NumSamples < RUNLENGTH) { 
    for(err = -1000; err <= 1000; err++){    // made-up data
      Actuator = PID_stm32(err,Coeff)/256;
    }
    PIDWork++;        // calculation finished
  }
  for(;;){ }          // done
}

//--------------end of Task 4-----------------------------

//------------------Task 5--------------------------------
// UART background ISR performs serial input/output
// Two software fifos are used to pass I/O data to foreground
// The interpreter runs as a foreground thread
// The UART driver should call OS_Wait(&RxDataAvailable) when foreground tries to receive
// The UART ISR should call OS_Signal(&RxDataAvailable) when it receives data from Rx
// Similarly, the transmit channel waits on a semaphore in the foreground
// and the UART ISR signals this semaphore (TxRoomLeft) when getting data from fifo
// Modify your intepreter from Lab 1, adding commands to help debug 
// Interpreter is a foreground thread, accepts input from serial port, outputs to serial port
// inputs:  none
// outputs: none

    // just a prototype, link to your interpreter
// add the following commands, leave other commands, if they make sense
// 1) print performance measures 
//    time-jitter, number of data points lost, number of calculations performed
//    i.e., NumSamples, NumCreated, MaxJitter, DataLost, FilterWork, PIDwork
      
// 2) print debugging parameters 
//    i.e., x[], y[] 
//--------------end of Task 5-----------------------------


//*******************final user main DEMONTRATE THIS TO TA**********
int finalmain (void){ 
  OS_Init();           // initialize, disable interrupts
  UART_Init(); 
	PortE_Init();
	PortF_Init();
	ST7735_InitR(INITR_REDTAB);
	ST7735_FillScreen(0);
  DataLost = 0;        // lost data between producer and consumer
  NumSamples = 0;
  MaxJitter = 0;       // in 1us units
	OS_InitSemaphore(&toDisplay, 1);

//********initialize communication channels
  OS_MailBox_Init();
  OS_Fifo_Init(128);    // ***note*** 4 is not big enough*****

//*******attach background tasks***********
  OS_AddSW1Task(&SW1Push,1);
  OS_AddSW2Task(&SW2Push,2);  // add this line in Lab 3
  ADC_Init(4);  // sequencer 3, channel 4, PD3, sampling in DAS()
  OS_AddPeriodicThread(&DAS,PERIOD,1); // 2 kHz real time sampling of PD3
  NumCreated = 0 ;
	
// create initial foreground threads
	NumCreated += OS_AddThread (&Consumer		, 0, 1);
	NumCreated += OS_AddThread (&PID				, 0, 3);
	NumCreated += OS_AddThread (&Interpreter, 0, 2);

	
//  NumCreated += OS_AddThread(&Interpreter,128,2); 
//  NumCreated += OS_AddThread(&Consumer,128,1); 
//  NumCreated += OS_AddThread(&PID,128,3);  // Lab 3, make this lowest priority
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
}

//+++++++++++++++++++++++++DEBUGGING CODE++++++++++++++++++++++++
// ONCE YOUR RTOS WORKS YOU CAN COMMENT OUT THE REMAINING CODE
// 
//*******************Initial TEST**********
// This is the simplest configuration, test this first, (Lab 1 part 1)
// run this with 
// no UART interrupts
// no SYSTICK interrupts
// no timer interrupts
// no switch interrupts
// no ADC serial port or LCD output
// no calls to semaphores
unsigned long Count1;   // number of times thread1 loops
unsigned long Count2;   // number of times thread2 loops
unsigned long Count3;   // number of times thread3 loops
unsigned long Count4;   // number of times thread4 loops
unsigned long Count5;   // number of times thread5 loops
void Thread1(void){
  Count1 = 0;          
  for(;;){
    PE0 ^= 0x01;       // heartbeat
    Count1++;
    OS_Suspend();      // cooperative multitasking
  }
}
void Thread2(void){
  Count2 = 0;          
  for(;;){
    PE1 ^= 0x02;       // heartbeat
    Count2++;
    OS_Suspend();      // cooperative multitasking
  }
}
void Thread3(void){
  Count3 = 0;          
  for(;;){
    PE2 ^= 0x04;       // heartbeat
    Count3++;
    OS_Suspend();      // cooperative multitasking
  }
}

int Testmain1(void){  // Testmain1
  OS_Init();          // initialize, disable interrupts
  PortE_Init();       // profile user threads
  NumCreated = 0 ;
  NumCreated += OS_AddThread(&Thread1, 0, 1); 
  NumCreated += OS_AddThread(&Thread2, 0, 2); 
  NumCreated += OS_AddThread(&Thread3, 0, 3); 
  // Count1 Count2 Count3 should be equal or off by one at all times
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
}

//*******************Second TEST**********
// Once the initalize test runs, test this (Lab 1 part 1)
// no UART interrupts
// SYSTICK interrupts, with or without period established by OS_Launch
// no timer interrupts
// no switch interrupts
// no ADC serial port or LCD output
// no calls to semaphores
void Thread1b(void){
  Count1 = 0;          
  for(;;){
    PE0 ^= 0x01;       // heartbeat
    Count1++;
  }
}
void Thread2b(void){
  Count2 = 0;          
  for(;;){
    PE1 ^= 0x02;       // heartbeat
    Count2++;
  }
}
void Thread3b(void){
  Count3 = 0;          
  for(;;){
    PE2 ^= 0x04;       // heartbeat
    Count3++;
  }
}
int testmain2 (void){  // Testmain2
  OS_Init();           // initialize, disable interrupts
  PortE_Init();       // profile user threads
	NumCreated += OS_AddThread(&Thread1b, 0, 1); 
  NumCreated += OS_AddThread(&Thread2b, 0, 2); 
  NumCreated += OS_AddThread(&Thread3b, 0, 3); 
  // Count1 Count2 Count3 should be equal on average
  // counts are larger than testmain1
 
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
}

//*******************Third TEST**********
// Once the second test runs, test this (Lab 2 part 2)
// no UART1 interrupts
// SYSTICK interrupts, with or without period established by OS_Launch
// Timer interrupts, with or without period established by OS_AddPeriodicThread
// PortF GPIO interrupts, active low
// no ADC serial port or LCD output
// tests the spinlock semaphores, tests Sleep and Kill
Sema4Type Readyc;        // set in background
int Lost;
void BackgroundThread1c(void){   // called at 1000 Hz
  int i; tcbType *tempSleep; 
	//Iterate through all the sleeping threads and decrement the counter
	for(i = 0; i < NumSleeping; i++){
		SleepPt->sleepState -= 1; 
		if (!(SleepPt->sleepState)) {
			//wake up this thread
			NumSleeping--; 
			tempSleep = SleepPt->nextThread;
			SleepPt->prevThread = RunPt; 
			SleepPt->nextThread = RunPt->nextThread; 
			RunPt->nextThread->prevThread = SleepPt; 
			RunPt->nextThread = SleepPt;
			
			SleepPt = (NumSleeping) ? tempSleep : 0; 
		}
	}
	Count1++;
  OS_Signal(&Readyc);			
}
void Thread5c(void){
  for(;;){
    OS_Wait(&Readyc);
    Count5++;   // Count2 + Count5 should equal Count1 
    Lost = Count1-Count5-Count2;
  }
}
void Thread2c(void){
  OS_InitSemaphore(&Readyc,0);
  Count1 = 0;    // number of times signal is called      
  Count2 = 0;    
  Count5 = 0;    // Count2 + Count5 should equal Count1  
  NumCreated += OS_AddThread(&Thread5c, 0,3); 
  OS_AddPeriodicThread(&BackgroundThread1c,1000,0); 
  for(;;){
    OS_Wait(&Readyc);
    Count2++;   // Count2 + Count5 should equal Count1
  }
}

void Thread3c(void){
  Count3 = 0;          
  for(;;){
    Count3++;
  }
}
void Thread4c(void){ int i;
  for(i=0;i<64;i++){
    Count4++;
    OS_Sleep(10);
  }
  OS_Kill();
  Count4 = 0;
}
void BackgroundThread5c(void){   // called when Select button pushed
  NumCreated += OS_AddThread(&Thread4c,0,3); 
}
  

//Sema4Type toDisplay;
//int value = 0; 
//void Thread_write (void) {
//	OS_bWait(&toDisplay); 
//	value ++; 
//	OS_bSignal (&toDisplay); 
//} 

//void Thread_display (void){
//	OS_bWait(&toDisplay); 
//	ST7735_Message (0, 0, "I am in display thread function", value); 
//	OS_bSignal(&toDisplay); 
//} 

int testmain3 (void){   // Testmain3
  Count4 = 0;          
  OS_Init();           // initialize, disable interrupts
	
// Count2 + Count5 should equal Count1
   NumCreated = 0 ;  
//	OS_InitSemaphore(&toDisplay, 1);
  
  OS_AddSW1Task(&BackgroundThread5c,2);
//	NumCreated += OS_AddThread(&Thread_write,NumCreated, 0, 1);
//  NumCreated += OS_AddThread(&Thread_display,NumCreated, 0, 1);
	
  NumCreated += OS_AddThread(&Thread2c, 0, 1);
  NumCreated += OS_AddThread(&Thread3c, 0, 1);
  NumCreated += OS_AddThread(&Thread4c, 0, 1);				// Is this supposed to be added to the TCB? -CH
	
//	OS_Wait(&Readyc);	
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
}

//*******************Fourth TEST**********
// Once the third test runs, run this example (Lab 1 part 2)
// Count1 should exactly equal Count2
// Count3 should be very large
// Count4 increases by 640 every time select is pressed
// NumCreated increase by 1 every time select is pressed

// no UART interrupts
// SYSTICK interrupts, with or without period established by OS_Launch
// Timer interrupts, with or without period established by OS_AddPeriodicThread
// Select switch interrupts, active low
// no ADC serial port or LCD output
// tests the spinlock semaphores, tests Sleep and Kill
Sema4Type Readyd;        // set in background
void BackgroundThread1d(void){   // called at 1000 Hz
static int i=0;	int idx; tcbType *tempSleep; 
  i++;
  if(i==50){
    i = 0;         //every 50 ms
    Count1++;
    OS_bSignal(&Readyd);
  }
	//Iterate through all the sleeping threads and decrement the counter
	for(idx = 0; idx < NumSleeping; idx++){
		SleepPt->sleepState -= 1; 
		if (!(SleepPt->sleepState)) {
			//wake up this thread
			NumSleeping--; 
			tempSleep = SleepPt->nextThread;
			SleepPt->prevThread = RunPt; 
			SleepPt->nextThread = RunPt->nextThread; 
			RunPt->nextThread->prevThread = SleepPt; 
			RunPt->nextThread = SleepPt;
			
			SleepPt = (NumSleeping) ? tempSleep : 0; 
		}
	}
}
void Thread2d(void){
  OS_InitSemaphore(&Readyd,0);
  Count1 = 0;          
  Count2 = 0;          
  for(;;){
    OS_bWait(&Readyd);
    Count2++;     
  }
}
void Thread3d(void){
  Count3 = 0;          
  for(;;){
    Count3++;
  }
}
void Thread4d(void){ int i;
  for(i=0;i<640;i++){
    Count4++;
    OS_Sleep(1);  // OS_Sleep(1);
  }
  OS_Kill();
}
void BackgroundThread5d(void){   // called when Select button pushed
  NumCreated += OS_AddThread(&Thread4d, 0,3); 
}
int Testmain4 (void){   // Testmain4
  Count4 = 0;          
  OS_Init();           // initialize, disable interrupts
  NumCreated = 0 ;
  OS_AddPeriodicThread(&BackgroundThread1d,TIME_1MS,0); //PERIOD
  NumCreated += OS_AddThread(&Thread2d, 0,2); 
  NumCreated += OS_AddThread(&Thread3d, 0,3); 
  NumCreated += OS_AddThread(&Thread4d, 0,3); 
	OS_AddSW1Task(&BackgroundThread5d,2);
  OS_Launch(TIME_2MS); // doesn't return, interrupts enabled in here
  return 0;            // this never executes
}

//******************* Lab 3 Preparation 2**********
// Modify this so it runs with your RTOS (i.e., fix the time units to match your OS)
// run this with 
// UART0, 115200 baud rate, used to output results 
// SYSTICK interrupts, period established by OS_Launch
// first timer interrupts, period established by first call to OS_AddPeriodicThread
// second timer interrupts, period established by second call to OS_AddPeriodicThread
// SW1 no interrupts
// SW2 no interrupts
unsigned long CountA;   // number of times Task A called
unsigned long CountB;   // number of times Task B called
unsigned long Count1;   // number of times thread1 loops


//*******PseudoWork*************
// simple time delay, simulates user program doing real work
// Input: amount of work in 100ns units (free free to change units
// Output: none
void PseudoWork(unsigned short work){
unsigned short startTime;
  startTime = OS_Time();    // time in 100ns units
  while(OS_TimeDifference(startTime,OS_Time()) <= work){} 
}
void Thread6(void){  // foreground thread
  Count1 = 0;          
  for(;;){
    Count1++; 
    PE0 ^= 0x01;        // debugging toggle bit 0  
  }
}
void Jitter(void) {   // prints jitter information (write this)
	unsigned long static LastTime; 
  unsigned long	thisTime;
	unsigned long jitter; 
	unsigned long static Filterwork = 0; 
	unsigned long diff = OS_TimeDifference(LastTime,thisTime);
	char buffer[40]; 
	thisTime = OS_Time();
	Filterwork++; 
 if (Filterwork > 1) { 	
	if(diff>PERIOD){
		jitter = (diff-PERIOD+4)/8;  // in 0.1 usec
	}else{
		jitter = (PERIOD-diff+4)/8;  // in 0.1 usec
	}
	if(jitter > MaxJitter){
		MaxJitter = jitter; // in usec
	}       // jitter should be 0
	if(jitter >= JitterSize){
		jitter = JITTERSIZE-1;
	}
	JitterHistogram[jitter]++; 
 }
 LastTime = OS_Time(); 
 sprintf(buffer, "Jitter 0.1us = %lu", MaxJitter); 
 UART_OutString(buffer);
 UART_OutString("\n\r"); 
}
void Thread7(void){  // foreground thread
	char buffer[40]; 
  UART_OutString("\n\rEE345M/EE380L, Lab 3 Preparation 2\n\r");
  OS_Sleep(10000);   // 10 seconds        
  //Jitter();         // print jitter information
	
	sprintf(buffer, "Jitter TimerA 0.1us = %lu", MaxJitter); 
	UART_OutString(buffer);
	UART_OutString("\n\r");
	
  UART_OutString("\n\r\n\r");
  OS_Kill();
}
#define workA 500       // {5,50,500 us} work in Task A
#define counts1us 10    // number of OS_Time counts per 1us
void TaskA(void){       // called every {1000, 2990us} in background
	static int i =0;
	if(i == 200){
//		PF1 ^= 0x02;
		i = 0;
	}
	i++;
  PE1 = 0x02;      // debugging profile  
  CountA++;
  PseudoWork(workA*counts1us); //  do work (100ns time resolution)
  PE1 = 0x00;      // debugging profile  
	
}
#define workB 250       // 250 us work in Task B
void TaskB(void){       // called every pB in background
  PE2 = 0x04;      // debugging profile  
  CountB++;
  PseudoWork(workB*counts1us); //  do work (100ns time resolution)
  PE2 = 0x00;      // debugging profile  
}


int testmain5 (void){       // Testmain5 Lab 3
	OS_Init();           // initialize, disable interrupts
  PortE_Init();
	PortF_Init();
	UART_Init();
  NumCreated = 0 ;
  NumCreated += OS_AddThread(&Thread6,0,2); 
  NumCreated += OS_AddThread(&Thread7,0, 1); 
  OS_AddPeriodicThread(&TaskA,TIME_1MS,1);           // 1 ms, higher priority
  OS_AddPeriodicThread(&TaskB,2*TIME_1MS,2);         // 2 ms, lower priority
	OS_AddSW1Task(&SW1Push, 4);
	OS_AddSW2Task(&SW2Push, 4);

  OS_Launch(TIME_2MS); // 2ms, doesn't return, interrupts enabled in here
  return 0;             // this never executes
}


//******************* Lab 3 Preparation 4**********
// Modify this so it runs with your RTOS used to test blocking semaphores
// run this with 
// UART0, 115200 baud rate,  used to output results 
// SYSTICK interrupts, period established by OS_Launch
// first timer interrupts, period established by first call to OS_AddPeriodicThread
// second timer interrupts, period established by second call to OS_AddPeriodicThread
// SW1 no interrupts, 
// SW2 no interrupts
Sema4Type s;            // test of this counting semaphore
unsigned long SignalCount1;   // number of times s is signaled
unsigned long SignalCount2;   // number of times s is signaled
unsigned long SignalCount3;   // number of times s is signaled
unsigned long WaitCount1;     // number of times s is successfully waited on
unsigned long WaitCount2;     // number of times s is successfully waited on
unsigned long WaitCount3;     // number of times s is successfully waited on
#define MAXCOUNT 20000
void OutputThread(void){  // foreground thread
  UART_OutString("\n\rEE345M/EE380L, Lab 3 Preparation 4\n\r");
  while(SignalCount1+SignalCount2+SignalCount3<100*MAXCOUNT){//*100
    OS_Sleep(1000);   // 1 second
    UART_OutString(".");
  }       
  UART_OutString(" done\n\r");
  UART_OutString("Signalled="); UART_OutUDec(SignalCount1+SignalCount2+SignalCount3);
  UART_OutString(", Waited="); UART_OutUDec(WaitCount1+WaitCount2+WaitCount3);
  UART_OutString("\n\r");
  OS_Kill();
}
void Wait1(void){  // foreground thread
  for(;;){
    OS_bWait(&s);    // three threads waiting
    WaitCount1++; 
  }
}
void Wait2(void){  // foreground thread
  for(;;){
    OS_bWait(&s);    // three threads waiting
    WaitCount2++; 
  }
}
void Wait3(void){   // foreground thread
  for(;;){
    OS_bWait(&s);    // three threads waiting
    WaitCount3++; 
  }
}
void Signal1(void){      // called every 799us in background
  if(SignalCount1<MAXCOUNT){
		if(s.Value <0){
			OS_bSignal(&s);
		}
    SignalCount1++;		
  }
}
// edit this so it changes the periodic rate
void Signal2(void){       // called every 1111us in background
  if(SignalCount2<MAXCOUNT){
    OS_bSignal(&s);
    SignalCount2++;
  }
}
void Signal3(void){       // foreground
  while(SignalCount3<98*MAXCOUNT){
    OS_bSignal(&s);
    SignalCount3++;
  }
  OS_Kill();
}

long add(const long n, const long m){
static long result;
  result = m+n;
  return result;
}
int testmain6 (void){      // Testmain6  Lab 3
  volatile unsigned long delay;
  OS_Init();           // initialize, disable interrupts
  delay = add(3,4);
	UART_Init(); 
  PortE_Init();
	PortF_Init();
	Count1 = 0;
  SignalCount1 = 0;   // number of times s is signaled
  SignalCount2 = 0;   // number of times s is signaled
  SignalCount3 = 0;   // number of times s is signaleds
  WaitCount1 = 0;     // number of times s is successfully waited on
  WaitCount2 = 0;     // number of times s is successfully waited on
  WaitCount3 = 0;	  // number of times s is successfully waited on
  OS_InitSemaphore(&s,0);	 // this is the test semaphore
  OS_AddPeriodicThread(&Signal1,(799*TIME_1MS)/1000,0); //should be 0  // 0.799 ms, higher priority
	OS_AddPeriodicThread(&Signal2,(1111*TIME_1MS)/1000,1);  // 1.111 ms, lower priority
  NumCreated = 0 ;
  NumCreated += OS_AddThread(&Thread6,0,6);    	// idle thread to keep from crashing
  NumCreated += OS_AddThread(&OutputThread,0,2); 	// results output thread
  NumCreated += OS_AddThread(&Signal3,0,2); 	// signalling thread
  NumCreated += OS_AddThread(&Wait1,0,2); 	// waiting thread
  NumCreated += OS_AddThread(&Wait2,0,2); 	// waiting thread
  NumCreated += OS_AddThread(&Wait3,0,2); 	// waiting thread
 
  OS_Launch(TIME_1MS);  // 1ms, doesn't return, interrupts enabled in here
  return 0;             // this never executes
}
Sema4Type dummy; 

void dummyThread1 (void) {
	int i; 
	for (i = 0; i< 100000; i++){
		OS_Wait(&dummy); 
	}
	OS_Kill();
}

//void dummyThread2 (void) {
//	int i; 
//	for (i = 0; i< 100000; i++){
//		OS_Signal(&dummy); 
//		OS_Suspend();
//	}
//	OS_Kill();
//}

void dummyThread3(void){
	for (;;){
	//	OS_Wait(&dummy); 
		Count1++;
	}
	OS_Kill();
}

void dummyInt(void){
	static int counter =0;
	if(dummy.Value <0){
	OS_Signal(&dummy);
	}
	counter ++;
}

void switchWork1(void){
	while(switchFlag == 0){
		// check for another button push
		//if Sw1 is pushed
			// output fifo data to graph
	}
	OS_Kill();
}

static int idxX = 0; 
void switchWork2 (unsigned long data) {
//	static int points[128] = {-1};
//	static int i, j =0;
	long pixelPos; 
	static long i;
	OS_DisableInterrupts(); 
	ADCFifo_Get(&x[i]);
	i=(i+1) % 64;
	if((switchFlag == 1) && (!fftActive)){
		if (filterFlag == 0) { 
			pixelPos = 148 - ((data* 366)/10000) ;  
		} else {
				pixelPos = (((Filter_Calc(data))* 366)/10000)  ;   
		} 
		ST7735_PlotBar(127);  // clip large magnitudes
		ST7735_PlotPoint(pixelPos); // called N times
		ST7735_PlotNext();
}
	OS_EnableInterrupts();

}

 void fft (void) {
	 int DCcomponent, ImComp, i, status, pixelPos; 
	 while(fftActive){
		status = StartCritical ();
		cr4_fft_64_stm32(y,x,64);  // complex FFT of last 64 ADC values
	  ST7735_PlotClear(0,127);  // clip large magnitudes
			
		for (i = 0; i < 64; i++) {  
			DCcomponent = (y[i]&0xFFFF); // Real part at frequency 0, imaginary part should be zero
			ImComp = (y[i] & 0xFFFF0000) >> 15; 
			y[i] = sqrt(DCcomponent*DCcomponent + ImComp*ImComp); 
			DCcomponent = y[i]; 
			//DCcomponent = (DCcomponent < 0) ? -DCcomponent : DCcomponent; 
			//pixelPos = 148 - ((DCcomponent* 366)/10000) ;
			pixelPos = (DCcomponent/5); 
			ST7735_PlotBar(pixelPos);  // clip large magnitudes
			//ST7735_PlotPoint(pixelPos); // called 4 times
			ST7735_PlotNext();
	 } 
	 EndCritical(status); 
 }
	OS_Kill();
 }
int main (void) { //Prachi's main
	OS_Init();           // initialize, disable interrupts
 	PortE_Init(); 
	PortF_Init();
	ST7735_InitR(INITR_REDTAB);
	ST7735_FillScreen(0);
	ST7735_PlotClear(0,127);
	OS_AddSW1Task(&SW1Push,1);
  OS_AddSW2Task(&SW2Push,2);
	OS_InitSemaphore(&toDisplay, 1);
	ADC_Init(4); 
	ADC_Collect(4, 12000, &switchWork2);
	OS_AddThread(&Interpreter, 0, 0);
	OS_AddThread(&dummyThread3, 0, 7);
	OS_EnableInterrupts(); 
	OS_Launch(TIME_2MS);

	return 0;
}

