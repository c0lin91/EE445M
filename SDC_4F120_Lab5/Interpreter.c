
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "Interpreter.h"
#include "ADC.h"
#include "UART2.h"
#include "FIFO.h"
#include "OS.h"
#include "lm4f120h5qr.h"
#include "Filter.h" 
#include "math.h"
#include "efile.h"

#define RxFIFOSIZE	64
#define Equal				0

void fft (void); 
void cr4_fft_64_stm32(void *pssOUT, void *pssIN, unsigned short Nbin);

const int COMMANDS = 12;
const int BADCOMMAND = COMMANDS;
const char BACKSPACE = 0x7F;
const char RETURN = 0x0D;

// Interpreter command strings and their respective function pointers.
// since sizeof() doesn't work for arrays of char and func pointers I had to create a constant,
// So be sure to update the const COMMANDS after adding or removing functions
const char* commands[COMMANDS] = { "echo", "adc", "clear", "fft", "performance", "debug", "filter", "ls", "rm", "cat", "touch", "vi" };
void (*interFunc[COMMANDS])(char* paramString) = {echo, adcToggle, clear, fftToggle, performance, debug, filter, ls, rm, cat, touch, vi };


//int mailboxFull; 		// Put extern back in when ADC file is ready
//extern unsigned long MaxJitter;
//extern unsigned long PIDWork;
//extern unsigned long DataLost;
//extern unsigned long FilterWork;
//extern unsigned long NumSamples;
extern unsigned long NumCreated;
long x[6]; // x and y were static in other file, not sure if that mattters when externing- CH
long y[6];
Sema4Type toDisplay; 
int idxX = 0, idxY = 0; 
char buffer [10]; 
char filterFlag=0;
char fftActive =0;
int printHeight = 0;

void vi (char* filename) {
	  char data; 
		int fileIdx = eFile_findFile( filename);
	 if (fileIdx == -1) {
		 //create the file
		 eFile_Create(filename); 
	 } 
		//open file for writing 
	 eFile_WOpen(filename); 
	 while (RxFifo_Get(&data)) { 
		 if (data == ESC) 
			 break; 
		 else { 
			 eFile_Write(data); 
		 } 
	 }
	 eFile_WClose(); 
} 
int skipLeadingSpace(char* string){
	int index =0;
	while(string[index] != 0 && (string[index] == ' ' || string[index] == '\t')){
		index +=1;
	}
	if(string[index] == 0){return -1; }
	return index;
}
 

void sendCommand(char* cmdString){
 int i = 0; int strSize; 

	for (i = 0; i < COMMANDS; i++) {
		strSize = strlen(commands[i]); 
		if (strncmp(commands[i], cmdString, strSize) == Equal) {  
			break;  
		} 
	}
	
	cmdString++;
	if(i != BADCOMMAND){
		interFunc[i](cmdString+strSize);
	}
	else{
		echo("ERROR: Command not found\r\n");
	}
}

void Interpreter(void){
	char value [RxFIFOSIZE];
	char buffer [1];
	char* cmd;
	int cmd_idx = 0;
	int value_idx = 0; 
	int newCommand = 0;
	int i =0;
	
	cmd = (char*)malloc(RxFIFOSIZE);
	while(1){	
	if (!newCommand) {
		while (RxFifo_Get(&value[value_idx])){
//			for(i = 0; i<RxFIFOSIZE; i++){
//				cmd[i] = 'd';
//			}
			sprintf (buffer, "%c", value[value_idx]); 
			
			if(buffer[0] == BACKSPACE){	
				if(cmd_idx!=0){
					cmd_idx --;
					cmd[cmd_idx] = 0;
				}
			}
			else if(cmd_idx == RxFIFOSIZE){		// Max character limit met
				echo("ERROR: Command overflow");
				UART_OutString("\rERROR: Command Overflow\r"); 
				cmd_idx = 0;
			}
			else if (buffer[0] == RETURN) { 	// Command sent
				cmd[cmd_idx] = 0;				// Terminate command with null char
				newCommand = 1; 
				cmd_idx = 0;
			}else{									
				cmd[cmd_idx] = buffer[0];
				cmd_idx++; 
			}
			
			value_idx++; 
		}
		}else{
			newCommand = 0; 
			sendCommand(cmd);
		}
	}	
	
}


// ******** Interpreter Functions ***********

//************** echo *************** 
// prints the parameter to the ST7735 screen
// Inputs: String parameter that will be printed
// Outputs: none

void echo (char* paramString) {
	UART_OutString(paramString);
//	if(printHeight < 160){
//		ST7735_DrawStr(0,printHeight, paramString, 0xFFFF, 0x0000);
//		printHeight +=8;
//	}else{
//		clear("");		// Clears screen and resets printHeight
//		ST7735_DrawStr(0,printHeight, paramString, 0xFFFF, 0x0000);
//	}
} 

//************** adcToggle *************** 
// Turns the adc interrupt on or off
// Inputs: String parameter that defines whether
//				 the adc will be turned on or off
// Outputs: none

void adcToggle(char* paramString){
	OS_bWait(&toDisplay);
	
	if(strcmp(paramString, "off") == Equal){
		TIMER0_CTL_R &= ~TIMER_CTL_TAEN;          // disable timer0A during setup
		//ST7735_FillScreen(0);
	}else if(strcmp(paramString, "on") == Equal){
		TIMER0_CTL_R |= TIMER_CTL_TAEN;           // enable timer0A 16-b, periodic, no interrupts
	}else{
		echo("Invalid parameter, enter on or off");
	}
	
	OS_bSignal(&toDisplay);
}

