#include "OS.h"
#include "ADC.h"
#include "GPIO.h"
#include "MACQ.h"


#define NUMPIXELS 51
#define FIRLENGTH 51

unsigned long dataLost = 0;
unsigned long numberOfSamplesTaken = 0;
unsigned long DoDigitalFilter = 0;
unsigned long ScopeDisplay = 0;
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
\
//--- Consumer
void Display(void); 
void Consumer(void){ 
	unsigned long consumerData, displayData;
	int i = 0;
	
  ADC_Collect(0, 2, 0x5, 1, 12800, &Producer); // start ADC sampling, channel 5, PD2, 12800 Hz
  //OS_AddThread(&Display,128,1); 

	while(1){
		for(i = 0;i < 64; i++){
			samples[i] = OS_Fifo_Get();
			consumerData = samples[50];    // get from producer				
		}

		filter();
		FFT();
		if(DoDigitalFilter == 1){
			displayData = results[0];
		}
		else{
			displayData = consumerData;
		}
		if(ScopeDisplay == 0){
			OS_MailBox_Send(displayData); // called every 2.5ms*64 = 160ms
		}
	}
}

//--- Display 
void Display(void){
   unsigned long data;

	while(1){
    	data = OS_MailBox_Recv();
    	OS_DisplayMessage(0,0,"Distance Sensor =",data); 
    }    
} 

void ButtonWork(void){
	static short count = 0;

	count++;
	DoDigitalFilter = ~(DoDigitalFilter)&0x01;
	OS_DisplayMessage(0,0,"Button Pushes  = ",count); 
	OS_DisplayMessage(0,1,"Digital Filter = ",DoDigitalFilter);
	OS_Kill();  // done, OS does not return from a Kill
}

void ButtonWork2(void){
	ScopeDisplay = ~(ScopeDisplay)&0x01;
	OS_PlotClear(0, 4095);
	OS_Kill();  // done, OS does not return from a Kill
}

//--- SW1Push
void SW1Push(void){
	OS_AddThread(&ButtonWork,100,2);
}
//--- SW2Push
void SW2Push(void){
	OS_AddThread(&ButtonWork2,100,2);
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
	
	OS_AddThread(&Consumer,128,2);
  OS_AddSWTask(&SW1Push,2,1); //thread,priority,button#
	OS_AddSWTask(&SW2Push,2,2); //thread,priority,button#
  OS_AddThread(&Oscilloscope,128,2);
	
  OS_Launch(TIME_2MS);
	return 0;
}
