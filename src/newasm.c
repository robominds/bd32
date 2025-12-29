/* newasm.c - assembler for BD32new */

#include	<conio.h>
#include	<stdio.h>
#include	<string.h>
#include        "didefs.h"
/* #include        "didecls.h" */
#include        "dasmdefs.h"
#include        "symbol.h"
#include	"disp-mac.h"
#include	"target.h"

#include	"prog-def.h"
#include	"scrninfo.h"
#include	"bd32new.h"
#include	"newasm.h"
#include	"newsym.h"

#define HOST

#ifdef  HOST
#include        <stdio.h>
#include        <ctype.h>
#undef          toupper
#endif

#define MAXFIELD        30
#define PC_FIELD        26
#define CTOI(x)         ((x) == '.' ? currvar : (x) - 'a')

LONG fields [MAXFIELD];

extern LONG sign_extend (LONG, int),disasm (LONG);
extern int eval_error;
char outputting;
char *asmin;
char *asmout;
int block_level,opt_level;
static LONG pcsave = 0;
extern char last_cmd [];

static char outfield;
static count;
static LONG outstuff,mask, fieldmasks [MAXFIELD];
BYTE fielddir [MAXFIELD];

static WORD opcode,outmask;

#define currvar fields [1]
#define FAIL    0
#define MATCH   1
#define NOMATCH 2
#define QUIT    4
#define ERROR   8

struct matchstrings {
char *before,*after,*strings [20];
} substrings [] = {

/* 0 - all modes (source) */

{       "%5,1g","",
{       "D[0-7]",
	"A[0-7]",
	"%(A[0-7]%)%!+",
	"%(A[0-7]%)+",
	"-%(A[0-7]%)",
	"%7,1g%e%-32768,9g%7,8p%9,8&%8?(%8~%8,9&%9?(%0q))%(%6,1gA[0-7]%)",
	"%7,1g%e%(<Z%1,12g>%6,1gA[0-7],<Z%1,13g>%8,1g[DA][0-7]<.[WL]>%14,1g<*[1248]>%)",
	"%7,1g%e.W",
	"%-1,5g%1,6g%7,1g%e%!%(<.L>",
	"%-2,5g%2,6g%7,1g%e%-32768,9g%7,8p%-2,8a%0,8-%9,8&%8?(%8~%8,9&%9?(%0q))%(PC%)",
	"%-3,5g%3,6g%7,1g%e%(<Z%1,12g>PC,<Z%1,13g>%8,1g[DA][0-7]<.[WL]>%14,1g<*[1248]>%)",
	"%-4,5g%4,6g%7,1g#%e",
	(char *) NULL
}},

/* 1 - data modes (source) */

{       "%5,1g","",
{       "D[0-7]",
	"\x7f",
	"%(A[0-7]%)%!+",
	"%(A[0-7]%)+",
	"-%(A[0-7]%)",
	"%7,1g%e%-32768,9g%7,8p%9,8&%8?(%8~%8,9&%9?(%0q))%(%6,1gA[0-7]%)",
	"%7,1g%e%(<Z%1,12g>%6,1gA[0-7],<Z%1,13g>%8,1g[DA][0-7]<.[WL]>%14,1g<*[1248]>%)",
	"%7,1g%e.W",
	"%-1,5g%1,6g%7,1g%e%!%(<.L>",
	"%-2,5g%2,6g%7,1g%e%-32768,9g%7,8p%-2,8a%0,8-%9,8&%8?(%8~%8,9&%9?(%0q))%(PC%)",
	"%-3,5g%3,6g%7,1g%e%(<Z%1,12g>PC,<Z%1,13g>%8,1g[DA][0-7]<.[WL]>%14,1g<*[1248]>%)",
	"%-4,5g%4,6g%7,1g#%e",
	(char *) NULL
}},

/* 2 - data alterable (dest) */

{       "%15,1g","",
{       "D[0-7]",
	"\x7f",
	"%(A[0-7]%)%!+",
	"%(A[0-7]%)+",
	"-%(A[0-7]%)",
	"%17,1g%e%-32768,19g%17,18p%19,18&%18?(%18~%18,19&%19?(%0q))%(%16,1gA[0-7]%)",
	"%17,1g%e%(<Z%1,22g>%16,1gA[0-7],<Z%1,23g>%18,1g[DA][0-7]<.[WL]>%24,1g<*[1248]>%)",
	"%17,1g%e.W",
	"%-1,15g%1,16g%17,1g%e%!%(<.L>",
	"\x7f",
	"\x7f",
	"\x7f",
	(char *) NULL
}},

/* 3 - memory modes (source) */

{       "%5,1g","",
{       "\x7f",
	"\x7f",
	"%(A[0-7]%)%!+",
	"%(A[0-7]%)+",
	"-%(A[0-7]%)",
	"%7,1g%e%-32768,9g%7,8p%9,8&%8?(%8~%8,9&%9?(%0q))%(%6,1gA[0-7]%)",
	"%7,1g%e%(<Z%1,12g>%6,1gA[0-7],<Z%1,13g>%8,1g[DA][0-7]<.[WL]>%14,1g<*[1248]>%)",
	"%7,1g%e.W",
	"%-1,15g%1,16g%17,1g%e%!%(<.L>",
	"%-2,15g%2,16g%17,1g%e%-32768,19g%17,18p%-2,18a%0,18-%19,18&%18?(%18~%8,19&%19?(%0q))%(PC%)",
	"%-3,15g%3,16g%17,1g%e%(<Z%1,22g>PC,<Z%1,23g>%18,1g[DA][0-7]<.[WL]>%24,1g<*[1248]>%)",
	"%-4,15g%4,16g%17,1g#%e",
	(char *) NULL
}},

/* 4 - memory alterable modes (dest) */

{       "%15,1g","",
{       "\x7f",
	"\x7f",
	"%(A[0-7]%)%!+",
	"%(A[0-7]%)+",
	"-%(A[0-7]%)",
	"%17,1g%e%-32768,19g%17,18p%19,18&%18?(%18~%18,19&%19?(%0q))%(%16,1gA[0-7]%)",
	"%17,1g%e%(<Z%1,22g>%16,1gA[0-7],<Z%1,23g>%18,1g[DA][0-7]<.[WL]>%24,1g<*[1248]>%)",
	"%17,1g%e.W",
	"%-1,15g%1,16g%17,1g%e%!%(<.L>",
	"\x7f",
	"\x7f",
	"\x7f",
	(char *) NULL
}},

/* 5 - control modes (dest) */

{       "%15,1g","",
{       "\x7f",
	"\x7f",
	"%(A[0-7]%)%!+",
	"\x7f",
	"\x7f",
	"%17,1g%e%-32768,19g%17,18p%19,18&%18?(%18~%18,19&%19?(%0q))%(%16,1gA[0-7]%)",
	"%17,1g%e%(<Z%1,22g>%16,1gA[0-7],<Z%1,23g>%18,1g[DA][0-7]<.[WL]>%24,1g<*[1248]>%)",
	"%17,1g%e.W",
	"%-1,15g%1,16g%17,1g%e%!%(<.L>",
	"%-2,15g%2,16g%17,1g%e%-32768,19g%17,18p%-2,18a%0,18-%19,18&%18?(%18~%18,19&%19?(%0q))%(PC%)",
	"%-3,15g%3,16g%17,1g%e%(<Z%1,22g>PC,<Z%1,23g>%18,1g[DA][0-7]<.[WL]>%24,1g<*[1248]>%)",
	"\x7f",
	(char *) NULL
}},

/* 6 - branch/branch conditional/BSR conditions */

{       "","",
{       "RA",
	"SR",
	"HI",
	"LS",
	"CC",
	"CS",
	"NE",
	"EQ",
	"VC",
	"VS",
	"PL",
	"MI",
	"GE",
	"LT",
	"GT",
	"LE",
	(char *) NULL
}},

/* 7 - DBcc/TRAP conditions */

{       "","",
{       "T",
	"F",
	"HI",
	"LS",
	"CC",
	"CS",
	"NE",
	"EQ",
	"VC",
	"VS",
	"PL",
	"MI",
	"GE",
	"LT",
	"GT",
	"LE",
	(char *) NULL
}}
};

