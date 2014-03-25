
#include "efile.h"
#include "eDisk.h" 
#include "UART2.h" 
#include "String.h" 
#include <string.h>
#include <stdio.h>

#define NOT_USED 0		// should be greater than 0 since there is meta-data -CH
#define FREE		0xff


BYTE FAT			[MAX_NUMBER_OF_BLOCKS] = {0xFF}; 
Directory dir [MAXFILES] ={0}; 

int writeBlockNum = -1;
Block* writeBlock; 
Block	writer;

int readBlockNum = -1;
Block* readBlock;
Block reader;
int bytesRead = 0;

//************** Private Functions *************

//---------- findOpenBlock -------
// Finds an open block
// Input: none
// Output: Index of open block in FAT array, -1 if none exists

int findOpenBlock(void){
	int k;
	int free_block;
	for(k = 2; k < MAX_NUMBER_OF_BLOCKS; k++) {
		if (FAT[k] == FREE){
			free_block = k;
			FAT[k] = 0; //make this space occupied. Null because acc to FAT we store the next pointer
			break; 
		}
	}		
	if (k == MAX_NUMBER_OF_BLOCKS){
		return -1; //no more space left 
	}
	return free_block;
}

//---------- getFileSize -------
// Gets the size of a file on disk
// Input: Block that the file starts on
// Output: Size of file on disk 
int getFileSize(int blockNum){
	int totalBlocks = 1; 
	Block* fileBlock;
	int result = STA_NOINIT;
	
	while (FAT[blockNum] != 0) { 
		totalBlocks++; 
		blockNum = FAT[blockNum]; 
	} 
	
	return totalBlocks*512; 
}

int eFile_findFile(char* name) {
		int i; 
		int blockNum = 0;
	//find file by name
	for(i=0; i<MAXFILES ; i++){		
		if(dir[i].blockNum != 0){
		 if( strcmp(name, dir[i].Name) == 0){
			 blockNum = dir[i].blockNum;
			 break;
		 }
	 }
	}
	if(i== MAXFILES){
//		UART_OutString("File not found");
		return -1;
	}else { 
		return blockNum; 
	} 
} 

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
		result = eDisk_ReadBlock ((BYTE*)FAT, 1); 
	} 
	writeBlock = &writer;
	readBlock = &reader;
	return 0;
}

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)

const BYTE format_disk[BLOCKSIZE] = {0};

int eFile_Format(void){
	int i, result; 
//	for (i =0; i < MAXFILES; i++) { 
//		dir[i].blockNum = 0; 
//		dir[i].Name[0] = 0; 
//	} 
	
	for (i = 0; i < MAX_NUMBER_OF_BLOCKS; i++) { 
		FAT[i] = FREE; 					
	}
	
	for (i = 0; i < MAX_NUMBER_OF_BLOCKS; i++) {  // +1442 for a MB
		result = eDisk_WriteBlock (format_disk, i);
	}	
	//Write the initailized Directory 
	result = STA_NOINIT; 
	while (result != 0) { 
		result = eDisk_WriteBlock (format_disk, 0);
	} 	
	result = STA_NOINIT;
	while (result != 0) { 
		result = eDisk_WriteBlock (FAT, 1); 	
	} 
	
	result = STA_NOINIT; 
	while (result != 0) { 
		result = eDisk_ReadBlock ((BYTE*) dir, 0);
	} 	
	result = STA_NOINIT;
	while (result != 0) { 
		result = eDisk_ReadBlock (FAT, 1); 	
	} 	
	return 0;
}	

//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create( char name[]){
	int i, k, free_block, result; 
	//Block currBlock; 
	
	result = STA_NOINIT; 
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
	} else {
			sprintf(dir[i].Name, "%s", name);
			//dir[i].Name = name; 
	}
	
	//find a free block 
	free_block = findOpenBlock();
	if(free_block == -1){ 
		return 1;
	}
	dir[i].blockNum = free_block; 

	//assign that block to this file -- initialize the metadata of a file
