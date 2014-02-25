// *********************** adc.c **************************
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
#include "lm4f120h5qr.h"
#include "adc.h"

#define TOUCH_YN       (*((volatile unsigned long *)0x40004010))     // PA2
#define TOUCH_XP       (*((volatile unsigned long *)0x40004020))     // PA3
#define TOUCH_XN       (*((volatile unsigned long *)0x40024040))     // PE4 / AIN9
#define TOUCH_YP       (*((volatile unsigned long *)0x40024080))     // PE5 / AIN8

#define PA2     0x04
#define PA3     0x08
#define PE4     0x10
#define PE5     0x20

#define NUM_SAMPLES 4
#define NUM_VALS_TO_AVG 8

#define XVAL_MIN    100
#define YVAL_MIN    150

unsigned char Touch_WaitForInput = 0;
short Touch_XVal;
short Touch_YVal;


void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts

//short xVals[NUM_VALS_TO_AVG];
//short yVals[NUM_VALS_TO_AVG];
//unsigned char filterCounter = 0;
//unsigned char numVals = 0;


//         YP / PE5 / AIN8
//      ---------------------
//      |                   |    
//   XP |                   | XN  
//  PA3 |                   | PE4
//      |                   | AIN9
//      |                   |
//      |                   |
//      ---------------------
//             YN / PA2




// ************** Touch_Init *******************************
// - Initializes the GPIO used for the touchpad
// *********************************************************
// Input: none
// Output: none
// *********************************************************
void Touch_Init(void){
    unsigned long wait = 0;    

    // Activate PORTA GPIO clock
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA;
    wait++;
    wait++;

//    // Configure PA2/PA3 for GPIO digital output
    GPIO_PORTA_DIR_R    |=  0x0C;
//    GPIO_PORTA_AFSEL_R  &= ~0x0C;
//    GPIO_PORTA_DEN_R    |=  0x0C;   
//    
//    // Set XN and YN low
//    TOUCH_XN = 0x00;
//    TOUCH_YN = 0x00;
    
    // Activate PORTE GPIO clock
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOE;
    wait++;
    wait++;
    
//    // Configure PE4/PE5 for GPIO digital output
    GPIO_PORTE_DIR_R    |=  0x30;
//    GPIO_PORTE_AFSEL_R  &= ~0x30;
//    GPIO_PORTE_DEN_R    |=  0x30;
//    GPIO_PORTE_AMSEL_R  &= ~0x30;
//    
//    // Set XP and YP high
//    TOUCH_XP = 0xFF;
//    TOUCH_YP = 0xFF;
}

// ************** ADC_Init *********************************
// - Initializes the ADC to use a specficed channel on SS3
// *********************************************************
// Input: channel number
// Output: none
// *********************************************************
void ADC_Init(void){
    long wait = 0;
    
    // Set bit 0 in SYSCTL_RCGCADC_R to enable ADC0
    SYSCTL_RCGCADC_R    |=  0x01;
    for (wait = 0; wait < 50; wait++){}
        
    SYSCTL_RCGCGPIO_R |= 0x10;
    
    // Set ADC sample to 125KS/s
    ADC0_PC_R = 0x01;
  
    // Disable all sequencers for configuration
    ADC0_ACTSS_R &= ~0x000F;

    // Set ADC0 SS3 to highest priority
    ADC0_SSPRI_R = 0x0123;    
    
    // Set bits 12-15 to 0x00 to enable software trigger on SS3
    ADC0_EMUX_R &= ~0xF000;

    // Set sample channel for sequencer 3
    ADC0_SSMUX3_R &= 0xFFF0;    
    ADC0_SSMUX3_R += 9;

    // TS0 = 0, IE0 = 1, END0 = 1, D0 = 0
    ADC0_SSCTL3_R = 0x006;
    
    // Disable ADC interrupts on SS3 by clearing bit 3
    ADC0_IM_R &= ~0x0008;
    
    // Re-enable sample sequencer 3
    ADC0_ACTSS_R |= 0x0008;
}

