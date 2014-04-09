#include "OS.h"
#include "ADC.h"
#include "GPIO.h"
#include "MACQ.h"
#include "can0.h"
#include "inc/tm4c123gh6pm.h"
#include "PING.h"
#include "PWM.h"
#include <Math.h>

#define NUMPIXELS 51
#define FIRLENGTH 51

#define SPEED_OF_SOUND_IN 13397.2441 //in/s
#define SPEED_OF_SOUND_CM 34029.0 //cm/s
//#define SPEED_OF_SOUND_MM 340290 //mm/s

#define NANO 50000000 

#define PF0       (*((volatile unsigned long *)0x40025004))
#define PF1       (*((volatile unsigned long *)0x40025008))
#define PF2       (*((volatile unsigned long *)0x40025010))
#define PF3       (*((volatile unsigned long *)0x40025020))
#define PF4       (*((volatile unsigned long *)0x40025040))

unsigned long dataLost = 0;
unsigned long numberOfSamplesTaken = 0;
unsigned long DoDigitalFilter = 0;
unsigned long ScopeDisplay = 0;
unsigned long RcvCount=0;
unsigned char sequenceNum=0;  
unsigned char XmtData[8];
unsigned char RcvData[8];
long samples[64];
long results[FIRLENGTH];
long buffer[64];




void cr4_fft_64_stm32(void *pssOUT, void *pssIN, unsigned short Nbin);


// Newton's method
// s is an integer
// sqrt(s) is an integer
unsigned long square_rt(unsigned long s){
unsigned long t;         // t*t will become s
int n;                   // loop counter to make sure it stops running
  t = s/10+1;            // initial guess 
  for(n = 16; n; --n){   // guaranteed to finish
    t = ((t*t+s)/t)/2;  
  }
  return t; 
}


long FFTresults[128];
void FFT(void){
	//cr4_fft_64_stm32(FFTresults,samples,64);
	//cr4_fft_64_stm32(FFTresults,results,64);
	cr4_fft_64_stm32(FFTresults,buffer,64);
}				

/*
const long h[FIRLENGTH]={4,-1,-8,-14,-16,-10,-1,6,5,-3,-13,
     -15,-8,3,5,-5,-20,-25,-8,25,46,26,-49,-159,-257,
     984,-257,-159,-49,26,46,25,-8,-25,-20,-5,5,3,-8,
     -15,-13,-3,5,6,-1,-10,-16,-14,-8,-1,4};
*/
const long h[51]={5,5,5,5,5,5,5,5,5,5,5,
     5,5,5,5,5,5,5,5,5,5,5,5,5,5,
     5,5,5,5,5,5,5,5,5,5,5,5,5,5,
     5,5,5,5,5,5,5,5,5,5,5,5};

//--- filter 
void filter(void){
	int i, n;
	long result;
	result = 0;
	n = FIRLENGTH-1;

	//result = (samples[46] + samples[47] + samples[48] + samples[49] + samples[50])/5;
	
	for (i = 0; i <= n; i++){ //convolution
		result += h[i]*samples[n-i];
		results[i] = result;
	}
	result = (result/256);
	
	results[0] = result;
	
	for (i = 0; i < 63; i++){
		buffer[i + 1] = buffer[i];
	}
	buffer[0] = result;
}



//--- Producer
void Producer(unsigned long ADCdata){  
	if(OS_Fifo_Put(ADCdata) == 0){ // send to consumer
		dataLost++;
	}	
}

void Consumer(void){ 
	unsigned long consumerData;
	double displayData;
	int i = 0;
	
  ADC_Collect(0, 2, 0x5, 1, 12800, &Producer); // start ADC sampling, channel 5, PD2, 12800 Hz
	while(1){
		for(i = 0;i < 64; i++){
			samples[i] = OS_Fifo_Get();
			consumerData = samples[50];    // get from producer				
		}

		filter();
		FFT();
		displayData = results[0];
		displayData = (pow(displayData, -1.181)) * (120760);
		OS_MailBox_Send(displayData); // called every 2.5ms*64 = 160ms
	}
}

\
//--- Consumer
void Display(void); 

//--- Display 
void Display(void){
   unsigned long data;

	while(1){
    	data = OS_MailBox_Recv();
    	OS_DisplayMessage(0,4,"Distance Sensor =",data); 
    }    
} 

void ButtonWork(void){
	static short count = 0;

	count++;
	DoDigitalFilter = ~(DoDigitalFilter)&0x01;
	OS_DisplayMessage(0,0,"Button Pushes  = ",count); 
	OS_DisplayMessage(0,1,"Dig ital Filter = ",DoDigitalFilter);
	OS_Kill();  // done, OS does not return from a Kill
}