char *outsubs [] =
{                                       /* 0 - 11: addressing modes */

"%1,25g",                                             /* Dn */
"%1,25g",                                             /* An */
"%1,25g",                                             /* (An) */
"%1,25g",                                             /* (An)+ */
"%1,25g",                                             /* -(An) */
"%25?(%17,7p)h*8h*8%1,25g",                           /* d16(An) */

	/* d8(An,Xn.W/L) or bd(An,Xn.W/L) */

"%25?(%15dstttuyyp%15?(wxqq0000%-1,16a%16?(r*8r*8%-1,16a%16?(r*8r*8))%1q)r*8%1q)%1,25g%5dijjjkoof%5?(mngg0000%-1,6a%6?(h*8h*8%-1,6a%6?(h*8h*8))%1q)h*8",
"%25?(%16,6p)%6?(%7,6a%6x%1,25g%1q)%25?(%17,7p)h*8h*8%1,25g",   /* xxx.W */
"%25?(%17,7p)h*8h*8h*8h*8%1,25g",                               /* xxx<.L> */
"%25?(%17,7p)%26,7-%-2,7ah*8h*8%1,25g",                          /* d16(PC) */

	/* d8(PC,Xn.W/L) or bd(PC,Xn.W/L) */

"%25?(%15dstttuyyp%15?(wxqq0000%-1,16a%16?(r*8r*8%-1,16a%16?(r*8r*8))%1q)r*8%1q)%1,25g%5dijjjkoof%5?(mngg0000%-1,6a%6?(h*8h*8%-1,6a%6?(h*8h*8))%1q)h*8",
"%25?(%17,7p)%1,25g%2?(h*8h*8%-1,2a%2?(h*8h*8)%1q)0*8h*8",    /* #xxx */

			/* 12 and up - miscellaneous substrings */
			/* 12 does immediate data using current variable */
			/* var. 2 must hold size code */

"%25?(%17,7p)%1,25g%2?(.*8.*8%-1,2a%2?(.*8.*8)%1q)0*8.*8",    /* #xxx */

			/* 13 does branch offset calculation */
			/* var. 3 holds target address */
			/* 4,5 used for scratchpad */

"%3,4p%4,1g%0-%-2a%4,5c%5?(%-1,5a%5?(1*8.*8.*8.*8.*8%1q)0*8.*8.*8%1q).*8%4?(%1q)0*80*8",

			/* 14 does register list for MOVEM to memory */
			/* var 3 holds [AD], 4 holds [0-7] */
			/* result goes into var 11 */

"%1,6g%8,7g%7,5g%4,5-%5,6<%3?(%7,6<)%6,11|%3,1g",

			/* 15 does register list for MOVEM from memory */
			/* var 3 holds [DA], 4 holds [0-7] */
			/* result goes into var 11 */

"%1,6g%8,7g%4,6<%3?(%7,6<)%6,11|%3,1g"
};

/* sizetest returns size code coresponding to min size that can represent stuff */

int sizetest (LONG stuff, int issigned)
{
	if (!issigned)
	{
		if (!(stuff & 0xffffff00L) || ((stuff & 0xffffff00L) == 0xffffff00L))
			return BYTESIZE;
		if (!(stuff & 0xffff0000L) || ((stuff & 0xffff0000L) == 0xffff0000L))
			return WORDSIZE;
		return LONGSIZE;
	}
	if (!(stuff & 0xffffff80L) || ((stuff & 0xffffff80L) == 0xffffff80L))
		return BYTESIZE;
	if (!(stuff & 0xffff8000L) || ((stuff & 0xffff8000L) == 0xffff8000L))
		return WORDSIZE;
	return LONGSIZE;
}

