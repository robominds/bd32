/* file newdasm.c - BD32new disassembler */

#include        <stdio.h>
#include        <ctype.h>
#include	<conio.h>
#include	<stdarg.h>
#include	<string.h>

#include        "didefs.h"
/* #include        "didecls.h" */
#include        "symbol.h"
#include        "dasmdefs.h"
#include	"color.h"
#include	"disp-mac.h"
#include	"target.h"
#include	"regnames.h"
#include	"prog-def.h"
#include	"scrninfo.h"

#include	"newdasm.p"
#include	"bd32new.p"
#include	"newsym.p"

static LONG dasm_start, dasm_pc;
static char Output [200];

char *sizes [] = { ".B",".W",".L" };
char bd_reserved,is_problem;

extern LONG fields [26];
extern int eval_error;
extern char last_cmd [];
extern int pauselinecount;

extern struct text_info *CurrentWindow, DisassemblyWindow;
extern unsigned ThrowAway;
extern LONG *DisassemblyAddress;
extern unsigned DisassemblyCount;

struct optable {
	char *template,*output;
	} ops0 [], ops1 [], ops2 [], ops3 [], ops4 [], ops5 [], ops6 [], ops7 [],
	  ops8 [], ops9 [], ops10 [], ops11 [], ops12 [], ops13 [], ops14 [], ops15 [];

static char *OutputBuffer;

void ResetBufferPointer (void)
{
	OutputBuffer = Output;
	*Output = '\0';
}

void pcb (char c)
{
	*OutputBuffer++ = c;
	*OutputBuffer = '\0';
}

void printb (char *String)
{
	while (*String) *OutputBuffer++ = *String++;
	*OutputBuffer = '\0';
}

int pfb (char *Format, ...)
{
	va_list marker;
	int ret;

	va_start( marker, Format);
	ret = vsprintf(OutputBuffer, Format, marker );
	va_end( marker );
	OutputBuffer += ret;
	*OutputBuffer = '\0';
	return ret;
}


/* sign extend stuff to the required size
 * only word and long are legal sizes here
 */

LONG sign_extend (LONG stuff, int size)
{
	switch (size)
	{
	case    BYTESIZE:
		return stuff & 0xff;
	case    WORDSIZE:
		return (stuff & 0x80) ? stuff | 0xffffff00L : stuff & 0x000000FFL;
	case    LONGSIZE:
		return (stuff & 0x8000) ? stuff | 0xffff0000L : stuff & 0x0000FFFFL;
	default:
		return stuff;
	}
}

void sltoh (char *buff, LONG stuff, int size)
{
	int ctr;

	buff += size;
	*buff-- = '\0';
	for (ctr = 8; ctr; ctr--,stuff >>= 4)
		*buff-- = ntoh (stuff & 0xf);
}

char *showmin_ (LONG stuff)
{
	static char buff [20];
	char *ptr;

	sltoh (buff,stuff,8);
	for (ptr = buff + 1; *ptr == '0'; ptr++) ;
	if (!*ptr) ptr--;
	return ptr;
}

char *showmin_1 (LONG stuff)
{
	char *ptr;
	static char buff [20];

	sltoh (buff,stuff,9);
	for (ptr = buff + 1; *ptr == '0'; ptr++) ;
	if (!*ptr) ptr--;
	*--ptr = '$';
	return ptr;
}

char *showmin (LONG stuff)
{
	struct symtab *symbol;

	if ((symbol = valfind (stuff)) != (struct symtab *) 0)
		return symbol->name;
	return showmin_1 (stuff);
}

LONG getstuff (int size)
{
	LONG stuff;

	switch (size)
	{
		case BYTESIZE:
			stuff = GETBYTE (dasm_pc + 1);
			dasm_pc += 2;
			break;

		case WORDSIZE:
			stuff = GETWORD (dasm_pc);
			dasm_pc += 2;
			break;

		case LONGSIZE:
			stuff = GETLONG (dasm_pc);
			dasm_pc += 4;
			break;
	}
	return stuff;
}

void immdata (int size)
{
	LONG stuff;

	stuff = getstuff (WORDSIZE);
	if (size == BYTESIZE) stuff &= 0xff;
	else if (size == LONGSIZE)
	{
		stuff <<= 16;
		stuff += getstuff (WORDSIZE);
	}
	pcb ('#');
	printb (showmin (stuff));
}

static char scalesizes [] = {'1','2','4','8'};

void badop (void);

