#ifndef INTERPRETER_H
#define INTERPRETER_H
void sendCommand(char* cmdString);

int skipLeadingSpace(char* string);

void Interpreter(void);

void echo(char* paramString);

void adcToggle(char* paramSting);

void clear(char* paramString);

void fftToggle(char* paramString);

void performance(char* paramString);

void debug(char* paramString);

void filter(char* paramString);

#endif