int getconst (char **where, LONG *data)
{
	int psign;

	psign = 0;
	if (!(**where == '-' || isdigit (**where))) return 0;
	if (**where == '-')
	{
		psign++;
		(*where)++;
	}
	while (isdigit (**where))
	{
		*data *= 10;
		*data += *((*where)++) - '0';
	}
	if (psign) *data = -*data;
	return 1;
}

int asm_expr (void)
{
	LONG stuff;
	int error;
	extern char *eval_ptr;

	eval_ptr = asmin;
	if ((error = eval_ (&stuff)) != 0)
	{
		if (error & (SYM_ERROR | OP_ERROR))
		{
			report_error (error,asmin);
			return ERROR;
		}
		else if (error & CHAR_ERROR) return NOMATCH;
	}
	asmin = eval_ptr;
	fields [ (short)currvar++] = stuff;
	return MATCH;
}

int asm_sym (void)
{
	if (!isalpha (*asmin) && *asmin != '_') return (NOMATCH);
	for (asmin++;*asmin && (isdigit (*asmin) || isalpha (*asmin)
			|| *asmin != '_');asmin++);
	if (*asmin == ':') asmin++;
	currvar++;
	return MATCH;
}

int asm_block_1 ();

char set_gc (char **);
void fbitout (LONG , LONG , int );

int asm_percent (void)
{
	char *saveasm,*saveasmin;
	int status,i,gotparam;
	LONG savevar,param,param2;

	gotparam = 0;
	param = param2 = 0L;
	asmout++;
	if ((gotparam = getconst (&asmout, &param)) != 0)
	{
		if (*asmout == ',')
		{
			asmout++;
			if (getconst (&asmout,&param2)) gotparam++;
		}
	}
	switch (tolower (*asmout))
	{
		case    'a':
		if (gotparam == 1) param2 = currvar;
		fields [(short) param2] += param;
		status = MATCH;
		break;

		case    'c':
		if (gotparam == 1) param2 = currvar;
		fields [(short) param2] = sizetest (fields [(short) param],1);
		break;

		case    'd':
		if (fields [(short) param] == 7L)        /* pc relative */
			fields [(short) param+2] -= (fields [PC_FIELD] + 2);
		fields [(short) param] = (fields [(short) param+6] || fields [(short) param+7] || fields [(short) param+8]
			|| sizetest (fields [(short) param+2],0) > BYTESIZE) ? 1 : 0;
		if (!fields [(short) param+2]) fields [(short) param+1] = 1;
		else if ((fields [(short) param+1] = 1 + sizetest (fields [(short) param+2],0)) == BYTESIZE+1)
			fields [(short) param+1] = 2;
		status = MATCH;
		break;

		case    'e':
		status = asm_expr ();
		break;

		case    'g':
		if (gotparam)
		{
			if (gotparam == 1) param2 = currvar;
			fields [(short) param2] = param;
		}
		else if (getconst (&asmin,&param))
			fields [(short) currvar] = param;
		status = MATCH;
		break;

		case    'm':
		saveasm = asmout;
		saveasmin = asmin;
		status = MATCH;
		for (asmout = substrings [(short) param].before; *asmout;)
			if (!((status = asm_block ()) & MATCH)) break;
		if (!(status & MATCH)) break;
		fields [(short) savevar = currvar] = 0;
		for (status=i=0; substrings [(short) param].strings[i]; i++)
		{
			currvar = savevar + 1;
			asmin = saveasmin;
			for (asmout = substrings [(short) param].strings[i]; *asmout;)
				if (!((status = asm_block ()) & MATCH)) break;
			if (status & (MATCH | ERROR)) break;
		}
		if (status & MATCH) fields [(short) savevar] += i;
		else if (status & ERROR) break;
		for (asmout = substrings [(short) param].after; *asmout;)
			if (!((status = asm_block ()) & MATCH)) break;
		asmout = saveasm;
		break;

		case    'p':
		if (gotparam == 1) param2 = currvar;
		fields [(short) param2] = fields [(short) param];
		status = MATCH;
		break;

		case    'q':
		if (outfield)
		{
			fbitout (fields [(short) CTOI(outfield)],mask,count);
			outfield = '\0';
		}
		if (gotparam && param)
		{
			status = MATCH | QUIT;
			break;
		}
		else return NOMATCH;

		case    's':
		asm_sym ();
		status = MATCH;
		break;

		case    'x':
		saveasm = asmout;
		if (param < MAXFIELD) asmout = outsubs [(short) fields [(short) param]];
		else asmout = outsubs [(short) param - 30];
		for (; *asmout;)
		{
			if (!((status = asm_block ()) & MATCH)) return status;
			if (status & QUIT) break;
		}
		asmout = saveasm;
		status = MATCH;
		break;

		case    ' ':
		while (isspace (*asmin)) asmin++;
		status = MATCH;
		break;

		case    '?':
		if (!fields [(short) param])
		{
			asmout++;
			savevar = currvar;
			if (skip_block (&asmout) == FAIL) return FAIL;
			currvar = savevar;
			return MATCH;
		}
		status = MATCH;
		break;

		case    '=':
		if (gotparam == 1) param2 = currvar;
		if (fields [(short) param2] != param)
		{
			asmout++;
			savevar = currvar;
			if (skip_block (&asmout) == FAIL) return FAIL;
			currvar = savevar;
			return MATCH;
		}
		status = MATCH;
		break;

		case    '+':
		case    '-':
		if (param >= MAXFIELD) break;
		if (gotparam == 1) param2 = currvar;
		if (*asmout == '+') fields [(short) param2] += fields [(short) param];
		else fields [(short) param2] -= fields [(short) param];
		status = MATCH;
		break;

		case    '>':
		case    '<':
		if (param >= MAXFIELD) break;
		if (gotparam == 1) param2 = currvar;
		if (*asmout == '>') fields [(short) param2] >>= fields [(short) param] & 31;
		else fields [(short) param2] <<= fields [(short) param] & 31;
		status = MATCH;
		break;

		case    '&':
		if (param >= MAXFIELD) break;
		if (gotparam == 1) param2 = currvar;
		fields [(short) param2] &= fields [(short) param];
		status = MATCH;
		break;

		case    '~':
		if (param >= MAXFIELD) break;
		fields [(short) param] = ~fields [(short) param];
		status = MATCH;
		break;

		case    '|':
		if (param >= MAXFIELD) break;
		if (gotparam == 1) param2 = currvar;
		fields [(short) param2] |= fields [(short) param];
		status = MATCH;
		break;

		case    '^':
		if (param >= MAXFIELD) break;
		if (gotparam == 1) param2 = currvar;
		fields [(short) param2] ^= fields [(short) param];
		status = MATCH;
		break;

		case    '%':
		if (param >= MAXFIELD) break;
		if (gotparam == 1) param2 = currvar;
		fields [(short) param2] %= fields [(short) param];
		status = MATCH;
		break;

		case    '/':
		if (param >= MAXFIELD) break;
		if (gotparam == 1) param2 = currvar;
		fields [(short) param2] /= fields [(short) param];
		status = MATCH;
		break;

		case    '!':
		asmout++;
		saveasmin = asmin;
		saveasm = asmout;
		if ((status = asm_block ()) == MATCH)
			return NOMATCH;
		if (status & ERROR) return status;
		asmin = saveasmin;
		asmout = saveasm;
		return skip_block (&asmout);

		case    '\\':
		if (param >= MAXFIELD) break;
		if (gotparam == 1) param2 = currvar;
		fields [(short) param2] = sign_extend (fields [(short) param2],(short) param);
		status = MATCH;
		break;

		default:
		if (*asmin++ != *asmout) status = NOMATCH;
		else status = MATCH;
	}
	asmout++;
	return status;
}