int sh_ea (int mode, int reg, int sizeset)
{
	char buff [10];
	LONG varsave [11],temp;
	int i;
	switch ((int) fields [mode])
	{
		case 0:
		strcpy (buff,"D0");
		goto case1;

		case 1:
		strcpy (buff,"A0");
case1:          buff [1] += fields [reg];
		printb (buff);
		break;

		case 2:
case2:          strcpy (buff,"(A0)");
		goto case3;

		case 3:
		strcpy (buff,"(A0)+");
case3:          buff [2] += fields [reg];
		printb (buff);
		break;

		case 4:
		strcpy (buff,"-(A0)");
		buff [3] += fields [reg];
		printb (buff);
		break;

		case 5:
		printb (showmin (sign_extend (getstuff (WORDSIZE), LONGSIZE)));
		goto case2;

		case 6:
		for (i=0; i<11; i++) varsave [i] = fields [i];
		parsebit ("abbbcddefghhijjj");
		if (fields [5])         /* then full format */
		{
			bd_reserved |= !fields[8];
			if (fields [8] > 1)
			{
				if (fields [8] > 2)
					printb (showmin (getstuff (LONGSIZE)));
				else printb (showmin (sign_extend(getstuff (WORDSIZE),LONGSIZE)));
			}
			else printb ("0");
			pcb ('(');
			if (fields [6]) pcb ('Z');
			pfb ("A%c,",'0'+varsave[reg]);
			if (fields [7]) pcb ('Z');
			is_problem |= fields[10];
			pcb (fields[1] ? 'A' : 'D');
			pcb ('0'+fields[2]);
			printb (fields[3] ? ".L" : ".W");
			if (fields[4]) pfb ("*%c",scalesizes [(short) fields[4]]);
			pcb (')');
		}
		else
		{
			printb (showmin (sign_extend((fields[6]<<7)+(fields[7]<<6)+(fields[8]<<4)
				+(fields[9]<<3)+fields[10],WORDSIZE)));
			strcpy (buff,"(A0,D0.W");
			buff [2] += varsave [reg];
			buff [5] += fields [2];
			if (fields [1]) buff [4] = 'A';
			if (fields [3]) buff [7] = 'L';
			printb (buff);
			if (fields[4]) pfb ("*%c",scalesizes [(short) fields[4]]);
			pcb (')');
		}
		for (i=0; i< 11; i++) fields [i] = varsave [i];
		break;

		case 7:
		if (fields [reg] > 4)
		{
			badop ();
			return 1;
		}
		switch ((int) fields [reg])
		{
			case 0:
			printb (showmin (sign_extend (getstuff (WORDSIZE),LONGSIZE)));
			printb (".W");
			break;

			case 1:
			printb (showmin (getstuff (LONGSIZE)));
			printb (".L");
			break;

			case 2:
			temp = sign_extend (getstuff (WORDSIZE),LONGSIZE);
			temp = temp+2+dasm_start;
			printb (showmin (temp));
			printb ("(PC)");
			break;

			case 3:
			for (i=0; i<11; i++) varsave [i] = fields [i];
			parsebit ("abbbcddefghhijjj");
			if (fields [5])         /* then full format */
			{
				bd_reserved |= !fields[8];
				if (fields [8] > 1)
				{
					if (fields [8] > 2)
						printb (showmin (dasm_start+2+getstuff (LONGSIZE)));
					else printb (showmin (dasm_start+2+sign_extend(getstuff (WORDSIZE),LONGSIZE)));
				}
				else printb ("0");
				printb ("(");
				if (fields [6]) printb ("Z");
				printb ("PC,");
				if (fields [7]) pcb ('Z');
				is_problem |= fields[10];
				pcb (fields[1] ? 'A' : 'D');
				pcb ('0'+fields[2]);
				printb (fields[3] ? ".L" : ".W");
				if (fields[4])
				{
					pcb ('*');
					pcb (scalesizes [(short) fields[4]]);
				}
				pcb (')');
			}
			else
			{
				printb (showmin (dasm_start+2+sign_extend((fields[6]<<7)+(fields[7]<<6)+(fields[8]<<4)
					+(fields[9]<<3)+fields[10],WORDSIZE)));
				strcpy (buff,"(PC,D0.W");
				buff [5] += fields [2];
				if (fields [1]) buff [4] = 'A';
				if (fields [3]) buff [7] = 'L';
				printb (buff);
				if (fields[4])
				{
					pcb ('*');
					pcb (scalesizes [(short) fields[4]]);
				}
				pcb (')');
			}
			for (i=0; i<11; i++) fields [i] = varsave [i];
			break;

			case 4:
			immdata ((short) fields [sizeset]);
		}
	}
	return 0;
}

BYTE param1, param2;
char *cc [] = { "RA","F ","HI","LS","CC","CS","NE","EQ",
		"VC","VS","PL","MI","GE","LT","GT","LE" };