void ButtonWork2(void){
	ScopeDisplay = ~(ScopeDisplay)&0x01;
	OS_PlotClear(0, 4095);
	OS_Kill();  // done, OS does not return from a Kill
}

#define PlotSize 128
typedef unsigned long PlotMacqType;
AddMacq(Plot, PlotSize, PlotMacqType)

void Oscilloscope(void){
	static long xCount, xCount2 = 0;
	unsigned long data;
	OS_PlotClear(0, 4095);
	
	while(1){	
		
		if(ScopeDisplay == 0){
			data = OS_MailBox_Recv();	
			OS_PlotPoint(data);
			OS_PlotNext();
			OS_DisplayMessage(0,3, "IR: ", data); 
			xCount++;
			if(xCount > 127){
				xCount = 0; 
				OS_PlotClear(0, 4095);
			}
			OS_Sleep(20);
		}
		else{
	
			OS_PlotBar(square_rt(FFTresults[xCount2]*FFTresults[xCount2])&0x2FF);
			OS_PlotNext();
			xCount2++;
			if(xCount2 > 63){
				xCount2 = 0; 
				//OS_Sleep(20);
				OS_PlotClear(0, 1023);
			}
		} 
		
	}
}

void CAN_Rx(void){
	 while(1){
    if(CAN0_GetMailNonBlock(RcvData)){
      RcvCount++;
      PF1 = RcvData[0];
      PF2 = RcvData[1];
      PF3 = RcvCount;   // heartbeat
    }
		OS_Suspend();
	//	OS_Sleep(TIME_1MS);
  }
	OS_Kill();
}	

void CAN_Tx(void){
  XmtData[0] = PF0<<1;  // 0 or 2
  XmtData[1] = PF4>>2;  // 0 or 4
  XmtData[2] = 0;       // ununsigned field
  XmtData[3] = sequenceNum;  // sequence count
  CAN0_SendData(XmtData);
  sequenceNum++;
}

void PortF_Init(void){
	int delay;
 SYSCTL_RCGC2_R |= 0x00000020;          // activate port F
  delay = SYSCTL_RCGC2_R;          // allow time to finish activating
  GPIO_PORTF_LOCK_R = 0x4C4F434B;  // unlock GPIO Port F
  GPIO_PORTF_CR_R = 0xFF;          // allow changes to PF4-0
  GPIO_PORTF_DIR_R = 0x0E;         // make PF3-1 output (PF3-1 built-in LEDs)
  GPIO_PORTF_AFSEL_R = 0;          // disable alt funct 
  GPIO_PORTF_DEN_R = 0x1F;         // enable pullup on inputs
  GPIO_PORTF_PUR_R = 0x11;         // enable digital I/O on PF4-0
  GPIO_PORTF_PCTL_R = 0x00000000;
  GPIO_PORTF_AMSEL_R = 0;          // disable analog functionality on PF
}

void PingProcessTime(void){
	double time1, time2, time3;
	OS_DisplayMessage(0, 0,"time (20ns): ", PING_Time);
	time1 = ((PING_Time*SPEED_OF_SOUND_IN)/NANO)/2;
	time2 = ((PING_Time*SPEED_OF_SOUND_CM)/NANO)/2;
//	time3 = ((PING_Time*SPEED_OF_SOUND_MM)/NANO)/2;

	OS_DisplayMessage(0, 1,"distance (in): ",time1);
  OS_DisplayMessage(0, 2,"distance (cm): ",time2);
//	OS_DisplayMessage(0, 3,"distance (mm): ",time3);	

	OS_Kill();
}


// ---------------------------- main ---------------------------------
int main(void){
/*
-implement killing even if foreground thread does not call kill
- add heartbeat  
- in DecrementSleep over underindexing can occur if more than 1 threads should be unlinked at the same time (change sleepst from unsigned to signed?)	
	*/
	OS_Init();
	OS_MailBox_Init();
  OS_Fifo_Init(16);
	PortF_Init();
	
	PING_Init(&PingProcessTime);
	OS_AddPeriodicThread(&PING_Start, 200, 1);
	
	CAN0_Open();	
	OS_AddThread(&CAN_Rx,128,1);
	OS_AddPeriodicThread(&CAN_Tx, 200, 2); // Transmit data over can every 100ms
	
	OS_AddThread(&Consumer,128,1);
  OS_AddThread(&Oscilloscope,128,1);
	
  OS_Launch(TIME_2MS);
	return 0;
}
