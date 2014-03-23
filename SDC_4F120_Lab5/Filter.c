#include "Filter.h"
#define N 51

int StartCritical(void); 
void EndCritical(int); 

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


