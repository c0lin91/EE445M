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

void ls(char* paramString);

void rm(char* paramString);

void cat(char* paramString);

void touch(char* paramString);

void vi(char* paramString);

#endif

