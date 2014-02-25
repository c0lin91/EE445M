; main.s
; Runs on LM4F120
; Provide a brief introduction to the floating point unit
; Jonathan Valvano
; April 27, 2013

;  This example accompanies the book
;   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers"
;   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2013
;   Example zz
;
;Copyright 2013 by Jonathan W. Valvano, valvano@mail.utexas.edu
;   You may use, edit, run or distribute this file
;   as long as the above copyright notice remains
;THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
;OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
;MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
;VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
;OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
;For more information about my classes, my research, and my books, see
;http://users.ece.utexas.edu/~valvano/
        AREA    DATA, ALIGN=2
ten     SPACE   4
tenth   SPACE   4
In      SPACE   4
A       SPACE   4   

Buffer  SPACE   80  ; results of circle test
        EXPORT ten [DATA,SIZE=4]
        EXPORT tenth [DATA,SIZE=4]
        EXPORT Buffer [DATA,SIZE=80]

        AREA    |.text|, CODE, READONLY, ALIGN=2
        THUMB
        EXPORT  Start
; FPU already initialized in this start.s
Start
     VLDR.F32 S0,=10.0 ;the example from the book
     LDR  R0,=ten
     VSTR.F32 S0,[R0]
; 10 = (0,0x82,0x200000) = (0,10000010,01000000000000000000000)
     VLDR.F32 S1,=0.1  ;another example from the book
     LDR  R0,=tenth
     VSTR.F32 S1,[R0]
; 0.1 about (0,0x7B,0x4CCCCD) = (0,01111011,10011001100110011001101)

     VLDR.F32 S2,=0.0 
     VLDR.F32 S3,=0.5 
     
     LDR  R0,=In
     VSTR.F32 S3,[R0] ;In = 0.5
     BL   Test
     VLDR.F32 S4,=0.785398  ;pi/4 
     
     LDR  R4,=Buffer
     MOV  R5,#20

loop 
     VMOV.F32 S0,S2   ; set input parameter
     BL   CircleArea 
     VSTR.F32 S0,[R4],#4
     VADD.F32 S2,S2,S3  ; next input
     SUBS R5,#1
     BNE  loop
done B    done

Test PUSH {LR}
     LDR  R0,=In
     VLDR.F32 S0,[R0] ;S0 is In
     BL   CircleArea
     LDR  R0,=A
     VSTR.F32 S0,[R0] ;A=pi*In*In
     POP  {PC}

; Calculate the area of a circle
; Input: S0 is the radius of the circle
; Output: S0 is the area of the circle
CircleArea
     VMUL.F32 S0,S0,S0  ; input squared
     VLDR.F32 S1,=3.14159265
     VMUL.F32 S0,S0,S1  ; pi*r*r
     BX  LR     
     
NVIC_CPAC_R equ 0xE000ED88
EnableFPU
      LDR R0, =NVIC_CPAC_R
      LDR R1, [R0] ; Read CPACR
      ORR R1, R1, #0x00F00000  ; Set bits 20-23 to enable CP10 and CP11 coprocessors
      STR R1, [R0]
      BX  LR
      
    ALIGN                           ; make sure the end of this section is aligned
    END                             ; end of file
