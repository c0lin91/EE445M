#ifndef FILTER_H
#define FILTER_H
int Filter_FIR(void);

void Filter_Init (void); 
short Filter_Calc (short newData);
#endif
