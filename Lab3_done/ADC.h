
 
	void ADC_Init(unsigned char channelNum); 
	unsigned long ADC_In(void); //Using Busy wait
	void ADC_Collect(unsigned int channelNum, unsigned int fs, void (*ADCtask) (unsigned long)); //using interrupts
