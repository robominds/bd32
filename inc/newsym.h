/* newsym.h - symbol table routines for BD32new */

#pragma once

#include "symbol.h"

extern char *eval_ptr;
extern int symcount, binlast, eval_error;
extern int pauselinecount;

/* Symbol table initialization */
void syminit(void);

/* Symbol display and search */
int do_sd(int argc, char **argv);
int binsearch(char *search);

/* Hash functions */
unsigned vhash(LONG number, unsigned size);
unsigned hash(char *string, unsigned size);

/* Symbol lookup and management */
struct symtab *valfind(LONG value);
struct symtab *symfind(char *name_);
void addbin(struct symtab *sym);
struct symtab *symadd(char *name_, LONG value);

/* Operator and symbol parsing */
int searchtab(char **look, char *table[]);
int isopchar(char c);
int getoperator(void);
int issym1(char c);
int issym(char c);
void getconstant(LONG *where);
int issymbol(char **string, char *buff);
unsigned IsRegisterName(char **Symbol);

/* Expression evaluation */
int unary_term(LONG *where);
int mul_term(LONG *where);
int add_term(LONG *where);
int shift_term(LONG *where);
int comp_term(LONG *where);
int eq_term(LONG *where);
int and_term(LONG *where);
int eor_term(LONG *where);
int or_term(LONG *where);
LONG eval(char *string);
int eval_(LONG *where);