// ************** ADC_Read *********************************
// - Takes a sample from the ADC
// *********************************************************
// Input: none
// Output: sampled value from the ADC
// *********************************************************
unsigned long ADC_Read(void){
    unsigned long result;
    
    // Set bit 3 to trigger sample start
    ADC0_PSSI_R = 0x008; 
    
    // Wait for SS3 RIS bit to be set to 1
    while((ADC0_RIS_R&0x08)==0){};
        
    // Read 12-bit result from ADC from FIFO
    result = ADC0_SSFIFO3_R&0xFFF;
        
    // Clear SS3 RIS bit to 0 to acknowledge completion
    ADC0_ISC_R = 0x0008;
    
    return result;
}

// ************** ADC_SetChannel ***************************
// - Configures the ADC to use a specific channel
// *********************************************************
// Input: none
// Output: none
// *********************************************************
void ADC_SetChannel(unsigned char channelNum){
    // Disable all sequencers for configuration
    ADC0_ACTSS_R &= ~0x000F;

    // Set sample channel for sequencer 3
    ADC0_SSMUX3_R &= 0xFFF0;    
    ADC0_SSMUX3_R += channelNum;

    // Re-enable sample sequencer 3
    ADC0_ACTSS_R |= 0x0008;    
}

// ************** ADC_ReadXVal *****************************
// - 
// *********************************************************
// Input: none
// Output: none
// *********************************************************
unsigned long Touch_ReadX(void){
    long i = 0;
    long sum = 0;
    long result = 0;
    
    GPIO_PORTA_DATA_R   &= ~PA2;

    // Configure PA3 (XP) for GPIO HIGH
    GPIO_PORTA_AFSEL_R  &= ~PA3;       // ASFEL = 0
    GPIO_PORTA_DEN_R    |=  PA3;       // DEN = 1
    GPIO_PORTA_DIR_R    |=  PA3;       // DIR = 1
    GPIO_PORTA_DATA_R   |=  PA3;       // DATA = 1
    
    // Configure PE4 (XN) for GPIO LOW
    GPIO_PORTE_AFSEL_R  &= ~PE4;       // ASFEL = 0
    GPIO_PORTE_DEN_R    |=  PE4;       // DEN = 1
    GPIO_PORTE_DIR_R    |=  PE4;       // DIR = 1
    GPIO_PORTE_DATA_R   &= ~PE4;       // DATA = 0
    
    // Configure PA2 (YN) for analog hi-Z
    GPIO_PORTA_AFSEL_R  &= ~PA2;       // AFSEL = 0
    GPIO_PORTA_DEN_R    &= ~PA2;       // DEN = 0
    GPIO_PORTA_DIR_R    &= ~PA2;       // DIR = 0
    
    // Configure PE5 (YP) for ADC use
    GPIO_PORTE_AFSEL_R  |=  PE5;       // AFSEL = 1
    GPIO_PORTE_DEN_R    &= ~PE5;       // DEN = 0
    GPIO_PORTE_AMSEL_R  |=  PE5;       // AMSEL = 1
   
    // Configure ADC to read from AIN8 (PE5, YP)
    ADC_SetChannel(8);
    
    // Take some samples to average
    for (i = 0; i < NUM_SAMPLES; i++){
        sum += ADC_Read();
    }

    GPIO_PORTE_AMSEL_R  &= ~PE5;       // AMSEL = 0
     
    // Compute average
    result = sum / NUM_SAMPLES;
    
    Touch_XVal = result;

    return result;
}