//	writeBlock->nextPtr = 0; 
//	writeBlock->usedBytes = NOT_USED;
//	
	//write that block to the disk 
	//Write the initailized Directory 
	result = STA_NOINIT; 
	while (result != 0) { 
		result = eDisk_WriteBlock ((BYTE*)dir, 0);
	} 	
	result = STA_NOINIT;
	while (result != 0) { 
		result = eDisk_WriteBlock ((BYTE*)FAT, 1); 
	}
//	result = STA_NOINIT;
//	while (result != 0) { 
//		result = eDisk_WriteBlock ((BYTE*)writeBlock, free_block); 
//	}
	return 0;
}  // create new file, make it empty 


//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen(char name[]){
	int i;
	int result = STA_NOINIT;

	if(writeBlockNum !=-1){		// different file already open
		UART_OutString("Another file is already open\r\n");
		return 1;
	}
	//find file by name
	for(i=0; i<MAXFILES ; i++){		
		if(dir[i].blockNum != 0){
		 if( strcmp(name, dir[i].Name) == 0){
			 writeBlockNum = dir[i].blockNum;
			 break;
		 }
	 }
	}
	if(i== MAXFILES){
		UART_OutString("File not found\r\n");
		return 1;
	}
	//Find the end of the file -- last block 
	while (FAT[writeBlockNum] != 0) { 
		writeBlockNum = FAT[writeBlockNum]; 
	}
	//read the last block into RAM	
	while(result != 0){
		result = eDisk_ReadBlock ((BYTE*)writeBlock, writeBlockNum);
	}
	return 0;	// open a file for writing 
}

//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write( char data){
	int result = STA_NOINIT;
	int freeBlock;
//	Block* currBlock; 
//	while(result != 0){
//		result = eDisk_ReadBlock ((BYTE*)currBlock, writeBlockNum);
//	}
	
	if(writeBlock->usedBytes == DATABYTES){
		freeBlock = findOpenBlock();
		if(freeBlock == -1){
			UART_OutString("Insufficient disk space");
			return 1;
		}
		FAT[writeBlockNum] = freeBlock; 
		FAT[freeBlock] = 0; 
		writeBlock->nextPtr = freeBlock; 
		result = STA_NOINIT;
		while(result != 0){
			result = eDisk_WriteBlock ((BYTE*)writeBlock, writeBlockNum);
		}
	
		result = STA_NOINIT;
		while(result != 0){
			result = eDisk_ReadBlock ((BYTE*)writeBlock, freeBlock);
		}	
		writeBlock->nextPtr = 0; 
		writeBlock->usedBytes = NOT_USED;
		writeBlock->data[writeBlock->usedBytes] = data; 
		writeBlock->usedBytes +=1;
		writeBlockNum = freeBlock; 
  } else { 
		writeBlock->data[writeBlock->usedBytes] = data; 
		writeBlock->usedBytes +=1;
	} 		
	result = STA_NOINIT;
	while(result != 0){
		result = eDisk_WriteBlock ((BYTE*)writeBlock, writeBlockNum);
	}
	result = STA_NOINIT;
	while(result != 0){						// Just testing that the write function is working
		result = eDisk_ReadBlock ((BYTE*)writeBlock, writeBlockNum);
	}
	
	return 0;
}

//---------- eFile_Close-----------------
// Deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently open)
int eFile_Close(void){
	int result; 
	result = STA_NOINIT; 
	while (result != 0) { 
		result = eDisk_WriteBlock ((BYTE*)dir, 0);
	} 	
	result = STA_NOINIT;
	while (result != 0) { 
		result = eDisk_WriteBlock ((BYTE*)FAT, 1); 
	}
	result = STA_NOINIT;
	while (result != 0) { 
		result = eDisk_WriteBlock ((BYTE*)&writeBlock, writeBlockNum); 
	}
	writeBlockNum = -1;
	return 0;
}


//---------- eFile_WClose-----------------
// close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(void){
	int result; 
	result = STA_NOINIT; 
	while (result != 0) { 
		result = eDisk_WriteBlock ((BYTE*)dir, 0);
	} 	
	result = STA_NOINIT;
	while (result != 0) { 
		result = eDisk_WriteBlock ((BYTE*)FAT, 1); 
	}
	result = STA_NOINIT;
	while (result != 0) { 
		result = eDisk_WriteBlock ((BYTE*)writeBlock, writeBlockNum); 
	}
	writeBlockNum = -1;
	return 0;
}// close the file for writing

