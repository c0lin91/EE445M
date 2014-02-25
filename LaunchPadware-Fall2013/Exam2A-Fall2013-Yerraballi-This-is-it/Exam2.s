;*****Your name goes here*******
; -5 points if you do not add your name

;This is Exam2_BCD  
;EE319K Fall 2013
;November 7, 2013
;You edit this file only
       AREA   Data, ALIGN=4


       AREA    |.text|, CODE, READONLY, ALIGN=2
       THUMB

;***** PosOrNeg subroutine*********************
;Determines if the given BCD number in R0 is a positive or negative number 
;Input:   R0 has a number in 3-digit 16-bit signed BCD format
;Output:  R0 returned as +1 if the number was positive and -1 if the number was negative
;Invariables: You must not permanently modify registers R4 to R11, and LR
;AAPCS rules: You must push and pop an even number of registers
;Error conditions: none, all inputs will be valid BCD numbers
      EXPORT PosOrNeg
PosOrNeg 
; put your code here
     MOV R1, #0x8000
	 AND R0, R1
	 LSR R0, #15
	 MOV R1, #-1
	 MUL R0, R1
	 CMP R0, #0
	 BNE Done
	 MOV R0, #1
Done  
     BX    LR
      
;***** BCD2Dec subroutine*********************
; Converts a 3-digit 16-bit signed number in BCD format into a 
; regular binary representation
; Note that BCD is positional number system with each succesive nibble
; having a place value that is a power of 10.
;Input:   R0 has a number in 3-digit signed BCD format
;Output:  R0 has the signed 32-bit binary-encoded value of the input
;Invariables: You must not permanently modify registers R4 to R11, and LR
;AARP rules: You must push and pop an even number of registers
;Error conditions: none, all inputs will be valid BCD numbers
      EXPORT BCD2Dec
BCD2Dec 
; put your code here
	  MOV R1, #0 ; Clear R1 so I can keep my result in here
	  
	  AND R2, R0, #0x000F ; R2 has the input
	  ADD R1, R2
	  
	  AND R2, R0, #0x00F0 
	  LSR R2, #4
	  MOV R3, #10
	  MUL R2, R3 
	  ADD R1, R2
	  
	  AND R2, R0, #0x0F00 
	  LSR R2, #8
	  MOV R3, #100
	  MUL R2, R3 
	  ADD R1, R2
	  
	  PUSH {R1, R2, LR} 
	  BL PosOrNeg
	  POP {R1, R2, LR} 
	  
	  MUL R0, R1
      BX   LR
      
;***** BCDMul subroutine*********************
; Multiplies two 3-digit 16-bit signed numbers in BCD format 
; and returns the result in regular binary representation
;Input:   R0, R1 have numbers in 3-digit 16-bit signed BCD format
;Output:  R0 has the signed 32-bit product of the two input numbers
;Invariables: You must not permanently modify registers R4 to R11, and LR
;AAPCS rules: You must push and pop an even number of registers
;Error conditions: none, all inputs will be valid BCD numbers. No overflow can occur
;Here are the test cases:
      EXPORT BCDMul
BCDMul 
; put your code here
      PUSH {LR}
	  PUSH {R1}
	  BL BCD2Dec
	  POP {R1}
      MOV R2, R0
	  MOV R0, R1
	  PUSH {R1, R2}
	  BL BCD2Dec
	  POP{R1, R2}
	  MUL R0, R2
	  POP {LR}
      BX   LR
	  
;***** DotProduct subroutine*********************
; Computes the dot product of two BCD-encoded arrays of numbers.
; The inputs are two pointers to arrays of 3-digit signed BCD numbers
; and the size of the two arrays.
;Input:   R0 has address of the first array A
;         R1 has address of the second array B
;         R2=n is the size of both arrays
;Output:  R0 has the binary-encoded result of computing the dot product
;         A[0]*BB[0] + AA[1]*BB[1] + ...+AA[n-1]*BB[n-1]
;Invariables: You must not permanently modify registers R4 to R11, and LR
;AAPCS rules: You must push and pop an even number of registers
;If the arrays are empty, return 0
;Error conditions: all numbers will be valid
      EXPORT DotProduct
DotProduct
; put your code here
	  PUSH {R4, R5, LR} 
	  MOV R3, R0 
	  MOV R4, R1
	  MOV R5, #0 
	  
	  CMP R2, #0 
	  BEQ Dot_Done
again	  
	  LDRH R0, [R3] 
	  LDRH R1, [R4] 
	  
	  PUSH {R2, R3, R4, R5} 
	  BL BCDMul
	  POP {R2, R3, R4, R5} 
	  ADD R5, R5, R0 
	  
	  ADD R3, #2
	  ADD R4, #2
	  SUBS R2, #1
	  BNE again

Dot_Done 
	  ; R5 is the result 
	  
	  MOV R0, R5
	  
	  POP {R4, R5, LR}
	  
	  BX   LR
	  ALIGN
      END
      