char set_gc (char **s)
{
	if (**s == '%') (*s)++;
	if (**s) return *((*s)++);
	else return '\0';
}

void skip_const (char **s)
{
	if (**s == '-') (*s)++;
	while (isdigit (**s)) (*s)++;
}

int skip_percent (char **s)
{
	(*s)++;
	skip_const (s);
	if (**s == ',')
	{
		(*s)++;
		skip_const (s);
	}
	if (**s)
	{
		(*s)++;
		return MATCH;
	}
	return FAIL;
}

int skip_set (char **s)
{
	if (**s == '[') (*s)++;
	for (;**s != ']';(*s)++)
		if (!**s) return FAIL;
	(*s)++;
	currvar++;
	return MATCH;
}

int skip_block_1 (char end,char **s)
{
	char c;

	if (end)
	{
		while (**s && **s != end)
			if (skip_block (s) == FAIL) return FAIL;
		(*s)++;
		return MATCH;
	}
	switch (**s)
	{
		case    '<':
		c = '>';
		goto caseparen;

		case    '(':
		c = ')';
caseparen:      (*s)++;
		while (**s && **s != c)
			if (skip_block (s) == FAIL) return FAIL;
		(*s)++;
		break;

		case    '>':
		case    ')':
		print ("Unmatched ");
		pc (**s);
		print (" in input spec\n");
		return FAIL;

		case    '%':
		return skip_percent (s);

		case    '[':
		return skip_set (s);

		default:
		(*s)++;
	}
	return MATCH;
}

int skip_block (char **s)
{
	int status;

	status = skip_block_1 ('\0',s);
	if (status == FAIL) return status;
	if (**s == '*')
		if (isdigit (*(*s+1))) *s += 2;
	return status;
}

int asm_set (void)
{
	char c, c1, c2;
	int count;

	for (count = (short) fields [(short) currvar] = 0, asmout++;
		*asmout && *asmout != ']';count++)
	{
		c1 = toupper (set_gc (&asmout));
		if (*asmout == '-')
		{
			asmout++;
			c2 = toupper (set_gc (&asmout));
			c = toupper (*asmin);
			if (c >= c1 && c <= c2)
			{
				fields [(short) currvar] = count + *asmin - c1;
				if (skip_set (&asmout) == FAIL) return FAIL;
				asmin++;
				return MATCH;
			}
			count += c2 - c1;
		}
		else if (c1 == toupper (*asmin))
		{
			fields [(short) currvar] = count;
			if (skip_set (&asmout) == FAIL) return FAIL;
			asmin++;
			return MATCH;
		}
	}
	currvar++;
	return NOMATCH;
}

int asm_block (void)
{
	char *save;
	int status,count;
        LONG varsave;

	save = asmout;
	status = asm_block_1 ();
	if (status & QUIT || !(status & MATCH)) return status;
	if (*asmout == '*')
		if (isdigit (*(asmout+1)))
		{
			asmout++;
			for (count = *asmout++ - '0' - 1;count;count--)
			{
				asmout = save;
				status = asm_block_1 ();
				if (status & QUIT || !(status & MATCH))
					return status;
			}
			asmout = save;
			varsave = currvar;
			if ((status = skip_block (&asmout)) == FAIL) return FAIL;
			else if (status & ERROR) return status;
			currvar = varsave;
		}
	return status;
}

