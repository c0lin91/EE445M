; PeriodicSysTickInts.s
; Runs on LM4F120
; Use the SysTick timer to request interrupts at a particular period.
; Daniel Valvano
; November 6, 2012

;  This example accompanies the book
;   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers"
;   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2013
;   Volume 1, Program 9.7
   
;   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
;   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013
;   Volume 2, Program 5.12, section 5.7
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

; oscilloscope or LED connected to PF2 for period measurement

        IMPORT   PLL_Init
        IMPORT   SysTick_Init

GPIO_PORTF2        EQU 0x40025010
GPIO_PORTF_DIR_R   EQU 0x40025400
GPIO_PORTF_AFSEL_R EQU 0x40025420
GPIO_PORTF_DEN_R   EQU 0x4002551C
GPIO_PORTF_AMSEL_R EQU 0x40025528
GPIO_PORTF_PCTL_R  EQU 0x4002552C
SYSCTL_RCGC2_R     EQU 0x400FE108
SYSCTL_RCGC2_GPIOF EQU 0x00000020   ; port F Clock Gating Control

        AREA    DATA, ALIGN=4
Counts  SPACE   4                   ; records number of SysTick interrupts
        EXPORT  Counts              ; global only for observation using debugger
  ALIGN                             ; make sure the end of this section is aligned

        AREA    |.text|, CODE, READONLY, ALIGN=2
        THUMB
        EXPORT  SysTick_Handler
        EXPORT  Start
SysTick_Handler
    ; toggle LED
    LDR R1, =GPIO_PORTF2            ; R1 = &GPIO_PORTF2 (pointer)
    LDR R0, [R1]                    ; R0 = [R1] = GPIO_PORTF2 (value)
    EOR R0, R0, #0x04               ; R0 = R0^0x04 (toggle PF2)
    STR R0, [R1]                    ; [R1] = R0
    ; increment Counts
    LDR R1, =Counts                 ; R1 = &Counts (pointer)
    LDR R0, [R1]                    ; R0 = [R1] = Counts (value)
    ADD R0, R0, #1                  ; R0 = R0 + 1 (Counts = Counts + 1)
    STR R0, [R1]                    ; [R1] = R0 (overflows after 49 days)
    BX  LR                          ; return from interrupt

Start
    BL  PLL_Init                    ; 50 MHz clock
    ; activate clock for Port F
    LDR R1, =SYSCTL_RCGC2_R         ; R1 = &SYSCTL_RCGC2_R (pointer)
    LDR R0, [R1]                    ; R0 = [R1] = SYSCTL_RCGC2_R (value)
    ORR R0, R0, #SYSCTL_RCGC2_GPIOF ; R0 = R0|SYSCTL_RCGC2_GPIOF
    STR R0, [R1]                    ; [R1] = R0
    NOP
    NOP                             ; allow time to finish activating
    ; set direction register
    LDR R1, =GPIO_PORTF_DIR_R       ; R1 = &GPIO_PORTF_DIR_R (pointer)
    LDR R0, [R1]                    ; R0 = [R1] = GPIO_PORTF_DIR_R (value)
    ORR R0, R0, #0x04               ; R0 = R0|0x04 (make PF2 output)
    STR R0, [R1]                    ; [R1] = R0
    ; regular port function
    LDR R1, =GPIO_PORTF_AFSEL_R     ; R1 = &GPIO_PORTF_AFSEL_R (pointer)
    LDR R0, [R1]                    ; R0 = [R1] = GPIO_PORTF_AFSEL_R (value)
    BIC R0, R0, #0x04               ; R0 = R0&~0x04 (disable alt funct on PF2)
    STR R0, [R1]                    ; [R1] = R0
    ; enable digital port
    LDR R1, =GPIO_PORTF_DEN_R       ; R1 = &GPIO_PORTF_DEN_R (pointer)
    LDR R0, [R1]                    ; R0 = [R1] = GPIO_PORTF_DEN_R (value)
    ORR R0, R0, #0x04               ; R0 = R0|0x04 (enable digital I/O on PF2)
    STR R0, [R1]                    ; [R1] = R0
    ; configure as GPIO
    LDR R1, =GPIO_PORTF_PCTL_R      ; R1 = &GPIO_PORTF_PCTL_R (pointer)
    LDR R0, [R1]                    ; R0 = [R1] = GPIO_PORTF_PCTL_R (value)
    BIC R0, R0, #0x00000F00         ; R0 = R0&~0x00000F00 (clear port control field for PF2)
    ADD R0, R0, #0x00000000         ; R0 = R0+0x00000000 (configure PF2 as GPIO)
    STR R0, [R1]                    ; [R1] = R0
    ; disable analog functionality
    LDR R1, =GPIO_PORTF_AMSEL_R     ; R1 = &GPIO_PORTF_AMSEL_R (pointer)
    MOV R0, #0                      ; R0 = 0 (disable analog functionality on PF)
    STR R0, [R1]                    ; [R1] = R0
    ; initialize Counts
    LDR R1, =Counts                 ; R1 = &Counts (pointer)
    MOV R0, #0                      ; R0 = 0
    STR R0, [R1]                    ; [R1] = R0 (Counts = 0)
    ; enable SysTick
    MOV R0, #50000                  ; initialize SysTick timer for 1,000 Hz interrupts
    BL  SysTick_Init                ; enable SysTick
    CPSIE I                         ; enable interrupts and configurable fault handlers (clear PRIMASK)
loop
    WFI                             ; wait for interrupt
    B   loop                        ; unconditional branch to 'loop'

    ALIGN                           ; make sure the end of this section is aligned
    END                             ; end of file
