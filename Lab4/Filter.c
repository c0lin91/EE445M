
#include "FIFO.h"
#define N 51

#define ADCFIFOSIZE 64 // must be a power of 2
#define ADCFIFOSUCCESS 1
#define ADCFIFOFAIL    0

AddIndexFifo(ADC,ADCFIFOSIZE,int, ADCFIFOSUCCESS,ADCFIFOFAIL)
 
int input  [N]; 
int output [N]; 
const short transferFunction[N]={4,-1,-8,-14,-16,-10,-1,6,5,-3,-13, 
 -15,-8,3,5,-5,-20,-25,-8,25,46,26,-49,-159,-257, 984, 
 -257,-159,-49,26,46,25,-8,-25,-20,-5,5,3,-8, 
 -15,-13,-3,5,6,-1,-10,-16,-14,-8,-1,4}; 


int *newestInput = &input[0]; 
int *newestOutput = &output[0]; 

short Data [102]; 
short *pt; //pointer to current

void Filter_Init (void) {
	pt = &Data[0]; 
} 

short Filter_Calc (short newData) { 
	int i; long sum; short *tempPt; 
	const short *apt;
  long status;	
	status = StartCritical();
	if (pt == &Data[0]) {
		pt = &Data[50]; 
	} else { 
		pt--; 
	}
	
	*pt = newData; 
	*(pt+51) = newData; 
	tempPt = pt; 
	apt = &transferFunction[0];
	sum = 0; 
	for (i = 51; i; i--) {
		sum += (*tempPt) * (*apt); 
		apt++; 
		tempPt++; 
	}
	EndCritical(status);
	return sum/256; 
}
int Filter_FIR(void) 
{  
	int tempOutput;
	int* tempInPtr; 
	int* tempOutPtr; 
	int i; 
	static int count = 0;

	ADCFifo_Get(newestInput); //may need some sort of semaphore for this ADC FIFO
//	*newestInput = 2024; 
	
	tempOutput = 0; 
	tempInPtr = newestInput; 
	tempOutPtr = newestOutput; 
	
	/*Fix newestPtrs for next iterations*/
	if (++newestOutput > &output[N])
		newestOutput = &output[0]; 
	if (++newestInput > &input[N]) 
		newestInput = &input[0]; 
	
	for (i = 0; i < N; i++) {
		tempOutput += *tempInPtr * transferFunction[i]; 
		tempInPtr--;
		if (tempInPtr < &input[0])
				tempInPtr = &input[N]; 
	}
	*tempOutPtr = tempOutput/256; 
	if (*tempOutPtr < 0) { 
		return -1*(*tempOutPtr); 
	} else { 
		return *tempOutPtr; 
	} 
}

