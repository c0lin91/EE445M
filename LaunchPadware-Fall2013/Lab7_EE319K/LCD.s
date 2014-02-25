; LCD.s
; Student names: change this to your names or look very silly
; Last modification date: change this to the last modification date or look very silly

; Runs on LM4F120 or TM4C123
; EE319K lab 7 device driver for the Kentec EB-LM4F120-L35
; This is the lowest level driver that interacts directly with hardware
; As part of Lab 7, students need to implement these three functions
;
;  Data pin assignments:
;  PB0-7   LCD parallel data input
;
;  Control pin assignments:
;  PA4     RD  Read control signal             -------------------------
;  PA5     WR  Write control signal            | PA7 | PA6 | PA5 | PA4 |
;  PA6     RS  Register/Data select signal     | CS  | RS  | WR  | RD  |
;  PA7     CS  Chip select signal              -------------------------
;
;  Touchpad pin assignments:
;  PA2     Y-                                  -------------  -------------
;  PA3     X-                                  | PA3 | PA2 |  | PE5 | PE4 |
;  PE4     X+    AIN9                          | X-  | Y-  |  | Y+  | X+  |
;  PE5     Y+    AIN8                          -------------  -------------

      EXPORT   LCD_GPIOInit
      EXPORT   LCD_WriteCommand
      EXPORT   LCD_WriteData

      AREA    |.text|, CODE, READONLY, ALIGN=2
      THUMB
      ALIGN          
SYSCTL_RCGC2_R          EQU   0x400FE108
GPIO_PORTA_DATA_R       EQU   0x400043FC
GPIO_PORTA_DIR_R        EQU   0x40004400
GPIO_PORTA_AFSEL_R      EQU   0x40004420
GPIO_PORTA_DEN_R        EQU   0x4000451C
GPIO_PORTB_DATA_R       EQU   0x400053FC
GPIO_PORTB_DIR_R        EQU   0x40005400
GPIO_PORTB_AFSEL_R      EQU   0x40005420
GPIO_PORTB_DEN_R        EQU   0x4000551C

; ************** LCD_GPIOInit ****************************
; Initializes Ports A and B for Kentec EB-LM4F120-L35
; Port A bits 4-7 are output to four control signals
; Port B bits 0-7 are output data is the data bus 
; Initialize all control signals high (off)
;  PA4     RD  Read control signal             -------------------------
;  PA5     WR  Write control signal            | PA7 | PA6 | PA5 | PA4 |
;  PA6     RS  Register/Data select signal     | CS  | RS  | WR  | RD  |
;  PA7     CS  Chip select signal              -------------------------
; wait 40 us
; Invariables: This function must not permanently modify registers R4 to R11
LCD_GPIOInit
   
      BX  LR
;* * * * * * * * End of LCD_GPIOInit * * * * * * * * 


; ************** LCD_WriteCommand ************************
; - Writes an 8-bit command to the LCD controller
; - RS low during command write
; 8-bit command passed in R0
; 1) LCD_DATA = 0x00;    // Write 0 as MSB of command 
; 2) LCD_CTRL = 0x10;    // Set CS, WR, RS low
; 3) LCD_CTRL = 0x70;    // Set WR and RS high
; 4) LCD_DATA = command; // Write 8-bit LSB command 
; 5) LCD_CTRL = 0x10;    // Set WR and RS low
; 6) wait 2 bus cycles     
; 7) LCD_CTRL = 0xF0;    // Set CS, WR, RS high
; ********************************************************
; Invariables: This function must not permanently modify registers R4 to R11
LCD_WriteCommand
      
      BX  LR
;* * * * * * * * End of LCD_WriteCommand * * * * * * * * 

; ************** LCD_WriteData ***************************
; - Writes 16-bit data to the LCD controller
; - RS high during data write
; 16-bit data passed in R0
; 1) LCD_DATA = (data>>8);  // Write MSB to LCD data bus
; 2) LCD_CTRL = 0x50;       // Set CS, WR low
; 3) LCD_CTRL = 0x70;       // Set WR high
; 4) LCD_DATA = data;       // Write LSB to LCD data bus 
; 5) LCD_CTRL = 0x50;       // Set WR low
; 6) wait 2 bus cycles     
; 7) LCD_CTRL = 0xF0;       // Set CS, WR high
; ********************************************************
; Invariables: This function must not permanently modify registers R4 to R11
LCD_WriteData
      
      BX  LR
;* * * * * * * * End of LCD_WriteData * * * * * * * * 




    ALIGN                           ; make sure the end of this section is aligned
    END                             ; end of file
    