//************** clear *************** 
// Clears the ST7735 screen
// Inputs: String parameter. If the string is
//				 not null, return
// Outputs: none

void clear(char* paramString){
	OS_bWait(&toDisplay); 
	//ST7735_FillScreen(0); --- to clear LCD!! Add it back when using LCD
	printHeight = 0;
	OS_bSignal(&toDisplay); 
}

//************** fftToggle *************** 
// Turns fft on or off
// Inputs: String parameter that defies wheter 
//	 			 the fft will be turned on or off 
// Outputs: none

void fftToggle(char* paramString){
	if(strcmp(paramString, "on") == Equal){
		fftActive = 1;
		OS_AddThread(&fft, 0, 0);
	}else if(strcmp(paramString, "off") == Equal){
		fftActive = 0;
		//ST7735_FillScreen(0);
	}else{
		echo("Invalid fft mode");
	}
}

//************** performance *************** 
// prints debugging information to ST7735 screen
// Inputs: String parameter. If the string is
//				 not null, return
// Outputs: none

void performance(char* paramString){
//	OS_bWait(&toDisplay); 
//	OS_DisableInterrupts(); 
//	ST7735_Message(0,0,"# Data Pts  =",NumSamples);
//	ST7735_Message(0,1,"Threads Made=",NumCreated);
//	ST7735_Message(0,2,"Jitter 0.1us=",MaxJitter);
//	ST7735_Message(0,3,"DataLost    =",DataLost);
//	ST7735_Message(1,0,"FilterWork  =",FilterWork);
//	ST7735_Message(1,1,"PIDWork     =",PIDWork);
//	OS_bSignal(&toDisplay); 
//	OS_EnableInterrupts(); 
}

//************** debug *************** 
// prints the either the raw adc or fft 
// data to putty
// Inputs: String parameter. Defines whether
// 				 the adc or fft data will be printed
// Outputs: none

void debug(char* paramString){
		int i;
		OS_bWait(&toDisplay); 
		if(strcmp(paramString, "adc") == Equal){
			for(i=0;i<64;i++){
				sprintf(buffer, "%d\r",(int)x[i]); 
				UART_OutString(buffer); 
			}
		}else if(strcmp(paramString, "fft") == Equal){
			for(i=0;i<64;i++){
				sprintf(buffer, "%d\r",(int)y[i]); 
				UART_OutString(buffer); 
			}
		}else{
			  echo("Invalid Debugging parameter, enter adc or fft");
		}
		OS_bSignal(&toDisplay); 
}

//************** filter *************** 
// Turns the filter on or off
// Inputs: String parameter that defies wheter 
//	 			 the filter will be turned on or off 
// Outputs: none

void filter(char* paramString){
	if(strcmp(paramString, "on") == Equal){
			// turn digital filter on
		filterFlag = 1; Filter_Init(); 
		//Filter_FIR();
	}else if(strcmp(paramString, "off") == Equal){
		// turn digital filter off
		filterFlag = 0;
	}else{
		echo("Invalid parameter: Enter 'on' or 'off'");
	}
}

//************** ls *************** 
// Lists all the files in the directory with file size
// Inputs: String parameter. Should be null
// Outputs: none

void ls(char* paramString){
	eFile_Directory();
}

//************** rm *************** 
// Removes a file from directory
// Inputs: String parameter that specifies the name of the file
// Outputs: none

void rm(char* paramString){
	char buffer[40];
	if(eFile_Delete(paramString) == 1){
		sprintf(buffer, "%s successfully deleted", paramString);
	}else{
		sprintf(buffer, "%s could not be deleted", paramString);
	}
	UART_OutString(buffer);
}


//************** cat *************** 
// Prints the contents of the directory
// Inputs: String parameter that specifies the name of the file
// Outputs: none

void cat(char* paramString){
	eFile_PrintFileContents(paramString);
}

//************** touch *************** 
// Prints the contents of the directory
// Inputs: String parameter that specifies the name of the file
// Outputs: none

void touch(char* paramString){
	if (eFile_Create(paramString)) 
			UART_OutString ("Error while creating the file\r\n"); 
	else 
		UART_OutString("File successfully created\r\n"); 
}

// ********** Function definitions to make compiler happy ***********
void fft (void) {
	 int DCcomponent, ImComp, i, status, pixelPos; 
	 while(fftActive){
		status = StartCritical ();
		cr4_fft_64_stm32(y,x,64);  // complex FFT of last 64 ADC values
	  //ST7735_PlotClear(0,127);  // clip large magnitudes
			
		for (i = 0; i < 64; i++) {  
			DCcomponent = (y[i]&0xFFFF); // Real part at frequency 0, imaginary part should be zero
			ImComp = (y[i] & 0xFFFF0000) >> 15; 
			y[i] = sqrt(DCcomponent*DCcomponent + ImComp*ImComp); 
			DCcomponent = y[i]; 
			//DCcomponent = (DCcomponent < 0) ? -DCcomponent : DCcomponent; 
			//pixelPos = 148 - ((DCcomponent* 366)/10000) ;
			pixelPos = (DCcomponent/5); 
			//ST7735_PlotBar(pixelPos);  // clip large magnitudes
			//ST7735_PlotPoint(pixelPos); // called 4 times
			//ST7735_PlotNext();
	 } 
	 EndCritical(status); 
 }
	OS_Kill();
 }