void ofmt (char *control)
{
	char gotone,paramcount,sizeset,c,*ptr;
	int reg,quit;
	WORD mask,reglist;
	static char *regnames [] =
{"A7","A6","A5","A4","A3","A2","A1","A0",
 "D7","D6","D5","D4","D3","D2","D1","D0",
 "D0","D1","D2","D3","D4","D5","D6","D7",
 "A0","A1","A2","A3","A4","A5","A6","A7" };

	bd_reserved = is_problem = sizeset = 0;
	for (quit = 0, ptr = control; *ptr && !quit; ptr++)
	{
		if ((c = *ptr) != '%')
		{
			pcb (c);
			continue;
		}
		param1 = param2 = paramcount = 0;
		if (isdigit (*++ptr))
		{
			param1 = *ptr++ - '0';
			paramcount++;
			if (isdigit (*ptr))
			{
				param2 = *ptr++ - '0';
				paramcount++;
			}
		}
		switch (*ptr)
		{
			case    'a':
			quit = sh_ea (param1, param2, sizeset);
			break;

			case    'c':
			printb (cc [(short) fields [(short) param1]]);
			break;

			case    'd':
			if (paramcount) printb (showmin (getstuff (param1)));
			else printb (showmin (getstuff ((short) fields [sizeset])));
			break;

			case    'g':
			switch (paramcount)
			{
				case    0:
				getstuff ((short) fields [sizeset]);
				break;

				case    1:
				fields [param1] = getstuff ((short) fields [sizeset]);
				break;

				case    2:
				fields [param2] = getstuff (param1);
				break;
			}
			break;

			case    'l':            /* make reglist for MOVEM */
			reglist = (short) getstuff (WORDSIZE);
			for (mask = 1 << 15,gotone = 0,
				reg = (param1) ? 16 : 0;
				mask; mask >>= 1,reg++)
				if (mask & reglist)
				{
					if (gotone++) pcb ('/');
					printb (regnames [reg]);
				}
			break;

			case    'p':
			printb (showmin (fields [param1]));
			break;

			case    'r':
			if (paramcount)
			{
				if (fields [param1] == 0xff)
				{
					printb (showmin (dasm_start + 2 + getstuff (LONGSIZE)));
					printb (".L");
				}
				else if (!fields [param1])
				{
					printb (showmin (dasm_start + 2 + sign_extend (getstuff (WORDSIZE),LONGSIZE)));
					printb (".W");
				}
				else
				{
					printb (showmin (dasm_start + 2 + sign_extend (fields [param1],WORDSIZE)));
					printb (".B");
				}
			}
			else
			{
				printb (showmin (dasm_start + 2 + sign_extend (getstuff (WORDSIZE),LONGSIZE)));
				printb (".W");
			}
			break;

			case    's':
			if (param1) sizeset = param1;
			else printb (sizes [(short) fields [sizeset]]);
			break;

			case    'u':
			printb (showmin_ (fields [param1]));
			break;

			case    'v':
			fields [param2] = param1;
			break;

			default:
			pcb (*ptr);
		}
	}
}

void badop (void)
{
	ResetBufferPointer ();
	pfb ("DC.W\t%s\tUndefined Opcode",showmin_1 (GETWORD (dasm_start)));
}
#define NF      void (*NULL) ()

struct optable *srch, *op_index [] =
{ops0, ops1, ops2, ops3, ops4, ops5, ops6, ops7,
ops8, ops9, ops10, ops11, ops12, ops13, ops14, ops15};

extern int column;

LONG disasm (LONG where)
{
	struct symtab *label;
	int ctr, labelsize;
	LONG address;

	enable_cache ();
	ResetBufferPointer ();
	dasm_start = where;
	if (dasm_start & 1)
	{
		ltoh (dasm_start);
		pf ("\tDC.B\t%s\tCorrect Word Alignment",showmin ((LONG) GETBYTE (dasm_start++)));
		clreol ();
		pc ('\n');
	}
	ltoh (dasm_start);
	pc (' ');
	for (srch = op_index [(short) (GETBYTE (where) >> 4)]; srch->template; srch++)
	{
		dasm_pc = dasm_start;

		if (parsebit (srch->template)) break;
	}
	ofmt (srch->output);
	label = valfind (dasm_start);
	labelsize = 0;
	if (label)
	{
		if ((labelsize = strlen (label->name)) > DASM_SYMBOLSIZE)
		{
			while (column < DASM_SYM_COLUMN) pc (' ');
			pf ("%s:",label->name);
			clreol ();
			print ("\n  ");
			ltoh (dasm_start);
			pc (' ');
		}
	}
	for (address = dasm_start,ctr = 0; address < dasm_pc && ctr < DASM_WORDS; address += 2,ctr++)
	{
		wtoh (GETWORD (address));
		pc (' ');
	}
	if (labelsize && labelsize <= DASM_SYMBOLSIZE)
	{
		while (column < DASM_SYM_COLUMN) pc (' ');
		pf ("%s:",label->name);
	}
	while (column < DASM_DASM_COLUMN) pc (' ');
	Output [GetWindowWidth (CurrentWindow) - column - 5] = '\0';
	print (Output);
	clreol ();
/*	if (bd_reserved)
	{
		clreol ();
		pc ('\n');
		print ("Warning: base displacement bits set to (0:0)");
	}
	if (is_problem)
	{
		clreol ();
		pc ('\n');
		print ("Error: Memory Indirect addressing mode selected");
	}
*/
	return (dasm_pc);
}