int asm_block_1 (void)
{
	int status;
        LONG savevar;
	char *saveasmin;

	switch (*asmout)
	{
		case    ' ':
		if (!isspace (*asmin)) return NOMATCH;
		while (isspace (*asmin)) asmin++;
		asmout++;
		break;

		case    '<':
		opt_level++;
		saveasmin = asmin;
		for (asmout++;;)
		{
			if (*asmout == '>')
			{
				asmout++;
				status = MATCH;
				break;
			}
			if ((status = asm_block ()) == FAIL)
			{
				print ("Unmatched '<' in input spec\n");
				return FAIL;
			}

			else if (status & (NOMATCH | QUIT))
			{
				asmin = saveasmin;
				status = MATCH | (status & ~NOMATCH);
				if (skip_block_1 ('>',&asmout) == FAIL) return FAIL;
				break;
			}
		}
		opt_level--;
		return status;

		case    '(':
		block_level++;
		for (asmout++;;)
		{
			if (*asmout == ')')
			{
				asmout++;
				status = MATCH;
				break;
			}
			if ((status = asm_block ()) == FAIL)
			{
				print ("Unmatched '(' in input spec\n");
				return FAIL;
			}
			if (!(status & MATCH)) return status;
			else if (status & QUIT)
			{
				savevar = currvar;
				if (skip_block_1 (')',&asmout) == FAIL) return FAIL;
				currvar = savevar;
				break;
			}
		}
		block_level--;
		return status;

		case    '>':
		case    ')':
		print ("Unmatched ");
		pc (*asmout);
		print (" in input spec\n");
		return FAIL;

		case    '%':
		return asm_percent ();

		case    '[':
		return asm_set ();

		default:
		if (outputting) return sbitout (*asmout++);
		if (*asmout != toupper (*asmin)) return (NOMATCH);
		asmout++;
		asmin++;
	}
	return MATCH;
}

