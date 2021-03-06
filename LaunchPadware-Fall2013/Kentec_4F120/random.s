       THUMB
       AREA    DATA, ALIGN=2
M      SPACE   4
       ALIGN          
       AREA    |.text|, CODE, READONLY, ALIGN=2
       EXPORT  Random_Init
       EXPORT  Random
Random_Init
       LDR R2,=M       ; R4 = &M, R4 points to M
       MOV R0,#1       ; Initial seed
       STR R0,[R2]     ; M=1
       BX  LR
;------------Random------------
; Return R0= random number
; Linear congruential generator 
; from Numerical Recipes by Press et al.
Random LDR R2,=M    ; R2 = &M, R4 points to M
       LDR R0,[R2]  ; R0=M
       LDR R1,=1664525
       MUL R0,R0,R1 ; R0 = 1664525*M
       LDR R1,=1013904223
       ADD R0,R1    ; 1664525*M+1013904223 
       STR R0,[R2]  ; store M
       BX  LR
       ALIGN      
       END  