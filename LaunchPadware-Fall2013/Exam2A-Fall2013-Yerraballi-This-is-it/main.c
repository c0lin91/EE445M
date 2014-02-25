// main.c
// Runs on LM3S1968 using the simulator
// Grading engine for Exam2_BCD Fall 2013
// Do not modify any part of this file
// Exam2 given November 7, 2013

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1

#include "PLL.h"
#include "UART.h"
#define COUNT0 5
struct TestCase0{
  short	bcd ;           // input to PosORNeg()
	long Sign;						// Proper result for PosORNeg()
  long Score;              // points to add if PosORNeg() correct
};
typedef const struct TestCase0 TestCase0Type;
TestCase0Type Tests0[COUNT0]={
{0x0125,1,4},
{0xF042,-1,4},
{0x0999,1,4},
{0xF001,-1,4},
{0x0000,1,4},
};
#define COUNT1 5
struct TestCase1{
  short	bcd ;           // input to BCD2Dec()
  long Correct;   // proper result of BCD2Dec()
  long Score;              // points to add if BCD2Dec() correct
};
typedef const struct TestCase1 TestCase1Type;
TestCase1Type Tests1[COUNT1]={
{0x0125,125,6},
{0xF042,-42,6},
{0x0999,999,6},
{0xF001,-1,6},
{0xF999,-999,6},
};

#define COUNT2 4
struct TestCase2{
  short	bcd1 ;           // first input to BCDMul()
	short	bcd2 ;           // second input to BCDMul()
  long Correct;   // proper result of BCDMul()
  long Score;              // points to add if BCDMul() correct
};
typedef const struct TestCase2 TestCase2Type;
TestCase2Type Tests2[COUNT2]={
{0x0005,0x0002,10,5},
{0xF001,0x0008,-8,5},
{0xF999,0xF999,998001,5},
{0x0013,0xF001,-13,5},
};

#define COUNT3 5
struct TestCase3{
  short  A[20];           // inputs to DotProduct()
	short  B[20];
	long  size;
  long Correct;   // proper result of DotProduct()
  long Score;              // points to add if DotProduct() correct
};
typedef const struct TestCase3 TestCase3Type;
TestCase3Type Tests3[COUNT3]={
{{0x0010,0x0020},{0x0030,0x0040},2,1100,6},
{{0xF010,0x0002},{0x0001,0x0003},2,-4,6},
{{0x0999,0xF999,0xF999},{0x0999,0xF999,0x0999},3,998001,6},
{{1,2,3,4,5},{1,1,1,1,2},5,20,6},
{{1,2,3,4,5,6,7,8,9,0x10},{1,2,3,4,5,6,7,8,9,0x10},10,385,6}
};
//prototypes to student code
long PosOrNeg(short);
long BCD2Dec(short);
long BCDMul(short,short);
long DotProduct(short [], short [], long);

int main(void){ 
  long result;
	unsigned long n,score;
  score = 0;
  PLL_Init();

  UART_Init();              // initialize UART

  UART_OutString("\n\rExam2_Binary Coded Decimal\n\r");
  UART_OutString("Test of PosORNeg\n\r");
  for(n = 0; n < COUNT0 ; n++){
    result = PosOrNeg(Tests0[n].bcd);
    if(result == Tests0[n].Sign){
      UART_OutString(" Yes, Your= "); 
      score = score + Tests0[n].Score;
    }else{
      UART_OutString(" No, Correct= ");
      UART_OutSDec(Tests0[n].Sign);  
      UART_OutString(", Your= "); 
    }
    UART_OutSDec(result);  
    UART_OutString(", Score = "); UART_OutUDec(score);  UART_OutCRLF();
  } 
  UART_OutString("Test of BCD2Dec\n\r");
  for(n = 0; n < COUNT1 ; n++){
    result = BCD2Dec(Tests1[n].bcd);
    if(result == Tests1[n].Correct){
      UART_OutString(" Yes, Your= "); 
      score = score + Tests1[n].Score;
    }else{
      UART_OutString(" No, Correct= ");
      UART_OutSDec(Tests1[n].Correct);  
      UART_OutString(", Your= "); 
    }
    UART_OutSDec(result);  
    UART_OutString(", Score = "); UART_OutUDec(score);  UART_OutCRLF();
  }
	UART_OutString("Test of BCDMul\n\r");
  for(n = 0; n < COUNT2 ; n++){
    result = BCDMul(Tests2[n].bcd1, Tests2[n].bcd2);
    if(result == Tests2[n].Correct){
      UART_OutString(" Yes, Your= "); 
      score = score + Tests2[n].Score;
    }else{
      UART_OutString(" No, Correct= ");
      UART_OutSDec(Tests2[n].Correct);  
      UART_OutString(", Your= "); 
    }
    UART_OutSDec(result);  
    UART_OutString(", Score = "); UART_OutUDec(score);  UART_OutCRLF();
  }
  UART_OutString("Test of Dot Product\n\r");
	for(n = 0; n < COUNT3 ; n++){
    result = DotProduct((short *)Tests3[n].A, (short *)Tests3[n].B,Tests3[n].size);
    if(result == Tests3[n].Correct){
      UART_OutString(" Yes, Your= "); 
      score = score + Tests3[n].Score;
    }else{
      UART_OutString(" No, Correct= ");
      UART_OutSDec(Tests3[n].Correct);  
      UART_OutString(", Your= "); 
    }
    UART_OutSDec(result);  
    UART_OutString(", Score = "); UART_OutUDec(score);  UART_OutCRLF();
  }
  UART_OutString("End of Exam2_BCD, Fall 2013\n\r");
  while(1){
  }
}
