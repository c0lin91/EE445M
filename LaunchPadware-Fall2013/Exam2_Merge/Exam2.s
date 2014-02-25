;*****Your name goes here*******
; -5 points if you do not add your name

;This is Exam2_Merge 
;EE319K Practice exam
;You edit this file only
       AREA   Data, ALIGN=4


       AREA    |.text|, CODE, READONLY, ALIGN=2
       THUMB

;***** Init *********************
;Initialize the array to a value
;Each value is 16-bit unsigned
;Initialize the first array to 1 and the second array to -1
;Inputs:  R0 is a pointer to an empty array of 16-bit unsigned numbers
;         R1 is the 16-bit unsigned value to be stored into each element
;         R2 is the number of 16-bit unsigned values in the array
;         The array maybe of zero length (R2 may be zero)
;Output:  none
;Error conditions: none
;Invariables: You must not permanently modify registers R4 to R11, and LR
;Test cases
;R0=>empty array, R1=3,R2=4, result new array is {3,3,3,3}
;R0=>empty array, R1=7,R2=1, result new array is {7}
;R0=>empty array, R1=5,R2=8, result new array is {5,5,5,5,5,5,5,5}
;R0=>empty array, R1=0,R2=0, result is no change to empty array
      EXPORT Init
Init 
; put your code here

Init_loop	  
	  CMP R2, #0
	  BEQ Init_Done
      
	  STRH R1, [R0] 
	  ADD R0, #2
	  SUB R2, #1
	  B Init_loop
Init_Done      

	 BX    LR

;***** Insert subroutine *********************
; Insert the number into the beginning of the output array
;   copy all the elements of the input array to the output array
;Inputs : Register R0 points to input array of 16-bit unsigned numbers
;         Register R1 contains the length of the input array
;         Register R2 contains number to insert
;         Register R3 is a pointer to an empty array that should be written into
;Outputs: The array at R3 is modified
;Invariables: You must not permanently modify registers R4 to R11, and LR
;Error conditions: none.
;test cases
;R0=>{1,2,3,4,5,6},R1=6,R2=8,result={8,1,2,3,4,5,6}
;R0=>{1},R1=1,R2=9,,result={9,1}
;R0=>{0,0,1,2},R1=4,R2=0xFE,,result={0xFE,0,0,1,2}
;R0=>{},R1=0,R2=0x45,result={0x45}

      EXPORT Insert
Insert 
; put your code here
	  ;PUSH {R4} 
	  STRH R2, [R3]  ; insert the new value
	  
	  
	  ADD R3, #2  ; move the pointer to the next location

Insert_Loop	  
	  CMP R1, #0
	  ;LDRH R1, [R0] 
	  ;CMP R1, #9999
	  BEQ Insert_Done
	
	  LDRH R2, [R0] 
      STRH R2, [R3] 
	  ADD R0, #2
	  ADD R3, #2
	  SUB R1, #1
	  B Insert_Loop
	  
Insert_Done
	  ;POP {R4} 
      BX   LR

;***** Merge subroutine *********************
; Assume both arrays are initially sorted 16-bit unsigned numbers
; Merge the two input arrays into the output array
;   making the output array contain all the numbers in sorted order
;Inputs : Register R0 points to the first array of 16-bit unsigned numbers
;         Register R1 points to the second array of 16-bit unsigned numbers
;         Register R2 contains the number of elements in the first array
;         Register R3 contains the number of elements in the second array
;         Top element on the stack is a pointer to an empty output array
;Outputs: the output array is modified
;Invariables: You must not permanently modify registers R4 to R11, and LR
;Error conditions: none.
;test cases
;R0=>{0x8000},             R1=>{0x7000},             R2=1,R3=1,output array={0x7000,0x8000}
;R0=>{0x01,0x03,0x05,0x07},R1=>{0x02,0x04,0x06,0x08},R2=4,R3=4,output array={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08}
;R0=>{0x01,0x02,0x03,0x04},R1=>{0x05,0x06,0x07,0x08},R2=4,R3=4,output array={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08}
;R0=>{0x01,0x02},          R1=>{0x05,0x06,0x07,0x08},R2=2,R3=4,output array={0x01,0x02,0x05,0x06,0x07,0x08}
;R0=>{0x01,0x02,0x03,0x04},R1=>{0x05},               R2=4,R3=1,output array={0x01,0x02,0x03,0x04,0x05}
;R0=>{0x01,0x02,0x03,0x04},R1=>{},                   R2=4,R3=0,output array={0x01,0x02,0x03,0x04}
;R0=>{},R1=>{},                                      R2=0,R3=0,output array={} (no change to output array)
      EXPORT Merge
Merge

; put your code here
	  PUSH {R4}
	  
	  LDR R4, [SP, #4] ; R4 is our pointer to the output array
	  PUSH {R5, R6}
	  
Merge_loop	  
	  CMP R2, #0 
	  BEQ First_Empty 
	  
	  CMP R3, #0 
	  BEQ Second_Empty 
	  
	  ; neither one of them is empty. Lets compare 
	  LDRH R5, [R0] 
	  LDRH R6, [R1] 
	  CMP R5, R6 
	  BHI High ; R5 > R6? 
	  
	  STRH R5, [R4] 
	  ADD R4, #2
	  ADD R0, #2
	  SUB R2, #1
	  B Merge_loop
	  
	  
High  ; R6 < R5... So, store R6 in output array
	  STRH R6, [R4] 
	  ADD R4, #2
	  ADD R1, #2 
	  SUB R3, #1 
	  B Merge_loop
 
First_Empty
		; is the second one also empty?
	  CMP R3, #0 
	  BEQ Merge_Done
	  
	  LDRH R6, [R1]
	  STRH R6, [R4] 
	  ADD R4, #2
	  ADD R1, #2 
	  SUB R3, #1
	  B First_Empty 
	  
Second_Empty 
      ; copy the first array because we know it's not empty
	  LDRH R5, [R0] 
	  STRH R5, [R4] 
	  ADD R4, #2
	  ADD R0, #2
	  SUBS R2, #1
							;CMP R2, #0 
	  BNE Second_Empty 
	  
Merge_Done	
		
	  POP {R5, R6} 
	  POP {R4}
      BX   LR

      ALIGN
      END
      
