
#include "efile.h"
#include "eDisk.h" 
#include "UART2.h" 
#include "String.h" 

#define NOT_USED 0 
BYTE freeSpace[MAX_NUMBER_OF_BLOCKS] = {0xFF}; 
Directory dir [MAXFILES]; 
//---------- eFile_Init-----------------
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
// since this program initializes the disk, it must run with 
//    the disk periodic task operating
int eFile_Init(void){
	int result = STA_NOINIT;
  
	//Try to power up the disk until successfull
	while (result != 0) { 
		result = eDisk_Init(0);
	} 
	//Error Checks
	if (result == STA_NOINIT) { 
		UART_OutString("Can not initialize the disk"); UART_OutChar(CR); UART_OutChar(LF);
		return 1; 
	} 
	if (result == STA_NODISK) {
			UART_OutString("Please insert a disk"); UART_OutChar(CR); UART_OutChar(LF);
			return 1; 
	} 
	if (result == STA_PROTECT) { 
		UART_OutString("No Write Permission to the current disk"); UART_OutChar(CR); UART_OutChar(LF);
		return 1; 
	} 
	
	//Passed all the errors
	//Read the directory and the free space
	result = STA_NOINIT; 
	while (result != 0) { 
		result = eDisk_ReadBlock ((BYTE*)dir, 0);
	} 
	result = STA_NOINIT;
	while (result != 0) { 
		result = eDisk_ReadBlock ((BYTE*)freeSpace, 1); 
	} 
	return 0;
}

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Format(void){
	int i, result; 
	for (i =0; i < MAXFILES; i++) { 
		dir[i].blockNum = 0; 
		dir[i].Name = 0; 
	} 
	
	for (i = 0; i < MAX_NUMBER_OF_BLOCKS; i++) { 
		freeSpace[i] = 0xFF; 
	} 
	//Write the initailized Directory 
	result = STA_NOINIT; 
	while (result != 0) { 
		result = eDisk_WriteBlock ((BYTE*)dir, 0);
	} 	
	result = STA_NOINIT;
	while (result != 0) { 
		result = eDisk_WriteBlock ((BYTE*)freeSpace, 1); 
	} 
	
	return 0;
}	

//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create( char name[]){
	int i, k, free_block, result; 
	Block currBlock; 
	//Find an open directory entry 
	for (i = 0; i < MAXFILES; i++) { 
		if (dir[i].blockNum == 0) 
			break; 
	} 
	if (i == MAXFILES) {
		return 1; //NO MORE FILES CAN BE MADE
	}
	//Make a directory entry
	if (strlen(name) > 7) { 
		int j; 
		for (j = 0; j < 7; j++){
			dir[i].Name[j] = name[j]; 
		} 
		dir[i].Name[j] = 0; 
	} else 
			dir[i].Name = name; 
	
	//find a free block 
	for(k = 0; k < MAX_NUMBER_OF_BLOCKS; k++) {
		if (freeSpace[k] == 0xFF){
			free_block = k;
			freeSpace[k] = 0; //make this space occupied. Null because acc to FAT we store the next pointer
			break; 
		}
	}		
	if (k == MAX_NUMBER_OF_BLOCKS)
		return 1; //no more space left 
	dir[i].blockNum = free_block; 

	//assign that block to this file -- initialize the metadata of a file
	currBlock.nextPtr = 0; 
	currBlock.usedBytes = NOT_USED;
	
	//write that block to the disk 
	//Write the initailized Directory 
	result = STA_NOINIT; 
	while (result != 0) { 
		result = eDisk_WriteBlock ((BYTE*)dir, 0);
	} 	
	result = STA_NOINIT;
	while (result != 0) { 
		result = eDisk_WriteBlock ((BYTE*)freeSpace, 1); 
	}
	result = STA_NOINIT;
	while (result != 0) { 
		result = eDisk_WriteBlock ((BYTE*)&currBlock, free_block); 
	}
	return 0;
}  // create new file, make it empty 


//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen(char name[]){
	return 1;	// open a file for writing 
}

//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write( char data){
	return 1;
}

//---------- eFile_Close-----------------
// Deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently open)
int eFile_Close(void){
	return 1;
}


//---------- eFile_WClose-----------------
// close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(void){
		return 1;
}// close the file for writing

//---------- eFile_ROpen-----------------
// Open the file, read first block into RAM 
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble read to flash)
int eFile_ROpen( char name[]){
	return 1;
}	// open a file for reading 
   
//---------- eFile_ReadNext-----------------
// retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext( char *pt){
return 1;	// get next byte 
}                           
//---------- eFile_RClose-----------------
// close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(void){
return 1;
}	// close the file for writing

//---------- eFile_Directory-----------------
// Display the directory with filenames and sizes
// Input: pointer to a function that outputs ASCII characters to display
// Output: characters returned by reference
//         0 if successful and 1 on failure (e.g., trouble reading from flash)
int eFile_Directory(void(*fp)(unsigned char)){
return 1;
}	

//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete( char name[]){
return 1;
}  // remove this file 

//---------- eFile_RedirectToFile-----------------
// open a file for writing 
// Input: file name is a single ASCII letter
// stream printf data into file
// Output: 0 if successful and 1 on failure (e.g., trouble read/write to flash)
int eFile_RedirectToFile(char *name){
	return 1;
}

//---------- eFile_EndRedirectToFile-----------------
// close the previously open file
// redirect printf data back to UART
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_EndRedirectToFile(void){
	return 1;
}