int parsebit (char *template)
{
	BYTE opcode,mask;
	LONG *currfield;

	for (mask = 0; mask < 11; mask++)
		fields [mask] = 0;

	for (mask = 0; *template; mask >>= 1, template++)
	{
		if (!mask)
		{
			mask = 1 << 7;
			opcode = GETBYTE (dasm_pc++);
			fields [0] <<= 8;
			fields [0] += opcode;
		}
		if (*template == '0')
		{
			if (! (mask & opcode)) continue;
			else return 0;
		}
		if (*template == '1')
		{
			if (mask & opcode) continue;
			else return 0;
		}
		currfield = &fields [tolower (*template) - 'a' + 1];
		*currfield = (*currfield << 1) + ((mask & opcode) ? 1 : 0);
	}
	return 1;
}

LONG Disassemble (LONG where, unsigned TheLine,
	unsigned Count, unsigned Blank)
{
	int LineCounter, BreakpointAddress, ScrollSave, frozen, save;
	LONG CurrentPC;
	struct text_info *OldWindow = CurrentWindow;

	frozen = stop_chip ();
	CurrentPC = GETREG (REG_PC);
	save = set_fc ();
	if (UseWindows)
	{
		SetWindow (&DisassemblyWindow);
		ScrollSave = SetScroll (0);
		gotoxy (1, 1 + TheLine);
	}
	for (LineCounter = 0;
		LineCounter < Count;
		LineCounter++)
	{
		DisassemblyAddress [LineCounter + TheLine] = where;
		BreakpointAddress = match_bp (where);
		if (
			(!UseWindows || !Blank) &&
			where == CurrentPC) textattr (PCCOLOR);
		else if (BreakpointAddress) textattr (BREAKPOINTCOLOR);
		else textattr (COMMANDOUTPUTCOLOR);
		print (BreakpointAddress ? "B*" : "  ");
		where = disasm (where);
		if (LineCounter != Count - 1) pc ('\n');
	}
	if (UseWindows) ThrowAway = 1;
	else textattr (COMMANDOUTPUTCOLOR);
	pc ('\n');
	ThrowAway = 0;
	if (save) restore_fc ();
	if (frozen) run_chip (0L);
	if (UseWindows)
	{
		SetWindow (OldWindow);
		SetScroll (ScrollSave);
	}
	return where;
}

extern LONG DisassemblyEndAddress;

int do_disasm (int argc, char **argv)
{
	static LONG where = 0;

	if (!argc) where = *DisassemblyAddress;

	if (argc > 1) if (gethex (argv [1],&where))
	{
		report_error (eval_error, argv [1]);
		return -1;
	}
	DisassemblyEndAddress = Disassemble (where, 0, DisassemblyCount, 0);
	if (argc)
	{
		where = DisassemblyEndAddress;
		strcpy (last_cmd, "dasm");
	}
	return 0;
}