struct match {
	char *in,*out;
	} asmtable [] = {
	{ "ABCD<.B> D[0-7],D[0-7]% ","1100ddd100000ccc" },      /* asmtable */
	{ "ABCD<.B> -%(A[0-7]%),-%(A[0-7]%)% ","1100ddd100001ccc" },
	{ "ADD%1g<.[BWL]> %0m,%11,1gD[0-7]% ","1101lll0ccfffggg%5x" },
	{ "ADD%1g<.[BWL]> D[0-7],%4m% ","1101ddd1ccpppqqq%1,25g%15x" },
	{ "ADDA%1g<.[\x7fWL]>%2,3p%-1,3a %0m,%11,1gA[0-7]% ","1101llld11fffggg%5x" },
	{ "ADDI%1g<.[BWL]> #%e,%2m% ","00000110ccpppqqq%3,1g%42x%1,25g%15x" },
	{ "ADDQ%1g<.[BWL]> #%e,%2m% ","0101ddd0ccpppqqq%1,25g%15x" },
	{ "ADDQ%1g<.[\x7fWL]> #%e,A[0-7]% ","0101ddd0cc001eee" },
	{ "ADDX%1g<.[BWL]> D[0-7],D[0-7]% ","1101eee1cc000ddd" },
	{ "ADDX%1g<.[BWL]> -%(A[0-7]%),-%(A[0-7]%)% ","1101eee1cc001ddd" },
	{ "AND%1g<.[BWL]> %0m,%11,1gD[0-7]% ","1100lll0ccfffggg%5x" },
	{ "AND%1g<.[BWL]> D[0-7],%4m% ","1100ddd1ccpppqqq%1,25g%15x" },
	{ "ANDI<.B>%0g%3,1g #%e,CCR% ","0000001000111100%3,1g%42x" },
	{ "ANDI<.W>%1g%3,1g #%e,SR% ","0000001001111100%3,1g%42x" },
	{ "ANDI%1g<.[BWL]> #%e,%2m% ","00000010ccpppqqq%3,1g%42x%1,25g%15x" },
	{ "AS[RL]%1g<.[BWL]> D[0-7],D[0-7]% ","1110eeecdd100fff" },
	{ "AS[RL]%1g<.[BWL]> #%e,D[0-7]% ","1110eeecdd000fff" },
	{ "AS[RL]%1g<.[BWL]> %4m% ","1110000c11pppqqq%1,25g%15x" },
	{ "B%6m %e% ","0110cccc%43x" },                 /* 18 */
	{ "B%6m %e.B% ","0110cccc%3,1g%0-%-2a.*8" },
	{ "B%6m %e.W% ","0110cccc%3,1g%0-%-2a0*8.*8.*8" },
	{ "B%6m %e.L% ","0110cccc%3,1g%0-%-2a1*8.*8.*8.*8.*8" },
	{ "BCHG<.L> D[0-7],D[0-7]% ","0000ccc101000ddd" },
	{ "BCHG<.L> #%e,D[0-7]% ","0000100001000ddd%2,3p%0,2g%3,1g%42x" },
	{ "BCHG<.B> D[0-7],%2m% ","0000ccc101pppqqq%1,25g%15x" },
	{ "BCHG<.B> #%e,%2m% ","0000100001pppqqq%2,3p%0,2g%3,1g%42x%1,25g%15x" },
	{ "BCLR D[0-7],%2m% ","0000ccc110pppqqq%1,25g%15x" },
	{ "BCLR #%e,%2m% ","0000100010pppqqq%2,3p%0,2g%3,1g%42x%1,25g%15x" },
	{ "BGND% ","0100101011111010" },
	{ "BKPT #%e% ","0100100001001ccc" },
	{ "BSET D[0-7],%2m% ","0000ccc111pppqqq%1,25g%15x" },
	{ "BSET #%e,%2m% ","0000100011pppqqq%2,3p%0,2g%3,1g%42x%1,25g%15x" },
	{ "BTST D[0-7],%1m% ","0000ccc100fffggg%5x" },
	{ "BTST #%e,%1m% ","0000100000fffggg%2,3p%0,2g%3,1g%42x%5x" },
	{ "CHK<.W> %1g%1m,%11,1gD[0-7]% ","0100lll110fffggg%5x" },      /* 34 */
	{ "CHK.L %1g%1m,%11,1gD[0-7]% ","0100lll100fffggg%5x" },
	{ "CHK2%1g<.[BWL]> %5m,%3,1g[DA][0-7]% ","00000cc011pppqqqdeee100000000000%1,25g%15x" },
	{ "CLR%1g<.[BWL]> %2m% ","01000010ccpppqqq%1,25g%15x" },
	{ "CMP%1g<.[BWL]> %0m,%11,1gD[0-7]% ","1011lll0ccfffggg%5x" },
	{ "CMPA%1g<.[\x7fWL]>%2,3p%-1,3a %0m,%11,1gA[0-7]","1011llld11fffggg%5x" },
	{ "CMPI%1g<.[BWL]> #%e,%1m% ","00001100ccfffggg%3,1g%42x%5x" },
	{ "CMPM%1g<.[BWL]> %(A[0-7]%)+,%(A[0-7]%)+% ","1011eee1cc001ddd" },
	{ "CMP2%1g<.[BWL]> %5m,%3,1g[DA][0-7]% ","00000cc011pppqqqdeee000000000000%1,25g%15x" },
	{ "DBRA %3,1gD[0-7],%e<.W>% ","%0,4-%-2,4a0101000011001ddde*8e*8" }, /* 43 */
	{ "DB%7m D[0-7],%e<.W>% ","%0,4-%-2,4a0101cccc11001ddde*8e*8" },
	{ "DIV%1g%3,1g[US]<.W> %1m,%11,1gD[0-7]% ","1000llld11fffggg%5x" },
	{ "DIV%1g%3,1g[US].L %1m,%11,1gD[0-7]% ","0100110001fffggg0llld00000000lll%5x" },
	{ "DIV%1g%3,1g[US]<.L> %1m,%11,1gD[0-7]:D[0-7]% ","0100110001fffggg0mmmd10000000lll%5x" },
	{ "DIV%1g%3,1g[US]L<.L> %1m,%11,1gD[0-7]:D[0-7]% ","0100110001fffggg0mmmd00000000lll%5x" },
	{ "EOR%1g<.[BWL]> D[0-7],%2m% ","1011ddd1ccpppqqq%15x" },       /* 49 */
	{ "EORI<.B>%0g%3,1g #%e,CCR% ","0000101000111100%3,1g%42x" },
	{ "EORI<.W>%1g%3,1g #%e,SR% ","0000101001111100%3,1g%42x" },
	{ "EORI%1g<.[BWL]> #%e,%2m% ","00001010ccpppqqq%3,1g%42x%1,25g%15x" },
	{ "EXG<.L> D[0-7],D[0-7]% ","1100ccc101000ddd" },
	{ "EXG<.L> A[0-7],A[0-7]% ","1100ccc101001ddd" },
	{ "EXG<.L> D[0-7],A[0-7]% ","1100ccc110001ddd" },
	{ "EXG<.L> A[0-7],D[0-7]% ","1100ddd110001ccc" },
	{ "EXT<.[WL]> D[0-7]% ","010010001c000ddd" },
	{ "EXTB<.L> D[0-7]% ","0100100111000ccc" },
	{ "ILLEGAL% ","0100101011111100" },             /* 59 */
	{ "JMP %5m% ","0100111011pppqqq%1,25g%15x" },   /* 60 */
	{ "JSR %5m% ","0100111010pppqqq%1,25g%15x" },
	{ "LEA<.L> %5m,%11,1gA[0-7]% ","0100lll111pppqqq%1,25g%15x" }, /* 62 */
	{ "LINK<.W> A[0-7],#%e% ","0100111001010ccc%3,1g%1,2g%42x" },
	{ "LINK.L A[0-7],#%e% ","0100100000001ccc%2,2g%3,1g%42x" },
	{ "LPSTOP #%e% ","11111000000000000000000111000000d*8d*8" },
	{ "LS[RL]%1g<.[BWL]> D[0-7],D[0-7]% ","1110eeecdd101fff" },
	{ "LS[RL]%1g<.[BWL]> #%e,D[0-7]% ","1110eeecdd001fff" },
	{ "LS[RL]%1g<.[BWL]> %4m% ","1110001c11pppqqq%1,25g%15x" },
	{ "MOVEA%1g<.[\x7fWL]> %0m,%11,1gA[0-7]% ","%0,2=(%2,2g)001clll001fffggg%5x" },/* 69 */
	{ "MOVE<.W> CCR,%1g%2m% ","0100001011pppqqq%1,25g%15x" },
	{ "MOVE<.W> SR,%1g%2m% ","0100000011pppqqq%1,25g%15x" },
	{ "MOVE<.L> USP,A[0-7]% ","0100111001101ccc" },
	{ "MOVE<.L> A[0-7],USP% ","0100111001100ccc" },
	{ "MOVE<.W> %1g%1m,CCR% ","0100010011fffggg%5x" },
	{ "MOVE<.W> %1g%1m,SR% ","0100011011fffggg%5x" },
	{ "MOVE<C><.L> SFC,[DA][0-7]% ","0100111001111010cddd0*80*4" },
	{ "MOVE<C><.L> DFC,[DA][0-7]% ","0100111001111010cddd0*80001" },
	{ "MOVE<C><.L> USP,[DA][0-7]% ","0100111001111010cddd10000*8" },
	{ "MOVE<C><.L> VBR,[DA][0-7]% ","0100111001111010cddd1000*81" },
	{ "MOVE<C><.L> [DA][0-7],SFC% ","0100111001111011cddd0*80*4" },
	{ "MOVE<C><.L> [DA][0-7],DFC% ","0100111001111011cddd0*80001" },
	{ "MOVE<C><.L> [DA][0-7],USP% ","0100111001111011cddd10000*8" },
	{ "MOVE<C><.L> [DA][0-7],VBR% ","0100111001111011cddd1000*81" },
	{ "MOVE<.W> %0m,%2m% ","%1,2g0011qqqpppfffggg%5x%15x% " },
	{ "MOVE.L %0m,%2m% ","%2,2g0010qqqpppfffggg%5x%15x% " },
	{ "MOVE.B %0m,%2m% ","%0,2g0001qqqpppfffggg%5x%15x% " },
	{ "MOVEM<.[WL]> %3,1g[AD][0-7]%44x</[AD][0-7]%44x>*7</[AD][0-7]%44x>*8,%3,1g-%(A[0-7]%)% ","010010001c100dddl*8l*8" },
	{ "MOVEM<.[WL]> %3,1g[AD][0-7]%44x</[AD][0-7]%44x>*7</[AD][0-7]%44x>*8,%5m% ","010010001cpppqqql*8l*8%1,25g%15x" },
	{ "MOVEM<.[WL]> %12,1g%(A[0-7]%)+,%3,1g[DA][0-7]%45x</[DA][0-7]%45x>*7</[DA][0-7]%45x>*8% ","010011001c011mmml*8l*8" },
	{ "MOVEM<.[WL]> %5m,%3,1g[DA][0-7]%45x</[DA][0-7]%45x>*7</[DA][0-7]%45x>*8% ","010011001cpppqqql*8l*8%1,25g%15x" },
	{ "MOVEP<.[WL]> D[0-7],%e%(A[0-7]%)% ","0000ddd11c001fffe*8e*8" },
	{ "MOVEP<.[WL]> %e%(A[0-7]%),D[0-7]% ","0000fff10c001eeed*8d*8" },
	{ "MOVEQ<.L> #%e,D[0-7]% ","0111ddd0cccccccc" },
	{ "MOVES%1g<.[BWL]> [DA][0-7],%4m% ","00001110ccpppqqqdeee10000*8%1,25g%15x" },
	{ "MOVES%1g<.[BWL]> %4m,%3,1g[DA][0-7]% ","00001110ccpppqqqdeee00000*8%1,25g%15x" },
	{ "MUL[US]<.W> %1m,%4,1gD[0-7]% ","1100eeec11fffggg%5x" },
	{ "MUL[US].L %1m,%4,1gD[0-7]% ","0100110000fffggg0eeec00000000eee%5x" },
	{ "MUL[US].L %1m,%4,1gD[0-7]:%3,1gD[0-7]% ","0100110000fffggg0dddc10000000eee%5x" },
	{ "NBCD<.B> %2m% ","0100100000pppqqq%1,25g%15x" },      /* 99 */
	{ "NEG%1g<.[BWL]> %2m% ","01000100ccpppqqq%1,25g%15x" },
	{ "NEGX%1g<.[BWL]> %2m% ","01000000ccpppqqq%1,25g%15x" },
	{ "NOP% ","0100111001110001" },
	{ "NOT%1g<.[BWL]> %2m% ","01000110ccpppqqq%1,25g%15x" },
	{ "OR%1g<.[BWL]> %1m,%3,1gD[0-7]% ","1000ddd0ccfffggg%5x" },    /* 104 */
	{ "OR%1g<.[BWL]> D[0-7],%2m% ","1000ddd1ccpppqqq%1,25g%15x" },
	{ "ORI<.B>%0g%3,1g #%e,CCR% ","0000000000111100%3,1g%42x" },
	{ "ORI<.W>%1g%3,1g #%e,SR% ","0000000001111100%3,1g%42x" },
	{ "ORI%1g<.[BWL]> #%e,%2m% ","00000000ccpppqqq%3,1g%42x%1,25g%15x" },
	{ "PEA<.L> %5m% ","0100100001pppqqq%1,25g%15x" },       /* 109 */
	{ "RESET% ","0100111001110000" },                       /* 110 */
	{ "RO[RL]%1g<.[BWL]> D[0-7],D[0-7]% ","1110eeecdd111fff" },
	{ "RO[RL]%1g<.[BWL]> #%e,D[0-7]% ","1110eeecdd011fff" },
	{ "RO[RL]%1g<.[BWL]> %4m% ","1110011c11pppqqq%1,25g%15x" },
	{ "ROX[RL]%1g<.[BWL]> D[0-7],D[0-7]% ","1110eeecdd110fff" },
	{ "ROX[RL]%1g<.[BWL]> #%e,D[0-7]% ","1110eeecdd010fff" },
	{ "ROX[RL]%1g<.[BWL]> %4m% ","1110010c11pppqqq%1,25g%15x" },
	{ "RTD #%e% ","0100111001110100c*8c*8" },
	{ "RTE% ","0100111001110011" },
	{ "RTR% ","0100111001110111" },
	{ "RTS% ","0100111001110101" },
	{ "SBCD<.B> D[0-7],D[0-7]% ","1000ddd100000ccc" },      /* 121 */
	{ "SBCD<.B> -%(A[0-7]%),-%(A[0-7]%)% ","1000ddd100001ccc" },
	{ "S%7m<.B> %2m% ","0101cccc11pppqqq%1,25g%15x" },
	{ "STOP #%e% ","0100111001110010c*8c*8" },
	{ "SUB%1g<.[BWL]> %0m,%11,1gD[0-7]% ","1001lll0ccfffggg%5x" },
	{ "SUB%1g<.[BWL]> D[0-7],%4m% ","1001ddd1ccpppqqq%1,25g%15x" },
	{ "SUBA%1g<.[\x7fWL]>%2,3p%-1,3a %0m,%11,1gA[0-7]% ","1001llld11fffggg%5x" },
	{ "SUBI%1g<.[BWL]> #%e,%2m% ","00000100ccpppqqq%3,1g%42x%1,25g%15x" },
	{ "SUBQ%1g<.[BWL]> #%e,%2m% ","0101ddd1ccpppqqq%1,25g%15x" },
	{ "SUBQ%1g<.[\x7fWL]> #%e,A[0-7]% ","0101ddd1cc001eee" },
	{ "SUBX%1g<.[BWL]> D[0-7],D[0-7]% ","1001eee1cc000ddd" },
	{ "SUBX%1g<.[BWL]> -%(A[0-7]%),-%(A[0-7]%)% ","1001eee1cc001ddd" },
	{ "SWAP<.W> D[0-7]% ","0100100001000ccc" },
	{ "TAS<.B> %2m% ","0100101011pppqqq%1,25g%15x" },       /* 134 */
	{ "%1,3gTBL[US]<N%1a>%4,1g%1g<.[BWL]> %5m,%11,1gD[0-7]% ","1111100000pppqqq0lll1d01ee000000%15x" },
	{ "%1,3gTBL[US]<N%1a>%4,1g%1g<.[BWL]> D[0-7]:D[0-7],D[0-7]% ","1111100000000fff0hhh1d00ee000ggg" },
	{ "TRAP #%e% ","010011100100cccc" },
	{ "TRAPV% ","0100111001110110" },
	{ "TRAP%7m%4,5g<.[\x7fWL] %1,5g%4,1g#%e>% ","%3,5+0101cccc11111fff%3?(%3,2p%4,1g%42x)" },
	{ "TST%1g<.[\x7fWL]> %0m% ","01001010ccfffggg%5x" },
	{ "TST.B %1m% ","0100101000fffggg%5x" },
	{ "UNLK A[0-7]% ","0100111001011ccc" },                 /* 142 */
	{ "","" }                                               /* 143 */
	};

