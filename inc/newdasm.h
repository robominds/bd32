/* newdasm.h - BD32new disassembler header */

#pragma once

#include "dasmdefs.h"

/* Buffer management for disassembly output */
void ResetBufferPointer(void);
void pcb(char c);
void printb(char *String);
int pfb(char *Format, ...);

/* Utility functions */
LONG sign_extend(LONG stuff, int size);
void sltoh(char *buff, LONG stuff, int size);
char *showmin_(LONG stuff);
char *showmin_1(LONG stuff);
char *showmin(LONG stuff);
LONG getstuff(int size);
void immdata(int size);
int sh_ea(int mode, int reg, int sizeset);
void ofmt(char *control);
void badop(void);

/* Main disassembly functions */
LONG disasm(LONG where);
int parsebit(char *template);
int do_disasm(int argc, char **argv);
