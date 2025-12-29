/* newsym.c - symbol table routines for BD32new
 * implements a hashed symbol table and expression evaluation
 */

#include        <stdio.h>
#include        <stdlib.h>
#include        <memory.h>
#include        <ctype.h>
#include        <string.h>
#include        "didefs.h"
#include        "symbol.h"
#include        "gptable.h"
#include        "gpdefine.h"
#include	"target.h"
#include	"regnames.h"
#include	"reglist.h"

#include	"scrninfo.h"
#include	"prog-def.h"
#include	"bd32new.h"
#include	"newsym.h"
#include	"target.h"

#define         TOKENSIZE       40
#define         OR_OP           1
#define         EOR_OP          2
#define         AND_OP          3
#define         NE_OP           4
#define         EQ_OP           5
#define         GE_OP           6
#define         LE_OP           7
#define         SR_OP           8
#define         SL_OP           9
#define         L_OP            10
#define         G_OP            11
#define         PLUS_OP         12
#define         MINUS_OP        13
#define         REM_OP          14
#define         DIV_OP          15
#define         MUL_OP          16
#define         NOT_OP          17
#define         OPEN_OP         18
#define         CLOSE_OP        19
#define         INV_OP          20
#define         UNKNOWN_OP      21

static char *optable [] = {
"|",
"^",
"&",
"!=",
"==",
">=",
"<=",
">>",
"<<",
"<",
">",
"+",
"-",
"%",
"/",
"*",
"!",
"(",
")",
"~",
NULL };

static struct symtab    *table [SYMTABSIZE],
			*vtable [SYMTABSIZE],
			*bintable [SYMTABSIZE],
			*last;
char *eval_ptr;
int symcount, binlast, eval_error;

extern int pauselinecount;

void syminit (void)
{
	memset (table,0,SYMTABSIZE * sizeof (struct symtab *));
	memset (vtable,0,SYMTABSIZE * sizeof (struct symtab *));
	memset (bintable,0,SYMTABSIZE * sizeof (struct symtab *));
	symcount = 0;
}

/* do_sd implements symbol display command */

int do_sd (int argc, char **argv)
{
	int temp, limit = symcount - 1, counter = 0;

	if (!symcount)
	{
		pf ("No symbols defined\n");
		return 0;
	}
	if (argc > 1)
	{
		counter = binsearch (argv [1]);
		if (binlast > 0) counter++;
	}
	if (argc > 2)
	{
		limit = binsearch (argv [2]);
		if (binlast > 0) limit++;
	}
	counter = min (counter, symcount - 1);
	limit = min (limit, symcount - 1);
	counter = max (counter, 0);
	limit = max (limit, 0);
	if (counter > limit)
	{
		temp = counter;
		counter = limit;
		limit = temp;
	}
	pauselinecount = 0;
	for (;counter <= limit; counter++)
	{
		if (check_esc ()) return 0;
		pf ("$%08lX\t%s\n", bintable [counter]->value, bintable [counter]->name);
		if (pause_check ()) break;
	}
	return 0;
}

/* binsearch returns pointer to record which most closely matches parameter
 * value returned is <0 or >0 to indicate relationship (<= or >)
 */

int binsearch (char *search)
{
	int low = 0, mid, high;

	if (!symcount)
	{
		binlast = -1;
		return 0;
	}
	for (high = symcount - 1; low <= high;)
	{
		mid = (low + high)/2;
		if (!(binlast = strcmp (search, bintable[mid]->name))) return mid;
		else if (binlast < 0) high = mid - 1;
		else low = mid + 1;
	}
	return mid;
}

/* vhash returns hash value based on 32-bit number (value we are looking for)
 */

unsigned vhash (LONG number, unsigned size)
{
	unsigned c,value;

	for (value = c = 0; c < 4; c++)
	{
		value = (value << 1) + ((short) number | 0xff);
		number >>= 8;
	}
	return value % size;
}

/* hash returns hash value of string, given modulo size of table */

unsigned hash (char *string, unsigned size)
{
	unsigned c,value;

	for (value = 0; (c = *string++) != '\0';)
		value = (value << 1) + c;
	return value % size;
}

/* valfind finds symbol which has the given value, or NULL if not found */

struct symtab *valfind (LONG value)
{
	struct symtab *current;

