; Timer0A.s
; Runs on LM4F120
; Use Timer0A in periodic mode to request interrupts at a particular
; period.
; Daniel Valvano
; November 6, 2012

;  This example accompanies the book
;   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers"
;   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2013
;   Volume 1, Program 9.8

;  "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
;   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013
;   Volume 2, Program 7.5, example 7.6

; Copyright 2013 by Jonathan W. Valvano, valvano@mail.utexas.edu
;   You may use, edit, run or distribute this file
;   as long as the above copyright notice remains
;THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
;OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
;MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
;VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
;OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
;For more information about my classes, my research, and my books, see
;http://users.ece.utexas.edu/~valvano/

NVIC_EN0_INT19        EQU 0x00080000  ; Interrupt 19 enable
NVIC_EN0_R            EQU 0xE000E100  ; IRQ 0 to 31 Set Enable Register
NVIC_PRI4_R           EQU 0xE000E410  ; IRQ 16 to 19 Priority Register
TIMER0_CFG_R          EQU 0x40030000
TIMER_CFG_16_BIT      EQU 0x00000004  ; 16-bit timer configuration,
                                      ; function is controlled by bits
                                      ; 1:0 of GPTMTAMR and GPTMTBMR
TIMER0_TAMR_R         EQU 0x40030004
TIMER_TAMR_TAMR_PERIOD EQU 0x00000002 ; Periodic Timer mode
TIMER0_CTL_R          EQU 0x4003000C
TIMER_CTL_TAEN        EQU 0x00000001  ; GPTM TimerA Enable
TIMER0_IMR_R          EQU 0x40030018
TIMER_IMR_TATOIM      EQU 0x00000001  ; GPTM TimerA Time-Out Interrupt
                                      ; Mask
TIMER0_ICR_R          EQU 0x40030024
TIMER_ICR_TATOCINT    EQU 0x00000001  ; GPTM TimerA Time-Out Raw
                                      ; Interrupt
TIMER0_TAILR_R        EQU 0x40030028
TIMER_TAILR_TAILRL_M  EQU 0x0000FFFF  ; GPTM TimerA Interval Load
                                      ; Register Low
TIMER0_TAPR_R         EQU 0x40030038
SYSCTL_RCGC1_R        EQU 0x400FE104
SYSCTL_RCGC1_TIMER0   EQU 0x00010000  ; timer 0 Clock Gating Control

        AREA    |.text|, CODE, READONLY, ALIGN=2
        THUMB
        EXPORT   Timer0A_Init

; ***************** Timer0A_Init ****************
; Activate Timer0A interrupts to run user task periodically
; Input: R0  interrupt period
;        Units of period are 1 us (assuming 50 MHz clock)
;        Maximum is 2^16-1
;        Minimum is determined by length of ISR
; Output: none
; Modifies: R0, R1, R2
Timer0A_Init
    ; 0) activate clock for Timer0
    LDR R1, =SYSCTL_RCGC1_R         ; R1 = &SYSCTL_RCGC1_R (pointer)
    LDR R2, [R1]                    ; R2 = [R1] = SYSCTL_RCGC1_R (value)
    ORR R2, R2, #SYSCTL_RCGC1_TIMER0; R2 = R2|SYSCTL_RCGC1_TIMER0
    STR R2, [R1]                    ; [R1] = R2
    NOP
    NOP                             ; allow time to finish activating
    ; 1) disable timer0A during setup
    LDR R1, =TIMER0_CTL_R           ; R1 = &TIMER0_CTL_R (pointer)
    LDR R2, [R1]                    ; R2 = [R1] = TIMER0_CTL_R (value)
    BIC R2, R2, #TIMER_CTL_TAEN     ; R2 = R2&~TIMER_CTL_TAEN (clear enable bit)
    STR R2, [R1]                    ; [R1] = R2
    ; 2) configure for 16-bit timer mode
    LDR R1, =TIMER0_CFG_R           ; R1 = &TIMER0_CFG_R (pointer)
    LDR R2, =TIMER_CFG_16_BIT       ; R2 = TIMER_CFG_16_BIT (value)
    STR R2, [R1]                    ; [R1] = R2
    ; 3) configure for periodic mode
    LDR R1, =TIMER0_TAMR_R          ; R1 = &TIMER0_TAMR_R (pointer)
    LDR R2, =TIMER_TAMR_TAMR_PERIOD ; R2 = TIMER_TAMR_TAMR_PERIOD (value)
    STR R2, [R1]                    ; [R1] = R2
    ; 4) reload value
    LDR R1, =TIMER0_TAILR_R         ; R1 = &TIMER0_TAILR_R (pointer)
    SUB R0, R0, #1                  ; R0 = R0 - 1 (counts down from R0 to 0)
    STR R0, [R1]                    ; [R1] = R0
    ; 5) 1us timer0A
    LDR R1, =TIMER0_TAPR_R          ; R1 = &TIMER0_TAPR_R (pointer)
    MOV R2, #49                     ; R2 = 49 (divide clock by 50)
    STR R2, [R1]                    ; [R1] = R2
    ; 6) clear timer0A timeout flag
    LDR R1, =TIMER0_ICR_R           ; R1 = &TIMER0_ICR_R (pointer)
    LDR R2, =TIMER_ICR_TATOCINT     ; R2 = TIMER_ICR_TATOCINT (value)
    STR R2, [R1]                    ; [R1] = R2
    ; 7) arm timeout interrupt
    LDR R1, =TIMER0_IMR_R           ; R1 = &TIMER0_IMR_R (pointer)
    LDR R2, =TIMER_IMR_TATOIM       ; R2 = TIMER_IMR_TATOIM (value)
    STR R2, [R1]                    ; [R1] = R2
    ; 8) set NVIC interrupt 19 to priority 2
    LDR R1, =NVIC_PRI4_R            ; R1 = &NVIC_PRI4_R (pointer)
    LDR R2, [R1]                    ; R2 = [R1] = NVIC_PRI4_R (value)
    AND R2, R2, #0x00FFFFFF         ; R2 = R2&0x00FFFFFF (clear interrupt 19 priority)
    ORR R2, R2, #0x40000000         ; R2 = R2|0x40000000 (interrupt 19 priority is in bits 31-29)
    STR R2, [R1]                    ; [R1] = R2
    ; 9) enable interrupt 19 in NVIC
    LDR R1, =NVIC_EN0_R             ; R1 = &NVIC_EN0_R (pointer)
    LDR R2, =NVIC_EN0_INT19         ; R2 = NVIC_EN0_INT19 (interrupt 19 enabled) (value)
    STR R2, [R1]                    ; [R1] = R2
    ; 10) enable timer0A
    LDR R1, =TIMER0_CTL_R           ; R1 = &TIMER0_CTL_R (pointer)
    LDR R2, [R1]                    ; R2 = [R1] = TIMER0_CTL_R (value)
    ORR R2, R2, #TIMER_CTL_TAEN     ; R2 = R2|TIMER_CTL_TAEN (set enable bit)
    STR R2, [R1]                    ; [R1] = R2
    BX  LR                          ; return

    ALIGN                           ; make sure the end of this section is aligned
    END                             ; end of file
