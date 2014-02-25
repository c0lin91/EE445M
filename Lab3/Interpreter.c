
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "Interpreter.h"
#include "ADC.h"
#include "ST7735.h"
#include "UART2.h"
#include "FIFO.h"
#include "OS.h"


#define RxFIFOSIZE	64


const char* commands[] = { "echo", "adc", "clear", "pop", "performance", "debug" };

const int totalCommands = 6; 
//int mailboxFull; 		// Put extern back in when ADC file is ready
extern unsigned long MaxJitter;
extern unsigned long PIDWork;
extern unsigned long DataLost;
extern unsigned long FilterWork;
extern unsigned long NumSamples;
extern unsigned long NumCreated;
extern long x[6]; // x and y were static in other file, not sure if that mattters when externing- CH
extern long y[6];
int idxX = 0, idxY = 0; 
char buffer [10]; 

int skipLeadingSpace(char* string){
	int index =0;
	while(string[index] != 0 && (string[index] == ' ' || string[index] == '\t')){
		index +=1;
	}
	if(string[index] == 0){return -1; }
	return index;
}
void echo (char* cmdString) {
	static int y = 50; 
	ST7735_DrawStr(0,y, cmdString, 0xFFFF, 0x0000);
} 
void findExe(int func, char* cmdString) {
	switch(func) { 
	case 0: 
		echo (cmdString); 
		break;
	case 1:
//		if (mailboxFull) {
//			mailboxFull = 0; // mailbox = empty
//			ST7735_Message (0, 0, "ADC Value: ", ADC_In());
//		} 
//		else {
//			ST7735_Message (0, 0, "ADC Value: Error! ", 0);
//		} 
		break; 
	case 2:
		ST7735_FillScreen(0);  
		break; 
	case 3: 
		break; 
	case 4:						// Add Perfomance
			OS_DisableInterrupts(); 
			//ST7735_FillScreen(0);  
			ST7735_Message(0,0,"# Data Pts  =",NumSamples);
			ST7735_Message(0,1,"Threads Made=",NumCreated);
			ST7735_Message(0,2,"Jitter 0.1us=",MaxJitter);
			ST7735_Message(0,3,"DataLost    =",DataLost);
			ST7735_Message(1,0,"FilterWork  =",FilterWork);
		  ST7735_Message(1,1,"PIDWork     =",PIDWork);
			OS_EnableInterrupts(); 
		break;
	case 5:
		//ST7735_FillScreen(0);
			cmdString +=1 ;
		 if(*cmdString == 'x'){
			 sprintf(buffer, "x[%d] = ", idxX); 
			 ST7735_Message(0,0,buffer,x[idxX++]);
			 sprintf(buffer, "x[%d] = ", idxX);
			 ST7735_Message(0,1,buffer,x[idxX++]);
			 idxX = idxX%64; 
//			ST7735_Message(0,2,"x[2] =",x[2]);
//			ST7735_Message(0,3,"x[3] =",x[3]);
//			ST7735_Message(1,0,"x[4] =",x[4]);
//		  ST7735_Message(1,1,"x[5] =",x[5]);
		 }else if(*cmdString == 'y'){
			ST7735_Message(0,0,"y[0] =",y[0]);
			ST7735_Message(0,1,"y[1] =",y[1]);
//			ST7735_Message(0,2,"y[2] =",y[2]);
//			ST7735_Message(0,3,"y[3] =",y[3]);
//			ST7735_Message(1,0,"y[4] =",y[4]);
//		  ST7735_Message(1,1,"y[5] =",y[5]);
		 }else{
			  echo("Invalid Debugging parameter, enter x or y");
		 }
		break;
	}
} 

void sendCommand(char* cmdString){
	int cmd_idx = 0; int i = 0; int strSize; 
	
	//cmd_idx += skipLeadingSpace(cmdString);

	if(cmd_idx == -1){ return; } 										// All whitespace
	for (i = 0; i < totalCommands; i++) {
		strSize = strlen(commands[i]); 
		if (!strncmp(commands[i], cmdString, strSize)) {  
			findExe(i, cmdString+cmd_idx+strSize); 
			break;  
		} 
	} 
}

void Interpreter(void){
	char value [RxFIFOSIZE];
	char buffer [RxFIFOSIZE];
	char command[RxFIFOSIZE];
	int cmd_idx = 0;
	int value_idx = 0; 
	int newCommand = 0;
	int i=0;
	int j=0;
	
	OS_DisableInterrupts(); 
	UART_Init();
	OS_EnableInterrupts(); 
	while(1){	
	if (!newCommand) {
		while (RxFifo_Get(&value[value_idx])){
			sprintf (buffer, "%c", value[value_idx]); 
			
			if(buffer[0] == 0x7F){				// backspace
				if(cmd_idx!=0){
					cmd_idx --;
					command[cmd_idx] = 0;
				}
			}
			else if(cmd_idx == RxFIFOSIZE){
			//	ST7735_DrawStr(10,81, "ERROR, Command Overflow", 0xFFFF, 0x0000); 
				cmd_idx = 0;
			}
			else if (buffer[0] != 0x0D) { 		// carrige return
				//ST7735_DrawChar(x, y, buffer[0], 0xFFFF, 0x0000, 1);
				command[cmd_idx] = buffer[0];
				cmd_idx++; 
			}else{									// carrige return
				i=0; j+=9;
				command[cmd_idx] = 0;				// Terminate character array with a null
				newCommand = 1; 
				cmd_idx = 0;
			}
			
			i+=5;
			if ((i > 132)) {
				i = 0; j += 9; 
			}
			if (j > 162) {
				i= 0; j = 0; 
				
				ST7735_FillScreen(0);  
			} 
//			ST7735_Message (0, 2, buffer, 0); 
				value_idx++; 
			}
		}else{
				newCommand = 0; 
				sendCommand(command);
		}
	}	
}