//---------- eFile_ROpen-----------------
// Open the file, read first block into RAM 
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble read to flash)
int eFile_ROpen( char name[]){
	int i;
	int result = STA_NOINIT;

	if(readBlockNum !=-1){		// different file already open
		UART_OutString("Another file is already open");
		return 1;
	}
	//find file by name
	for(i=0; i<MAXFILES ; i++){		
		if(dir[i].blockNum != 0){
		 if( strcmp(name, dir[i].Name) == 0){
			 readBlockNum = dir[i].blockNum;
			 break;
		 }
	 }
	}
	
	if(i== MAXFILES){
		UART_OutString("File not found\r");
		return 1;
	}
	
	//read the first block into RAM	
	while(result != 0){
		result = eDisk_ReadBlock ((BYTE*)readBlock, readBlockNum);
	}
	return 0;	// open a file for writing 
}	
   
//---------- eFile_ReadNext-----------------
// retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext( char *pt){
	int result = STA_NOINIT;

	if(bytesRead == DATABYTES - 1){
		// go to next block to read
		if(readBlock->nextPtr == 0){
			return 1;
		}else{
			while(result != 0){
				result = eDisk_ReadBlock ((BYTE*)readBlock, readBlock->nextPtr);
				if(result == 1){return 1;}
			}
			bytesRead = 0;
		}
	}
	
	*pt = readBlock->data[bytesRead];
	
	bytesRead++;
	return 0;
}                           
//---------- eFile_RClose-----------------
// close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(void){
	readBlockNum = -1;
	bytesRead = 0;
	return 1;
}	// close the file for writing

//---------- eFile_Directory-----------------
// Display the directory with filenames and sizes
// Input: pointer to a function that outputs ASCII characters to display
// Output: characters returned by reference
//         0 if successful and 1 on failure (e.g., trouble reading from flash)
//int eFile_Directory(void(*fp)(unsigned char)){
int eFile_Directory(void){
	int i, fileSize, numFiles = 0;
	char buffer[40];
	for(i=0; i<MAXFILES ; i++){		
		if(dir[i].blockNum != 0){
			numFiles++; 
			fileSize = getFileSize(dir[i].blockNum);
			sprintf(buffer, "%s\tSize of file on disk: %d bytes\r\n", dir[i].Name, fileSize);
			UART_OutString(buffer);
		}
	}
	if (!numFiles) { 
		UART_OutString("Disk Empty!! \r\n"); 
	} 
	return 1;
}	

//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete( char name[]){
	int i;
	int result = STA_NOINIT;
	int blockNum, tempBlock;
	for(i=0; i<MAXFILES ; i++){		
		if(dir[i].blockNum != 0){
		 if( strcmp(name, dir[i].Name) == 0){
			 break;
		 }
	 }
	}
	if(i== MAXFILES){
		UART_OutString("File not found");
		return 1;
	}
	
		// Clear directory entry
	blockNum = dir[i].blockNum;
	dir[i].blockNum = 0; 
	dir[i].Name[0] = 0; 

	do{
		eDisk_WriteBlock ((BYTE*)format_disk, blockNum);
		tempBlock = blockNum;
		blockNum = FAT[blockNum];
		FAT[tempBlock] = FREE;
	}while(blockNum !=0);
	while (result != 0) { 
		result = eDisk_WriteBlock ((BYTE*)dir, 0);
	} 
	
	result = STA_NOINIT;
	while (result != 0) { 
		result = eDisk_WriteBlock ((BYTE*)FAT, 0);
	} 
	return 0;
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

//---------- eFile_PrintFileContents-----------------
// Print the contents of this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., File doesn't exist)
int eFile_PrintFileContents(char *name){
	int fileFinished = 0;
	char charRead;
	if(eFile_ROpen(name) == 1){
		return 1;
	}
	
	while(!fileFinished){				// Print the file contents letter by letter
		if( eFile_ReadNext(&charRead) == 0){
			UART_OutChar(charRead);
		}else{fileFinished = 1;}	
	}
	
	eFile_RClose();
	
	return 0;
}
