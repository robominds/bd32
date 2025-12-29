/* newasm.h - assembler for BD32new header */

#pragma once

#include "dasmdefs.h"

#define MAXFIELD 30
#define PC_FIELD 26
#define CTOI(x) ((x) == '.' ? currvar : (x) - 'a')

extern LONG fields[MAXFIELD];
extern LONG sign_extend(LONG, int);
extern LONG disasm(LONG);
extern int eval_error;
extern char last_cmd[];
extern char outputting;
extern char *asmin;
extern char *asmout;
extern int block_level, opt_level;

/* Assembly expression parsing */
int sizetest(LONG stuff, int issigned);
int getconst(char **where, LONG *data);
int asm_expr(void);
int asm_sym(void);
int asm_percent(void);

/* Parsing helper functions */
char set_gc(char **s);
void skip_const(char **s);
int skip_percent(char **s);
int skip_set(char **s);
int skip_block_1(char end, char **s);
int skip_block(char **s);

/* Assembly block parsing */
int asm_set(void);
int asm_block(void);
int asm_block_1(void);

/* Bit output functions */
void bitout(int b);
void fbitout(LONG var, LONG mask, int count);
int sbitout(char s);

/* Main assembly functions */
int asmoutput(char *s);
int sasm(char *string);
int do_asm(int argc, char **argv);