// ************** ADC_ReadYVal *****************************
// - 
// *********************************************************
// Input: none
// Output: none
// *********************************************************
unsigned long Touch_ReadY(void){
    long i = 0;
    long sum = 0;
    long result = 0;

    // Configure PE5 (YP) for GPIO HIGH
    GPIO_PORTE_AFSEL_R  &= ~PE5;       // ASFEL = 0
    GPIO_PORTE_DEN_R    |=  PE5;       // DEN = 1
    GPIO_PORTE_DIR_R    |=  PE5;       // DIR = 1
    GPIO_PORTE_DATA_R   |=  PE5;       // DATA = 1
    //TOUCH_XP = 0xFF;
    
    // Configure PA2 (YN) for GPIO LOW
    GPIO_PORTA_AFSEL_R  &= ~PA2;       // ASFEL = 0
    GPIO_PORTA_DEN_R    |=  PA2;       // DEN = 1
    GPIO_PORTA_DIR_R    |=  PA2;       // DIR = 1
    GPIO_PORTA_DATA_R   &= ~PA2;       // DATA = 0
    
    // Configure PA3 (XP) for analog hi-Z
    GPIO_PORTA_AFSEL_R  &= ~PA3;       // AFSEL = 0
    GPIO_PORTA_DEN_R    &= ~PA3;       // DEN = 0
    GPIO_PORTA_DIR_R    &= ~PA3;       // DIR = 0
    
    // Configure PE4 (XN) for ADC use
    GPIO_PORTE_AFSEL_R  |=  PE4;       // AFSEL = 1
    GPIO_PORTE_DEN_R    &= ~PE4;       // DEN = 0
    GPIO_PORTE_AMSEL_R  |=  PE4;       // AMSEL = 1
   
    // Configure ADC to read from AIN9 (PE4, XN)
    ADC_SetChannel(9);
    
    // Take some samples to average
    for (i = 0; i < NUM_SAMPLES; i++){
        sum += ADC_Read();
    }
    
    // Compute average
    result = sum / NUM_SAMPLES;
    
    Touch_YVal = result;

    return result;
}

// ************** ADC_ReadZ1 *******************************
// - 
// *********************************************************
// Input: none
// Output: none
// *********************************************************
unsigned long Touch_ReadZ1(void){
    long i = 0;
    long sum = 0;
    long result = 0;

    // Configure PA2 (YN) for GPIO HIGH
    GPIO_PORTA_AFSEL_R  &= ~PA2;       // ASFEL = 0
    GPIO_PORTA_DEN_R    |=  PA2;       // DEN = 1
    GPIO_PORTA_DIR_R    |=  PA2;       // DIR = 1
    GPIO_PORTA_DATA_R   |=  PA2;       // DATA = 1
    
    // Configure PA3 (XP) for GPIO LOW
    GPIO_PORTA_AFSEL_R  &= ~PA3;       // ASFEL = 0
    GPIO_PORTA_DEN_R    |=  PA3;       // DEN = 1
    GPIO_PORTA_DIR_R    |=  PA3;       // DIR = 1
    GPIO_PORTA_DATA_R   &= ~PA3;       // DATA = 0
    
    // Configure PE5 (YP) for analog hi-Z
    GPIO_PORTE_AFSEL_R  &= ~PE5;       // AFSEL = 0
    GPIO_PORTE_DEN_R    &= ~PE5;       // DEN = 0
    GPIO_PORTE_DIR_R    &= ~PE5;       // DIR = 0
    
    // Configure PE4 (XN) for ADC use
    GPIO_PORTE_AMSEL_R  |=  PE4;       // AMSEL = 1
    GPIO_PORTE_AFSEL_R  |=  PE4;       // AFSEL = 1
    GPIO_PORTE_DEN_R    &= ~PE4;       // DEN = 0
    //GPIO_PORTE_DIR_R    &= ~PE4;       // DIR = 0    

    // Configure ADC to read from AIN9 (PE4, XN)
    ADC_SetChannel(9);
    
    // Take some samples to average
    for (i = 0; i < NUM_SAMPLES; i++){
        sum += ADC_Read();
    }
        
    // Compute average
    result = sum / NUM_SAMPLES;

    return result;
}

