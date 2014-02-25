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

 



      BX   LR

      ALIGN
      END
      
