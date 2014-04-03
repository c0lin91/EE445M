

// period is 16-bit number of PWM cycles in one period (3<=period) 
// duty is number of PWM cycles output is high (2<=duty<=period-1) 
// PWM clock rate = processor clock rate/SYSCTL_RCC_PWMDIV 
// = BusClock/2 (in this example) 
void PWM_Init(unsigned short period, unsigned short duty);
void PWM_Test(void);
void PWM0_A_Duty(unsigned short duty);
void PWM0_B_Duty(unsigned short duty);

void PWM0_Init(unsigned short period, unsigned short duty);

void Motor_Init(unsigned short period, unsigned short duty);
void Motor_Right_Duty(unsigned short duty);
void Motor_Left_Duty(unsigned short duty);