void bitout (int b)
{
	if (!outmask)
	{
		opcode = 0;
		outmask = 0x8000;
	}
	if (b) opcode |= outmask;
	outmask >>= 1;
	if (!outmask)
	{
		PUTWORD (fields [0] - 1,opcode);
		fields [0]++;
	}
	else if (outmask == 0x80) fields [0]++;
}

void fbitout (LONG var, LONG mask, int count)
{
	for (;count;count--,mask >>= 1)
		bitout ((var & mask) ? 1 : 0);
}

int sbitout (char s)
{
	if (s == '0' || s == '1')
	{
		if (outfield)
		{
			fbitout (outstuff,mask,count);
			outfield = '\0';
		}
		bitout (s - '0');
		return MATCH;
	}
	if (! (s == '.' || islower (s))) return NOMATCH;
	if (outfield)
	{
		if (s == outfield)
		{
			mask <<= 1;
			count++;
			return MATCH;
		}
		fbitout (outstuff,mask,count);
	}
	count = 1;
	mask = fieldmasks [(short) CTOI (s)];
	outstuff = fields [(short) CTOI (s)];
	outfield = s;
	return MATCH;
}
int asmoutput (char *s)
{
	int status,i;

	for (outmask = i = 0; i < PC_FIELD; i++)
	{
		fieldmasks [i] = 1;
		fielddir [i] = 0;
	}
	for (outputting = 1, outfield = '\0', asmout = s; *asmout;)
	{
		status = asm_block ();
		if (status & QUIT || !(status & MATCH)) break;
	}
	if (outfield)
	{
		fbitout (fields [(short) CTOI(outfield)],mask,count);
		outfield = '\0';
	}
	return MATCH;
}