struct optable ops0 [] =
{
	{ "0000000000111100","ORI \t#%0d,CCR" },
	{ "0000000001111100","ORI \t#%1d,SR" },
	{ "00000aa011bbbccc0ddd000000000000","%1sCMP2%s\t%23a,D%4u" },
	{ "00000aa011bbbccc1ddd000000000000","%1sCMP2%s\t%23a,A%4u" },
	{ "00000aa011bbbccc0ddd100000000000","%1sCHK2%s\t%23a,D%4u" },
	{ "00000aa011bbbccc1ddd100000000000","%1sCHK2%s\t%23a,A%4u" },
	{ "00000aa011bbbccc","DC.W\t%0p\tUndefined Opcode" },
	{ "00000000aabbbccc","%1sORI%s\t#%d,%23a" },
	{ "0000aaa100001bbb","%59vMOVEP.W\t%92a,D%1u" },
	{ "0000aaa101001bbb","%59vMOVEP.L\t%92a,D%1u" },
	{ "0000aaa110001bbb","%59vMOVEP.W\tD%1u,%92a" },
	{ "0000aaa111001bbb","%59vMOVEP.L\tD%1u,%92a" },
	{ "0000aaa100cccddd","BTST\tD%1u,%34a" },
	{ "0000aaa101cccddd","BCHG\tD%1u,%34a" },
	{ "0000aaa110cccddd","BCLR\tD%1u,%34a" },
	{ "0000aaa111cccddd","BSET\tD%1u,%34a" },
	{ "0000001000111100","ANDI\t#%0d,CCR" },
	{ "0000001001111100","ANDI\t#%1d,SR" },
	{ "00000010aabbbccc","%1sANDI%s\t#%d,%23a" },
	{ "00000100aabbbccc","%1sSUBI%s\t#%d,%23a" },
	{ "00000110aabbbccc","%1sADDI%s\t#%d,%23a" },
	{ "0000100000aaabbb","BTST\t#%0d,%12a" },
	{ "0000100001aaabbb","BCHG\t#%0d,%12a" },
	{ "0000100010aaabbb","BCLR\t#%0d,%12a" },
	{ "0000100011aaabbb","BSET\t#%0d,%12a" },
	{ "0000101000111100","EORI\t#%1d,CCR" },
	{ "0000101001111100","EORI\t#%1d,SR" },
	{ "0000101011aaaaaa","DC.W\t%0p\tUndefined Opcode" },
	{ "00001010aabbbccc","%1sEORI%s\t#%d,%23a" },
	{ "0000110011aaaaaa","DC.W\t%0p\tUndefined Opcode" },
	{ "00001100aabbbccc","%1sCMPI%s\t#%d,%23a" },
	{ "00001110aabbbccc0ddd000000000000","%1sMOVES%s\t%23a,D%4u" },
	{ "00001110aabbbccc1ddd000000000000","%1sMOVES%s\t%23a,A%4u" },
	{ "00001110aabbbccc0ddd100000000000","%1sMOVES%s\tD%4u,%23a" },
	{ "00001110aabbbccc1ddd100000000000","%1sMOVES%s\tA%4u,%23a" }
},
ops1 [] =
{
	{ "0001bbbcccdddeee","%1s%01vMOVE.B\t%45a,%32a" }
},

ops2 [] =
{
	{ "0010bbb001dddeee","%1s%21vMOVEA.L\t%45a,A%2u" },
	{ "0010bbbcccdddeee","%1s%21vMOVE.L\t%45a,%32a" }
},

ops3 [] =
{
	{ "0011bbb001dddeee","%1s%11vMOVEA.W\t%45a,A%2u" },
	{ "0011bbbcccdddeee","%1s%11vMOVE.W\t%45a,%32a" }
},

