
#include "FIFO.h"
#define N 64

#define ADCFIFOSIZE 64 // must be a power of 2
#define ADCFIFOSUCCESS 1
#define ADCFIFOFAIL    0

AddIndexFifo(ADC,ADCFIFOSIZE,int, ADCFIFOSUCCESS,ADCFIFOFAIL)
 
int input  [N]; 
int output [N]; 
int transferFunction [N]; 

int *newestInput = &input[0]; 
int *newestOutput = &output[0]; 

void Filter_FIR(void) 
{  
	int tempOutput;
	int* tempInPtr; 
	int* tempOutPtr; 
	int i; 
	ADCFifo_Get(newestInput); //may need some sort of semaphore for this ADC FIFO
	
	tempOutput = 0; 
	tempInPtr = newestInput; 
	tempOutPtr = newestOutput; 
	
	/*Fix newestPtrs for next iterations*/
	if (++newestOutput > &output[N])
		newestOutput = &output[0]; 
	if (++newestInput > &input[N]) 
		newestInput = &input[0]; 
	
	for (i = 0; i < N; i++) {
		tempOutput += *tempInPtr-- * transferFunction[i]; 
		if (tempInPtr < &input[0])
				tempInPtr = &input[N]; 
	}
	*tempOutPtr = tempOutput; 
}