struct match *asm_index [] =
{/* A */ asmtable,       /* B */ &asmtable [18], /* C */ &asmtable [34],
 /* D */ &asmtable [43], /* E */ &asmtable [49], /* F */ &asmtable [143],
 /* G */ &asmtable [143], /* H */ &asmtable [143], /* I */ &asmtable [59],
 /* J */ &asmtable [60], /* K */ &asmtable [143], /* L */ &asmtable [62],
 /* M */ &asmtable [69], /* N */ &asmtable [99], /* O */ &asmtable [104],
 /* P */ &asmtable [109], /* Q */ &asmtable [143], /* R */ &asmtable [110],
 /* S */ &asmtable [121], /* T */ &asmtable [134], /* U */ &asmtable [142],
 /* V */ &asmtable [143], /* W */ &asmtable [143], /* X */ &asmtable [143],
 /* Y */ &asmtable [143], /* Z */ &asmtable [143] };

int sasm (char *string)
{
	int status,i;
	struct match *srch;

	if (fields [0] & 1) fields [0]++;
	skipwhite (&string);
	if (isalpha (*string)) srch = asm_index [toupper (*string) - 'A'];
	else srch = asmtable;
	for (outputting = 0; *srch->in; srch++)
	{
		for (i = 2; i < PC_FIELD; i++)
			fields [i] = 0;
		currvar = 2;
		for (i = 0, asmin = string, asmout = srch->in; *asmout;)
			if (!((status = asm_block ()) & MATCH)) break;
		if ((status & ERROR) || (!*asmout && !*asmin)) break;
	}
	if (!(status & ERROR) && *srch->in && !*asmin) asmoutput (srch->out);
	return status;
}

int do_asm (int argc, char **argv)
{
	char *cmd_ptr, buf [128];
	LONG where,next;
	extern column;
	int status;
	extern char *getline (char *);

	strcpy (last_cmd, "asm");
	if (argc > 1)
	{
		if (gethex (argv [1],&where))
		{
			report_error (eval_error, argv [1]);
			return -1;
		}
	}
	else where = pcsave;
	for (;;)
	{
		next = disasm (where);
		pc ('\n');
		fields [PC_FIELD] = fields [0] = where;
		ltoh (fields [0]);
		while (column < DASM_DASM_COLUMN) pc (' ');
		cmd_ptr = getline (buf);
		if (!strlen (cmd_ptr))
		{
			where = next;
			continue;
		}
		if (cmd_ptr [0] == '.') break;
		status = sasm (cmd_ptr);
		if (!(status & ERROR))
			if (!(status & MATCH)) print ("Not Matched\n");
			else
			{
				where = fields [0];
				if (UseWindows) DISPLAYCODE;
			}
	}
	pcsave = where;
	return 0;
}