	last = current = vtable [vhash (value,SYMHASH)];
	while (current)
		if (current->value == value) return current;
		else
		{
			last = current;
			current = current->nextval;
		}
	return current;
}

/* symfind returns pointer to symbol table entry, or NULL if not found */

struct symtab *symfind (char *name_)
{
	struct symtab *current;
	char name [NAMESIZE];

	strncpy (name,name_,NAMESIZE-1);
	name [NAMESIZE-1] = '\0';
	last = current = table [hash (name,SYMHASH)];
	while (current)
		if (!strcmp (current->name, name)) return current;
		else
		{
			last = current;
			current = current->next;
		}
	return current;
}

/* vlink links symbol into value chain
 * assumes that the symbol is not already value-linked
 */

static void vlink (struct symtab *symbol)
{
	struct symtab *ptr, *last;
	unsigned which;

	ptr = vtable [which = vhash (symbol->value,SYMHASH)];
	for (last = NULL; ptr; ptr = ptr->nextval)
		last = ptr;
	ptr = last;
	if (!ptr) vtable [which] = symbol;
	else ptr->nextval = symbol;
	symbol->lastval = ptr;
	symbol->nextval = NULL;
}

/* addbin adds symbol to binary table */

void addbin (struct symtab *sym)
{
	int where;

	where = binsearch (sym->name);
	if (binlast > 0) where++;
	if (where < symcount)
		memmove (&bintable [where + 1],
			&bintable [where],
			(symcount - where) * sizeof (struct symtab *));
	bintable [where] = sym;
	symcount++;
}

/* symadd adds symbol and value to symbol table
 * if symbol is already there, it is updated
 * returns address of symbol descriptor
 * returns NULL if error occurs (eg memory allocation etc)
 */

struct symtab *symadd (char *name_, LONG value)
{
	char name [NAMESIZE];
	struct symtab *where,**ptr;

	strncpy (name,name_,NAMESIZE-1);
	name [NAMESIZE-1] = '\0';
	if ((where = symfind (name)) != (struct symtab *)0)
	{
		where->value = value;
		if (where->lastval)
			where->lastval->nextval = where->nextval;
		if (where->nextval)
			where->nextval->lastval = where->lastval;
		vlink (where);
		return where;
	}
	if (!last) ptr = &table [hash (name,SYMHASH)];
	else ptr = &last->next;
	if (!(where = *ptr = (struct symtab *) malloc (sizeof (struct symtab)))) return NULL;
	strcpy (where->name,name);
	where->value = value;
	where->next = (struct symtab *) NULL;
	vlink (where);
	addbin (where);
	return where;
}

/* searchtab returns index into table of char pointers
 * which matches parameter
 */

int searchtab (char **look,char *table [])
{
	int tptr;
	char ok, *c1, *c2;

	for (tptr = 0; (c2 = table [tptr]) != (char *) 0; tptr++)
	{
		for (ok = 1,c1 = *look; *c2;)
			if (*c1++ != *c2++)
			{
				ok = 0;
				break;
			}
		if (ok) break;
	}
	if (ok) *look = c1;
	return ok ? tptr + 1 : 0;
}

/* isopchar returns non-zero if c is a legal operator character */

int isopchar (char c)
{
	return c && (int) strchr ("<>=!^&|/*+-%()~",c);
}

/* getoperator looks for legal operator
 * returns the code for the operator found, or zero if nothing found
 */

char last_operator;

int getoperator (void)
{
	skipwhite (&eval_ptr);
	if (!isopchar (*eval_ptr)) return 0;
	last_operator = *eval_ptr;
	return searchtab (&eval_ptr, optable);
}

/* issym1 returns non-zero if c is legal first character of symbol */

int issym1 (char c)
{
	return (isalpha (c) || c == '_');
}

/* issym returns non-zero if c is legal non-first character of symbol */

int issym (char c)
{
	return (isalnum (c) || c == '_');
}

/* getconstant gets constant numeric string and places value at where
 * handles decimal, hex constants
 */

void getconstant (LONG *where)
{
	LONG acc;
	int increment;

	increment = 0;
	acc = 0;
	skipwhite (&eval_ptr);
	if (*eval_ptr == '$')
		increment = 1;
	else if (*eval_ptr == '0' && toupper (eval_ptr[1]) == 'X')
		increment = 2;
	if (increment)
	for (eval_ptr += increment; isxdigit (*eval_ptr); eval_ptr++)
	{
		acc <<= 4;
		acc += htoc (*eval_ptr);
	}
	else for (;isdigit (*eval_ptr); eval_ptr++)
	{
		acc *= 10;
		acc += htoc (*eval_ptr);
	}
	*where = acc;
}

