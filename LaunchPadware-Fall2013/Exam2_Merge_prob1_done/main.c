// main.c
// Runs on LM3S1968 using the simulator
// Grading engine for Exam2_Merge Practice Exam
// Do not modify any part of this file
// Exam given Spring 2011

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1

#include "PLL.h"
#include "UART.h"

unsigned short Answer[12];
#define COUNT1 4
struct TestCase1{
  unsigned short Value;           // value to store
  unsigned long Size;             // size of array
  unsigned short Correct[10];     // proper result of Init()
  long Score;                     // points to add if Init() correct
};
typedef const struct TestCase1 TestCase1Type;
TestCase1Type Tests1[COUNT1]={
{3,4,{0xFF,0x03,0x03,0x03,0x03,0xFF,0xFF,0xFF,0xFF,0xFF},5},
{7,1,{0xFF,0x07,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},5},
{5,8,{0xFF,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0xFF},5},
{0,0,{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},10}};


struct TestCase2{
  unsigned short Input[6];        // input array for Insert()
  unsigned long Size;             // size of array
  unsigned short Value;           // value to insert
  unsigned short Correct[10];     // proper result of Insert()
  long Score;     // points to add if Insert() correct
};
typedef const struct TestCase2 TestCase2Type;
#define COUNT2 4
TestCase2Type Tests2[COUNT2]={
{{0x01,0x02,0x03,0x04,0x05,0x06},6,0x08,{0xFF,0x08,0x01,0x02,0x03,0x04,0x05,0x06,0xFF,0xFF},5},
{{0x01,0x02,0x03,0x04,0x05,0x06},1,0x09,{0xFF,0x09,0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},5},
{{0x00,0x00,0x01,0x02,0x05,0x06},4,0xFE,{0xFF,0xFE,0x00,0x00,0x01,0x02,0xFF,0xFF,0xFF,0xFF},10},
{{0x01,0x02,0x03,0x04,0x05,0x06},0,0x45,{0xFF,0x45,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},10}};




struct TestCase3{
  unsigned short Input1[4];        // first array for Merge()
  unsigned short Input2[4];        // second array for Merge()
  unsigned long Size1;             // size of first array
  unsigned long Size2;             // size of second array
  unsigned short Correct[10];      // proper result of Merge()
  long Score;     // points to add if Merge() correct
};
typedef const struct TestCase3 TestCase3Type;
#define COUNT3 7
TestCase3Type Tests3[COUNT3]={
{{0x8000,0x00,0x00,0x00},{0x7000,0x00,0x00,0x00},1,1,{0xFF,0x7000,0x8000,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},5},
{{0x01,0x03,0x05,0x07},{0x02,0x04,0x06,0x08},4,4,{0xFF,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0xFF},5},
{{0x01,0x02,0x03,0x04},{0x05,0x06,0x07,0x08},4,4,{0xFF,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0xFF},5},
{{0x01,0x02,0x03,0x04},{0x05,0x06,0x07,0x08},2,4,{0xFF,0x01,0x02,0x05,0x06,0x07,0x08,0xFF,0xFF,0xFF},5},
{{0x01,0x02,0x03,0x04},{0x05,0x06,0x07,0x08},4,1,{0xFF,0x01,0x02,0x03,0x04,0x05,0xFF,0xFF,0xFF,0xFF},5},
{{0x01,0x02,0x03,0x04},{0x05,0x06,0x07,0x08},4,0,{0xFF,0x01,0x02,0x03,0x04,0xFF,0xFF,0xFF,0xFF,0xFF},10},
{{0x01,0x02,0x03,0x04},{0x05,0x06,0x07,0x08},0,0,{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},10}};





//prototypes to student code
void Init(unsigned short *buffer,unsigned short value, unsigned long size);
void Insert(const unsigned short *buffer, unsigned long size, unsigned short value, unsigned short *answer);
void Merge(const unsigned short *buffer1,const unsigned short *buffer2,unsigned long size1, unsigned long size2, unsigned short *answer);
int main(void){ int i;
  unsigned long result,n,score;
  score = 0;
  PLL_Init();

  UART_Init();              // initialize UART

  UART_OutString("\n\rExam2_Merge\n\r");
  UART_OutString("Test of Init\n\r");
  for(n = 0; n < COUNT1 ; n++){
    for(i=0; i<10; i++){
      Answer[i] = 0xFF;
    }
    Init(&Answer[1],Tests1[n].Value,Tests1[n].Size);
    result = 1;
    for(i=0; i<10; i++){
      if(Answer[i] != Tests1[n].Correct[i]) result=0;
    }
    if(result){
      UART_OutString(" Yes"); 
      score = score + Tests1[n].Score;
    }else{
      UART_OutString(" No");
    }
 
    UART_OutString(", Score = "); UART_OutUDec(score);  UART_OutCRLF();
  }

  UART_OutString("Test of Insert\n\r");
  for(n = 0; n < COUNT2 ; n++){
    for(i=0; i<10; i++){
      Answer[i] = 0xFF;
    }
    Insert(Tests2[n].Input,Tests2[n].Size,Tests2[n].Value,&Answer[1]);
    result = 1;
    for(i=0; i<10; i++){
      if(Answer[i] != Tests2[n].Correct[i]) result=0;
    }
    if(result){
      UART_OutString(" Yes"); 
      score = score + Tests2[n].Score;
    }else{
      UART_OutString(" No");
    } 
    UART_OutString(", Score = "); UART_OutUDec(score);  UART_OutCRLF();
  }
  
  UART_OutString("Test of Merge\n\r");
  for(n = 0; n < COUNT3 ; n++){
    for(i=0; i<10; i++){
      Answer[i] = 0xFF;
    }
    Merge(Tests3[n].Input1,Tests3[n].Input2,Tests3[n].Size1,Tests3[n].Size2,&Answer[1]);
    result = 1;
    for(i=0; i<10; i++){
      if(Answer[i] != Tests3[n].Correct[i]) result=0;
    }
    if(result){
      UART_OutString(" Yes"); 
      score = score + Tests3[n].Score;
    }else{
      UART_OutString(" No");
    }
    UART_OutString(", Score = "); UART_OutUDec(score);  UART_OutCRLF();

  }
  

  UART_OutString("End of Exam2_Merge\n\r");
  while(1){
  }
}