ops4 [] =
{
	{ "0100000011aaabbb","MOVE\tSR,%12a" },
	{ "01000000aabbbccc","%1sNEGX%s\t%23a" },
	{ "0100aaa100bbbccc","%5s%25vCHK.L\t%23a,D%1u" },
	{ "0100aaa110bbbccc","%5s%15vCHK.W\t%23a,D%1u" },
	{ "0100100111000aaa","EXTB.L\tD%1u" },
	{ "0100aaa111bbbccc","LEA \t%23a,A%1u" },
	{ "0100001011aaabbb","MOVE\tCCR,%12a" },
	{ "01000010aabbbccc","%1sCLR%s\t%23a" },
	{ "0100010011aaabbb","MOVE\t%12a,CCR" },
	{ "01000100aabbbccc","%1sNEG%s\t%23a" },
	{ "0100011011aaabbb","MOVE\t%12a,SR" },
	{ "01000110aabbbccc","%1sNOT%s\t%23a" },
	{ "0100100000001aaa","LINK\tA%1u,#%22g%2p" },
	{ "0100100000aaabbb","NBCD\t%12a" },
	{ "0100100001000aaa","SWAP\tD%1u" },
	{ "0100100001001aaa","BKPT\t#%1p" },
	{ "0100100001aaabbb","PEA \t%12a" },
	{ "0100100010000aaa","EXT.W\tD%1u" },
	{ "0100100011000aaa","EXT.L\tD%1u" },
	{ "0100100010aaabbb","%5s%15vMOVEM.W\t%1l,%12a" },
	{ "0100100011aaabbb","%5s%25vMOVEM.L\t%1l,%12a" },
	{ "0100101011111010","BGND" },
	{ "0100101011111100","ILLEGAL" },
	{ "0100101011aaabbb","TAS \t%12a" },
	{ "01001010aabbbccc","%1sTST%s\t%23a" },
	{ "0100110000aaabbb0ccc000000000ddd","%5s%25vMULU.L\t%12a,D%3u" },
	{ "0100110000aaabbb0ccc010000000ddd","%5s%25vMULU.L\t%12a,D%4u:D%3u" },
	{ "0100110000aaabbb0ccc100000000ddd","%5s%25vMULS.L\t%12a,D%3u" },
	{ "0100110000aaabbb0ccc110000000ddd","%5s%25vMULS.L\t%12a,D%4u:D%3u" },
	{ "0100110001aaabbb0000000000000000","%5s%25vDIVU.L\t%12a,D0" },
	{ "0100110001aaabbb0000100000000000","%5s%25vDIVS.L\t%12a,D0" },
	{ "0100110001aaabbb0001000000000001","%5s%25vDIVU.L\t%12a,D1" },
	{ "0100110001aaabbb0001100000000001","%5s%25vDIVS.L\t%12a,D1" },
	{ "0100110001aaabbb0010000000000010","%5s%25vDIVU.L\t%12a,D2" },
	{ "0100110001aaabbb0010100000000010","%5s%25vDIVS.L\t%12a,D2" },
	{ "0100110001aaabbb0011000000000011","%5s%25vDIVU.L\t%12a,D3" },
	{ "0100110001aaabbb0011100000000011","%5s%25vDIVS.L\t%12a,D3" },
	{ "0100110001aaabbb0100000000000100","%5s%25vDIVU.L\t%12a,D4" },
	{ "0100110001aaabbb0100100000000100","%5s%25vDIVS.L\t%12a,D4" },
	{ "0100110001aaabbb0101000000000101","%5s%25vDIVU.L\t%12a,D5" },
	{ "0100110001aaabbb0101100000000101","%5s%25vDIVS.L\t%12a,D5" },
	{ "0100110001aaabbb0110000000000110","%5s%25vDIVU.L\t%12a,D6" },
	{ "0100110001aaabbb0110100000000110","%5s%25vDIVS.L\t%12a,D6" },
	{ "0100110001aaabbb0111000000000111","%5s%25vDIVU.L\t%12a,D7" },
	{ "0100110001aaabbb0111100000000111","%5s%25vDIVS.L\t%12a,D7" },
	{ "0100110001aaabbb0ccc000000000ddd","%5s%25vDIVUL.L\t%12a,D%4u:D%3u" },
	{ "0100110001aaabbb0ccc010000000ddd","%5s%25vDIVU.L\t%12a,D%4u:D%3u" },
	{ "0100110001aaabbb0ccc100000000ddd","%5s%25vDIVSL.L\t%12a,D%4u:D%3u" },
	{ "0100110001aaabbb0ccc110000000ddd","%5s%25vDIVS.L\t%12a,D%4u:D%3u" },
	{ "0100110010aaabbb","%5s%15vMOVEM.W\t%12a,%0l" },
	{ "0100110011aaabbb","%5s%25vMOVEM.L\t%12a,%0l" },
	{ "0100111001001111aaaaaaaaaaaaaaaa","SYSCALL\t%1p" },
	{ "010011100100aaaa","TRAP\t#%1p" },
	{ "0100111001010aaa","LINK\tA%1u,#%1d" },
	{ "0100111001011aaa","UNLK\tA%1u" },
	{ "0100111001100aaa","MOVE\tA%1u,USP" },
	{ "0100111001101aaa","MOVE\tUSP,A%1u" },
	{ "0100111001110000","RESET" },
	{ "0100111001110001","NOP" },
	{ "0100111001110010","STOP\t#%1d" },
	{ "0100111001110011","RTE" },
	{ "0100111001110100","RTD \t#%1d" },
	{ "0100111001110101","RTS" },
	{ "0100111001110110","TRAPV" },
	{ "0100111001110111","RTR" },
	{ "01001110011110100aaa000000000000","MOVEC\tSFC,D%1u" },
	{ "01001110011110100aaa000000000001","MOVEC\tDFC,D%1u" },
	{ "01001110011110100aaa100000000000","MOVEC\tUSP,D%1u" },
	{ "01001110011110100aaa100000000001","MOVEC\tVBR,D%1u" },
	{ "01001110011110101aaa000000000000","MOVEC\tSFC,A%1u" },
	{ "01001110011110101aaa000000000001","MOVEC\tDFC,A%1u" },
	{ "01001110011110101aaa100000000000","MOVEC\tUSP,A%1u" },
	{ "01001110011110101aaa100000000001","MOVEC\tVBR,A%1u" },
	{ "01001110011110110aaa000000000000","MOVEC\tD%1u,SFC" },
	{ "01001110011110110aaa000000000001","MOVEC\tD%1u,DFC" },
	{ "01001110011110110aaa100000000000","MOVEC\tD%1u,USP" },
	{ "01001110011110110aaa100000000001","MOVEC\tD%1u,VBR" },
	{ "01001110011110111aaa000000000000","MOVEC\tA%1u,SFC" },
	{ "01001110011110111aaa000000000001","MOVEC\tA%1u,DFC" },
	{ "01001110011110111aaa100000000000","MOVEC\tA%1u,USP" },
	{ "01001110011110111aaa100000000001","MOVEC\tA%1u,VBR" },
	{ "0100111010aaabbb","JSR \t%12a" },
	{ "0100111011aaabbb","JMP \t%12a" }
},