int issymbol (char **string, char *buff)
{
	char *ptr;
	int ctr;

	skipwhite (string);
	if (!issym1 (**string)) return 0;
	*buff = *(*string)++;
	for (ctr = 1,ptr = buff + 1; issym (**string); ctr++)
		*ptr++ = *(*string)++;
	*ptr = '\0';
	return ctr;
}

extern struct RegList regs [];

unsigned IsRegisterName (char **Symbol)
{
	char temp [5], *From, *To;
	struct RegList *Array;

	for (From = *Symbol, To = temp; isalnum (*From);)
	{
		*To++ = toupper (*From++);
		if (To - temp > 3) return 0;
	}
	while (To - temp < 3) *To++ = ' ';
	*To = '\0';
	for (Array = regs; (To = Array->RegName) != (char *) 0; Array++)
		if (!strcmp (To, temp)) break;
	if (To)
	{
		*Symbol = From;
		return Array->RegCode + 1;
	}
	return 0;
}

/* getsymbol gathers up characters and searches symbol table for match
 * returns non-zero if legal symbol found;
 * eval_error set if symbol not found in symbol table
 */

char last_symbol [TOKENSIZE];

getsymbol (LONG *where)
{
	char symbol [TOKENSIZE];
	unsigned RegCode;
	struct symtab *tptr;

	if ((RegCode = IsRegisterName (&eval_ptr)) != 0)
	{
		*where = GETREG (RegCode - 1);
		return 0;
	}
	if (!issymbol (&eval_ptr,symbol)) return 0;
	strcpy (last_symbol, symbol);
	if (!(tptr = symfind (symbol))) return SYM_ERROR;
	else *where = tptr->value;
	return 0;
}

LONG eval (char *);

int unary_term (LONG *where)
{
	int i,error;

	error = 0;
	if ((i = getoperator ()) != 0)
	{
		if (i != OPEN_OP) return OP_ERROR;
		if ((error = eval_ (where)) != 0) return error;
		return (getoperator () != CLOSE_OP) ? OP_ERROR : 0;
	}
	skipwhite (&eval_ptr);
	if (isdigit (*eval_ptr)
		|| ((*eval_ptr == '$') && isxdigit (*(eval_ptr+1))))
	{
		getconstant (where);
		return 0;
	}
	if (issym1 (*eval_ptr)) return getsymbol (where);
	return CHAR_ERROR;
}

int mul_term (LONG *where)
{
	LONG lterm;
	int error,i,opsize;
	char *save,*which;
	static char ops [] = {"BWL"};

	error = 0;
	save = eval_ptr;
	if ((i = getoperator ()) != 0)
	{
		if (i != NOT_OP && i != MINUS_OP && i != MUL_OP && i != INV_OP)
		{
			eval_ptr = save;
			return unary_term (where);
		}
		if ((error = mul_term (&lterm)) != 0) return error;
		switch (i)
		{
			case    NOT_OP:
			*where = !lterm;
			break;

			case    MINUS_OP:
			*where = -lterm;
			break;

			case    INV_OP:
			*where = ~lterm;
			break;

			default:
			opsize = 1;
			if (*eval_ptr == '.')
			{
				eval_ptr++;
				if (!(which = strchr (ops,toupper (*eval_ptr))))
					return OP_ERROR;
				opsize = which - ops;
				eval_ptr++;
			}
			*where = GETMEM (lterm);
		}
		return 0;
	}
	return unary_term (where);
}

int add_term (LONG *where)
{
	LONG rterm;
	int i,error;
	char *save;

	if ((error = mul_term (where)) != 0) return error;
	for (;;)
	{
		save = eval_ptr;
		i = getoperator ();
		if (i != MUL_OP && i != DIV_OP && i != REM_OP)
		{
			eval_ptr = save;
			break;
		}
		if ((error = mul_term (&rterm)) != 0) return error;
		switch (i)
		{
			case    MUL_OP:
			*where *= rterm;
			break;

			case    DIV_OP:
			*where /= rterm;
			break;

			default:
			*where %= rterm;
			break;
		}
	}
	return 0;
}

