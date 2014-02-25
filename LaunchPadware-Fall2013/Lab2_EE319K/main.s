;****************** main.s ***************
; Program written by: put your names here
; Date Created: 8/25/2013 
; Last Modified: 8/25/2013 
; Section 1-2pm     TA: Saugata Bhattacharyya
; Lab number: 2
; Brief description of the program
; The overall objective of this system is a digital lock
; Hardware connections
;  PF4 is switch input  (1 means SW1 is not pressed, 0 means SW1 is pressed)
;  PF2 is LED output (1 activates blue LED) 
; The specific operation of this system 
;    1) Make PF2 an output and make PF4 an input (enable PUR for PF4). 
;    2) The system starts with the LED ON (make PF2 =1). 
;    3) Delay for about 1 ms
;    4) If the switch is pressed (PF4 is 0), then toggle the LED once, else turn the LED ON. 
;    5) Repeat steps 3 and 4 over and over

GPIO_PORTF_DATA_R       EQU   0x400253FC
GPIO_PORTF_DIR_R        EQU   0x40025400
GPIO_PORTF_AFSEL_R      EQU   0x40025420
GPIO_PORTF_PUR_R        EQU   0x40025510
GPIO_PORTF_DEN_R        EQU   0x4002551C
GPIO_PORTF_AMSEL_R      EQU   0x40025528
GPIO_PORTF_PCTL_R       EQU   0x4002552C
SYSCTL_RCGC2_R          EQU   0x400FE108

       AREA    |.text|, CODE, READONLY, ALIGN=2
       THUMB
       EXPORT  Start
Start

loop  

       B    loop


       ALIGN      ; make sure the end of this section is aligned
       END        ; end of file