ops5 [] =
{
	{ "0101aaaa11001bbb","DB%1c\tD%2u,%r" },      /* 122 */
	{ "0101aaaa11111100","TRAP%1c" },
	{ "0101aaaa11111010","TRAP%1c.W\t#%12g%2p" },
	{ "0101aaaa11111011","TRAP%1c.L\t#%22g%2p" },
	{ "0101aaaa11bbbccc","S%1c\t%23a" },
	{ "01010000bbcccddd","%2sADDQ%s\t#8,%34a" },
	{ "0101aaa0bbcccddd","%2sADDQ%s\t#%1p,%34a" },
	{ "01010001bbcccddd","%2sSUBQ%s\t#8,%34a" },
	{ "0101aaa1bbcccddd","%2sSUBQ%s\t#%1p,%34a" }
},

ops6 [] =
{
	{ "01100000aaaaaaaa","BRA \t%1r" },          /* 131 */
	{ "01100001aaaaaaaa","BSR \t%1r" },
	{ "0110aaaabbbbbbbb","B%1c\t%2r" }
},

ops7 [] =
{
	{ "0111aaa0bbbbbbbb","MOVEQ\t#%2p,D%1u" }
},

ops8 [] =
{
	{ "1000aaa100000bbb","SBCD\tD%2u,D%1u" },     /* 135 */
	{ "1000aaa100001bbb","SBCD\t-(A%2u),-(A%1u)" },
	{ "1000aaa011bbbccc","%5s%15vDIVU.W\t%23a,D%1u" },
	{ "1000aaa111bbbccc","%5s%15vDIVS.W\t%23a,D%1u" },
	{ "1000aaa0bbcccddd","%2sOR%s\t%34a,D%1u" },
	{ "1000aaa1bbcccddd","%2sOR%s\tD%1u,%34a" }
},

ops9 [] =
{
	{ "1001aaa011bbbccc","%4s%14vSUBA.W\t%23a,A%1u" },   /* 141 */
	{ "1001aaa111bbbccc","%4s%24vSUBA.L\t%23a,A%1u" },
	{ "1001aaa1bb000ccc","%2sSUBX%s\tD%3u,D%1u" },
	{ "1001aaa1bb001ccc","%2sSUBX%s\t-(A%3u),-(A%1u)" },
	{ "1001aaa0bbcccddd","%2sSUB%s\t%34a,D%1u" },
	{ "1001aaa1bbcccddd","%2sSUB%s\tD%1u,%34a" }
},

ops10 [] =
{
	{ "1010aaaaaaaaaaaa","* Line 1010 Trap (%1p)" }
},

ops11 [] =
{

	{ "1011aaa011bbbccc","%4s%14vCMPA.W\t%23a,A%1u" },	/* 148 */
	{ "1011aaa111bbbccc","%4s%24vCMPA.L\t%23a,A%1u" },
	{ "1011aaa1bb001ccc","%2sCMPM%s\t(A%3u)+,(A%1u)+" },
	{ "1011aaa0bbcccddd","%2sCMP%s\t%34a,D%1u" },
	{ "1011aaa1bbcccddd","%2sEOR%s\tD%1u,%34a" },
},

ops12 [] =
{

	{ "1100aaa110001bbb","EXG \tD%1u,A%2u" },              /* 153 */
	{ "1100aaa101001bbb","EXG \tA%1u,A%2u" },
	{ "1100aaa101000bbb","EXG \tD%1u,D%2u" },
	{ "1100aaa100000bbb","ABCD\tD%2u,D%1u" },
	{ "1100aaa100001bbb","ABCD\t-(A%2u),-(A%1u)" },
	{ "1100aaa011bbbccc","%5s%15vMULU.W\t%23a,D%1u" },
	{ "1100aaa111bbbccc","%5s%15vMULS.W\t%23a,D%1u" },
	{ "1100aaa0bbcccddd","%2sAND%s\t%34a,D%1u" },
	{ "1100aaa1bbcccddd","%2sAND%s\tD%1u,%34a" }
},