// ************** ADC_ReadZ1 *******************************
// - 
// *********************************************************
// Input: none
// Output: none
// *********************************************************
unsigned long Touch_ReadZ2(void){
    long i = 0;
    long sum = 0;
    long result = 0;

    // Configure PA2 (YN) for GPIO HIGH
    GPIO_PORTA_AFSEL_R  &= ~PA2;       // ASFEL = 0
    GPIO_PORTA_DEN_R    |=  PA2;       // DEN = 1
    GPIO_PORTA_DIR_R    |=  PA2;       // DIR = 1
    GPIO_PORTA_DATA_R   |=  PA2;       // DATA = 1
    
    // Configure PA3 (XP) for GPIO LOW
    GPIO_PORTA_AFSEL_R  &= ~PA3;       // ASFEL = 0
    GPIO_PORTA_DEN_R    |=  PA3;       // DEN = 1
    GPIO_PORTA_DIR_R    |=  PA3;       // DIR = 1
    GPIO_PORTA_DATA_R   &= ~PA3;       // DATA = 0
    
    // Configure PE4 (XN) for analog hi-Z
    GPIO_PORTE_AFSEL_R  &= ~PE4;       // AFSEL = 0
    GPIO_PORTE_DEN_R    &= ~PE4;       // DEN = 0
    GPIO_PORTE_DIR_R    &= ~PE4;       // DIR = 0
    GPIO_PORTE_AMSEL_R  &= ~PE4;       // AMSEL = 0
    
    // Configure PE5 (YP) for ADC use
    GPIO_PORTE_AFSEL_R  |=  PE5;       // AFSEL = 1
    GPIO_PORTE_DEN_R    &= ~PE5;       // DEN = 0
    GPIO_PORTE_DIR_R    &= ~PE5;       // DIR = 0
    GPIO_PORTE_AMSEL_R  |=  PE5;       // AMSEL = 1
   
    // Configure ADC to read from AIN9 (PE5, YP)
    ADC_SetChannel(8);
    
    // Take some samples to average
    for (i = 0; i < NUM_SAMPLES; i++){
        sum += ADC_Read();
    }
    
    // Compute average
    result = sum / NUM_SAMPLES;

    return result;
}

//coord Touch_GetCoords(void){
//    coord result;
//    short sumX = 0;
//    short sumY = 0;
//    short i = 0;

//    xVals[filterCounter] = Touch_ReadXVal();
//    yVals[filterCounter] = Touch_ReadYVal();
//    
//    if (numVals < NUM_VALS_TO_AVG) 
//        numVals++;
//    
//    filterCounter++;
//    if (filterCounter >= NUM_VALS_TO_AVG) 
//        filterCounter = 0;
//    
//    for (i = 0; i < numVals; i++){
//        sumX += xVals[i];
//        sumY += yVals[i];
//    }
//    
//    result.x = sumX / numVals;
//    result.y = sumY / numVals;
//    
//    return result;
//}


void Touch_BeginWaitForTouch(void){
    // XP = 1  XN = DIG IN, INT ON FALL EDGE YP = Hi-Z YN = 0
    DisableInterrupts();
    
    // Set XP high
    TOUCH_XP = 0xFF;
    
    // Set YN low
    TOUCH_YN = 0x00;
    
    // Configure XN (PA3) for digital input
    GPIO_PORTA_DIR_R &= ~0x08;
    
    // Configure YP (PE5) for analog Hi-Z
    GPIO_PORTE_DIR_R &= ~0x20;
    GPIO_PORTE_DEN_R &= ~0x20;
   
    // Setup falling edge interrupt on XN (PA3)
    GPIO_PORTA_PUR_R |=  0x08;   // enable weak pull up
    GPIO_PORTA_IS_R  &= ~0x08;     // (d) PF4 is edge-sensitive
    GPIO_PORTA_IBE_R &= ~0x08;    //     PF4 is not both edges
    GPIO_PORTA_IEV_R &= ~0x08;    //     PF4 falling edge event
    GPIO_PORTA_ICR_R  =  0x08;      // (e) clear flag4
    GPIO_PORTA_IM_R  |=  0x08;      // (f) arm interrupt on PF4


    NVIC_PRI0_R = (NVIC_PRI7_R&0xFFFFFF00)|0x000000a0;
    NVIC_EN0_R = NVIC_EN0_INT0;  // (h) enable interrupt 30 in NVIC


    EnableInterrupts();
}

long Touch_GetCoords(void){
    long result, temp, xPos, yPos;
    
    long cal[7] = {
     	280448,	//   86784,          // M0
        -3200,	// -1536,          // M1
        -220093760,	//-17357952,      // M2
        -3096,		//-144,           // M3
        -275592,		//-78576,         // M4
        866602824,	// 69995856,       // M5
        2287498		//201804,         // M6
    };
    
    xPos = Touch_XVal;
    yPos = Touch_YVal;
    
    temp = (((xPos * cal[0]) + (yPos * cal[1]) + cal[2]) / cal[6]);
    yPos =(((xPos * cal[3]) + (yPos * cal[4]) + cal[5]) / cal[6]);
    xPos = temp;
    
    result = xPos << 16;
    result += yPos;
    
    return result;
}



