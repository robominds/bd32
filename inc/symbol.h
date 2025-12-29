/* symbol.h - symbol table definitions for CPU32Bug */

#pragma once

#define SYMTABSIZE      1000	/* number of symbols that can be stored */
#define SYMHASH         719     /* largest prime < 0.8*SYMTABSIZE */
#define NAMESIZE        32      /* how many sig characters (+1) in name */

struct symtab
{
	char name [NAMESIZE];   /* the symbol */
	LONG value;             /* its value */
	struct symtab *next,    /* next symbol with same symbol hash */
	*nextval,               /* next symbol with same value hash */
	*lastval;               /* last symbol with same value hash */
};

#define         OP_ERROR        1
#define         SYM_ERROR       2
#define         CHAR_ERROR      4
#define         NOCHAR_ERROR    8