int shift_term (LONG *where)
{
	LONG rterm;
	int i,error;
	char *save;

	if ((error = add_term (where)) != 0) return error;
	for (;;)
	{
		save = eval_ptr;
		i = getoperator ();
		if (i != PLUS_OP && i != MINUS_OP)
		{
			eval_ptr = save;
			break;
		}
		if ((error = add_term (&rterm)) != 0) return error;
		*where = (i == PLUS_OP) ? *where + rterm : *where - rterm;
	}
	return 0;
}

int comp_term (LONG *where)
{
	LONG rterm;
	int i,error;
	char *save;

	if ((error = shift_term (where)) != 0) return error;
	for (;;)
	{
		save = eval_ptr;
		i = getoperator ();
		if (i != SL_OP && i != SR_OP)
		{
			eval_ptr = save;
			break;
		}
		if ((error = shift_term (&rterm)) != 0) return error;
		*where = (i == SL_OP) ? *where << rterm : *where >> rterm;
	}
	return 0;
}

int eq_term (LONG *where)
{
	LONG rterm;
	int i,error;
	char *save;

	if ((error = comp_term (where)) != 0) return error;
	for (;;)
	{
		save = eval_ptr;
		i = getoperator ();
		if (i != G_OP && i != GE_OP
		&& i != L_OP && i != LE_OP)
		{
			eval_ptr = save;
			break;
		}
		if ((error = comp_term (&rterm)) != 0) return error;
		switch (i)
		{
			case    G_OP:
			*where = *where > rterm;
			break;

			case    GE_OP:
			*where = *where >= rterm;
			break;

			case    L_OP:
			*where = *where < rterm;
			break;

			default:
			*where = *where <= rterm;
			break;
		}
	}
	return 0;
}

int and_term (LONG *where)
{
	LONG rterm;
	int i,error;
	char *save;

	if ((error = eq_term (where)) != 0) return error;
	for (;;)
	{
		save = eval_ptr;
		i = getoperator ();
		if (i != EQ_OP && i != NE_OP)
		{
			eval_ptr = save;
			break;
		}
		if ((error = eq_term (&rterm)) != 0) return error;
		*where = (i == EQ_OP) ? *where == rterm : *where != rterm;
	}
	return 0;
}

int eor_term (LONG *where)
{
	LONG rterm;
	int error;
	char *save;

	if ((error = and_term (where)) != 0) return error;
	for (;;)
	{
		save = eval_ptr;
		if (getoperator () != AND_OP)
		{
			eval_ptr = save;
			break;
		}
		if ((error = and_term (&rterm)) != 0) return error;
		*where &= rterm;
	}
	return 0;
}

int or_term (LONG *where)
{
	LONG rterm;
	int error;
	char *save;

	if ((error = eor_term (where)) != 0) return error;
	for (;;)
	{
		save = eval_ptr;
		if (getoperator () != EOR_OP)
		{
			eval_ptr = save;
			break;
		}
		if ((error = eor_term (&rterm)) != 0) return error;
		*where ^= rterm;
	}
	return 0;
}

/* eval evaluates expression contained in string
 * returns long result
 * global eval_error is non-zero if error occurred
 */

LONG eval (char *string)
{
	LONG lterm = 0;
	char *save;
	int i;

	eval_error = 0;
	eval_ptr = string;
	for (;;)
	{
		if ((eval_error = eval_ (&lterm)) != 0) break;
		save = eval_ptr;
		skipwhite (&eval_ptr);
		if (!*eval_ptr) break;
		if (!isopchar (*eval_ptr))
		{
			eval_error = CHAR_ERROR;
			break;
		}
		if ((i = getoperator ()) != 0)
		{
			if (i != OR_OP)
			{
				eval_error = OP_ERROR;
				break;
			}
			eval_ptr = save;
		}
	}
	return (lterm);
}

int eval_ (LONG *where)
{
	LONG rterm;
	int error;
	char *save;

	skipwhite (&eval_ptr);
	if (!*eval_ptr) return NOCHAR_ERROR;
	if ((error = or_term (where)) != 0) return error;
	for (;;)
	{
		save = eval_ptr;
		if (getoperator () != OR_OP)
		{
			eval_ptr = save;
			break;
		}
		if ((error = or_term (&rterm)) != 0) return error;
		*where |= rterm;
	}
	return 0;
}
