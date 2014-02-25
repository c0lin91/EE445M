// *********************** adc.h **************************
// Written by:
// - Steven Prickett
// - YeonGi Jeon
// Date Created: 11/5/2012
// Last Modified: 11/12/2012
//
// Description:
// - Interfaces with the LM3S1968 onboard ADC.
// * Adapted from Valvano, Program 10.1, pg 359
// ********************************************************

#ifndef ADC_H
#define ADC_H

typedef struct {
    short x;
    short y;
} coord;

// ************** Touch_Init *******************************
// - Initializes the GPIO used for the touchpad
// *********************************************************
// Input: none
// Output: none
// *********************************************************
void Touch_Init(void);

// ************** ADC_Init *********************************
// - Initializes the ADC to use a specficed channel on SS3
// *********************************************************
// Input: channel number
// Output: none
// *********************************************************
void ADC_Init(void);

// ************** ADC_Read *********************************
// - Takes a sample from the ADC
// *********************************************************
// Input: none
// Output: sampled value from the ADC
// *********************************************************
unsigned long ADC_Read(void);

// ************** ADC_SetChannel ***************************
// - Configures the ADC to use a specific channel
// *********************************************************
// Input: none
// Output: none
// *********************************************************
void ADC_SetChannel(unsigned char channelNum);

// ************** ADC_ReadX ********************************
// - 
// *********************************************************
// Input: none
// Output: none
// *********************************************************
unsigned long Touch_ReadX(void);

// ************** ADC_ReadY ********************************
// - 
// *********************************************************
// Input: none
// Output: none
// *********************************************************
unsigned long Touch_ReadY(void);

// ************** ADC_ReadZ1 *******************************
// - 
// *********************************************************
// Input: none
// Output: none
// *********************************************************
unsigned long Touch_ReadZ1(void);

// ************** ADC_ReadZ2 ********************************
// - 
// *********************************************************
// Input: none
// Output: none
// *********************************************************
unsigned long Touch_ReadZ2(void);


long Touch_GetCoords(void);

void Touch_BeginWaitForTouch(void);

#endif