ops13 [] =
{

	{ "1101aaa1bb000ccc","%2sADDX%s\tD%3u,D%1u" },        /* 162 */
	{ "1101aaa1bb001ccc","%2sADDX%s\t-(A%3u),-(A%1u)" },
	{ "1101aaa011bbbccc","%4s%14vADDA.W\t%23a,A%1u" },
	{ "1101aaa111bbbccc","%4s%24vADDA.L\t%23a,A%1u" },
	{ "1101aaa0bbcccddd","%2sADD%s\t%34a,D%1u" },
	{ "1101aaa1bbcccddd","%2sADD%s\tD%1u,%34a" }
},

ops14 [] =
{
	{ "1110000011aaabbb","%5s%15vASR.W\t%12a" },                 /* 168 */
	{ "1110000111aaabbb","%5s%15vASL.W\t%12a" },
	{ "1110001011aaabbb","%5s%15vLSR.W\t%12a" },
	{ "1110001111aaabbb","%5s%15vLSL.W\t%12a" },
	{ "1110010011aaabbb","%5s%15vROXR.W\t%12a" },
	{ "1110010111aaabbb","%5s%15vROXL.W\t%12a" },
	{ "1110011011aaabbb","%5s%15vROR.W\t%12a" },
	{ "1110011111aaabbb","%5s%15vROL.W\t%12a" },
	{ "11100000bb000ccc","%2sASR%s\t#8,D%3u" },
	{ "11100000bb001ccc","%2sLSR%s\t#8,D%3u" },
	{ "11100000bb010ccc","%2sROXR%s\t#8,D%3u" },
	{ "11100000bb011ccc","%2sROR%s\t#8,D%3u" },
	{ "11100001bb000ccc","%2sASL%s\t#8,D%3u" },
	{ "11100001bb001ccc","%2sLSL%s\t#8,D%3u" },
	{ "11100001bb010ccc","%2sROXL%s\t#8,D%3u" },
	{ "11100001bb011ccc","%2sROL%s\t#8,D%3u" },
	{ "11101aaa11bbbbbb","DC.W\t%0p\tUndefined Opcode" },
	{ "1110aaa0bb000ccc","%2sASR%s\t#%1p,D%3u" },
	{ "1110aaa0bb001ccc","%2sLSR%s\t#%1p,D%3u" },
	{ "1110aaa0bb010ccc","%2sROXR%s\t#%1p,D%3u" },
	{ "1110aaa0bb011ccc","%2sROR%s\t#%1p,D%3u" },
	{ "1110aaa1bb000ccc","%2sASL%s\t#%1p,D%3u" },
	{ "1110aaa1bb001ccc","%2sLSL%s\t#%1p,D%3u" },
	{ "1110aaa1bb010ccc","%2sROXL%s\t#%1p,D%3u" },
	{ "1110aaa1bb011ccc","%2sROL%s\t#%1p,D%3u" },
	{ "1110aaa0bb100ccc","%2sASR%s\tD%1u,D%3u" },
	{ "1110aaa0bb101ccc","%2sLSR%s\tD%1u,D%3u" },
	{ "1110aaa0bb110ccc","%2sROXR%s\tD%1u,D%3u" },
	{ "1110aaa0bb111ccc","%2sROR%s\tD%1u,D%3u" },
	{ "1110aaa1bb100ccc","%2sASL%s\tD%1u,D%3u" },
	{ "1110aaa1bb101ccc","%2sLSL%s\tD%1u,D%3u" },
	{ "1110aaa1bb110ccc","%2sROXL%s\tD%1u,D%3u" },
	{ "1110aaa1bb111ccc","%2sROL%s\tD%1u,D%3u" },
	{ "1110aaaaaaaaaaaa","DC.W\t%0p\tUndefined Opcode" }
},

ops15 [] =
{

	{ "11110000000000000000000111000000","LPSTOP\t#%11g%1p" }, /* 202 */
	{ "1111101000aaabbb0ccc0001dd000000","%4sTBLU%s\t%12a,D%3u" },
	{ "1111101000aaabbb0ccc0101dd000000","%4sTBLUN%s\t%12a,D%3u" },
	{ "1111101000aaabbb0ccc1001dd000000","%4sTBLS%s\t%12a,D%3u" },
	{ "1111101000aaabbb0ccc1101dd000000","%4sTBLSN%s\t%12a,D%3u" },
	{ "1111aaaaaaaaaaaa","* Line 1111 Trap (%1p)" },
	{ "aaaaaaaaaaaaaaaa","DC.W\t%0p\tUndefined Opcode" }
};
