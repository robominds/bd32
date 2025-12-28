// bd32f2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
/* BD32new.c - assembly language debug monitor for CPU32 systems */
#include <wtypes.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <ctype.h>
#include        <string.h>
#include        <setjmp.h>
#include        <dos.h>
//#include        <bios.h>
#include        <stdarg.h>
#include	<conio.h>
//#include	<dir.h>
//#include        "didefs.h"
#include        "digp.h"
#include        "gpdefine.h"
#include        "symbol.h"
#include        "prog-def.h"
#include	"logfile.h"
#include	"color.h"
#include	"disp-mac.h"
#include	"trgtstat.h"
#include	"target.h"
#include	"regnames.h"
#include	"reglist.h"
#include	"scrninfo.h"

#include	"bd32new.p"
#include	"newasm.p"
#include	"newdasm.p"
#include	"newsym.p"
#include	"di_error.p"
#include	"target.p"

#define MAXARG  17              /* max no. of arguments */
#define MAXCMD  128             /* size of command line buffer */
#define SEP_CHAR        ','     /* command separator character */
#define OPTION_CHAR     ';'     /* option delimiter character */

#define VERSTRING       "1.22"
#define CFG_FILE        "bd32.cfg"      /* name of configuration file */
#define ESC_KEY 0x1b

#define	PROG_EXT	".d32"
#define	MSG_EXT		".msg"
#define	PROG_LOADERR	-999
#define	BGND		0x4afa          /* BGND opcode */

#define	REGISTERBOTTOM	5
#define	COMMANDTOP	REGISTERBOTTOM + 1
#define	CODETOP		REGISTERBOTTOM + 2

/* define register struct */

#define REGCOUNT        23      /* no. of registers in machine */

struct RegList regs [] = {
	{"D0 ", REG_D0},{"D1 ", REG_D1},{"D2 ", REG_D2},{"D3 ", REG_D3},
	{"D4 ", REG_D4},{"D5 ", REG_D5},{"D6 ", REG_D6},{"D7 ", REG_D7},
	{"A0 ", REG_A0},{"A1 ", REG_A1},{"A2 ", REG_A2},{"A3 ", REG_A3},
	{"A4 ", REG_A4},{"A5 ", REG_A5},{"A6 ", REG_A6},{"A7 ", REG_A7},
	{"USP", REG_USP},{"SSP", REG_SSP},{"SR ", REG_SR},{"PC ", REG_PC},
	{"VBR", REG_VBR},{"SFC", REG_SFC},{"DFC", REG_DFC},{0, 0}
};

char *cmd_ptr, commandline [MAXCMD],*cmd_option,
*signon = "BD32 Version " VERSTRING "\n",
*uprompt = "BD32->",
*breakmask = "$8007FFFF";
#define	DefaultBreakpointMask	0x8007ffffL

struct ScreenInfo *screensave;
unsigned char

Attributes []		= {	(BLUE << 4) | YELLOW,	/* Command Output */
				(BLUE << 4) | WHITE,	/* Command Input */
				(BLUE << 4) | LIGHTGRAY,/* Prompt */
				(BLUE << 4) | WHITE,	/* Register Names */
				(RED << 4) | WHITE,	/* Breakpoint */
				(CYAN << 4),		/* PC Highlight */
				LIGHTGRAY << 4,		/* Status Line */
				(LIGHTGRAY << 4) | RED,	/* Status Item */
				(BLUE << 4) | WHITE};	/* Window Border */
int UseWindows = 0;
static int REGISTERTOP;
static int ConfigPort, ConfigSpeed;
char ConfigPortName [40];
extern int eval_error, fc;
int cmd_error, opsize;
jmp_buf cmd_jmp;
extern char frozen, *eval_ptr, last_symbol [];

LONG *DisassemblyAddress, DisassemblyEndAddress;
unsigned DisassemblyCount;

FILE *macro;
FILE *infile = NULL;
unsigned ThrowAway = 0;
unsigned StepBreakpoint = 0;
unsigned TraceRedisplayNeeded = 0;

LONG execute_address, lowest;

struct cmd_table {
char *name,*description;
int (*function) (int, char **);
} commands [];

unsigned CtrlBreakCount = 0;
int ExtKey = 0;

int HandleCtrlBreak (void)
{
	CtrlBreakCount++;
	return 1;
}

struct text_info	RegisterWindow,
			DisassemblyWindow,
			CommandLineWindow,
			SmallCommandLineWindow,
			StatusLineWindow,
			*CurrentWindow;

void SetTextInfo (struct text_info *TheInfo)
{
	textmode (TheInfo->currmode);
	window (TheInfo->winleft, TheInfo->wintop,
		TheInfo->winright, TheInfo->winbottom);
	textattr (TheInfo->attribute);
	gotoxy (TheInfo->curx, TheInfo->cury);
}

void GetWindow (struct text_info *Window)
{
	gettextinfo (Window);
}

void SetWindow (struct text_info *Window)
{
	if (Window == CurrentWindow) return;
	GetWindow (CurrentWindow);
	SetTextInfo (Window);
	CurrentWindow = Window;
}

#define	TOPLEFTCORNER		0
#define	TOPRIGHTCORNER		1
#define	BOTTOMLEFTCORNER	2
#define	BOTTOMRIGHTCORNER	3
#define	VERTICAL		4
#define	HORIZONTAL		5
#define	LEFTJUNCTION		6
#define	RIGHTJUNCTION		7
#define	TOPJUNCTION		8
#define	BOTTOMJUNCTION		9
#define	CENTERJUNCTION		10

char	DoubleLineCharacters [] = "\xc9\xbb\xc8\xbc\xba\xcd\xcc\xb9\xcb\xca\xce",
	SingleLineCharacters [] = "\xda\xbf\xc0\xd9\xb3\xc4\xc3\xb4\xc2\xc1\xc5",
	BlankBorderCharacters [] = "           ",
	SolidBorderCharacters [] = "\xdb\xdb\xdb\xdb\xdb\xdb\xdb\xdb\xdb\xdb\xdb",
	FuzzyBorderCharacters [] = "\xb1\xb1\xb1\xb1\xb1\xb1\xb1\xb1\xb1\xb1\xb1";

char *BorderArray [] = {DoubleLineCharacters, SingleLineCharacters,
			BlankBorderCharacters, SolidBorderCharacters,
			FuzzyBorderCharacters };

#define	DOUBLELINEBORDER	0
#define	SINGLELINEBORDER	1
#define	BLANKBORDER		2
#define	SOLIDBORDER		3
#define	FUZZYBORDER		4
#define	MAXBORDER		4

void RepeatChar (char Char, unsigned Count, unsigned char Attribute)
{
	union REGS regs;

	regs.h.ah = 9;
	regs.h.al = Char;
	regs.h.bl = Attribute;
	regs.h.bh = 0;
	regs.x.cx = Count;
	int86 (0x10, &regs, &regs);
}

int SetScroll (int NewValue)
{
	int temp = _wscroll;

	_wscroll = NewValue;
	return temp;
}

void DrawBox (unsigned Left, unsigned Top, unsigned Right, unsigned Bottom,
	unsigned BorderType, unsigned char Attribute)
{
	char *Characters = BorderArray [min (MAXBORDER, BorderType)];
	int Line, ScrollSave = SetScroll (0);

	textattr (Attribute);
	gotoxy (Left, Top);
	putch (Characters [TOPLEFTCORNER]);
	RepeatChar (Characters [HORIZONTAL], Right - Left - 1, Attribute);
	gotoxy (Right, Top);
	putch (Characters [TOPRIGHTCORNER]);
	for (Line = Top + 1; Line <= Bottom - 1; Line++)
	{
		gotoxy (Left, Line);
		putch (Characters [VERTICAL]);
		gotoxy (Right, Line);
		putch (Characters [VERTICAL]);
	}
	gotoxy (Left, Bottom);
	putch (Characters [BOTTOMLEFTCORNER]);
	RepeatChar (Characters [HORIZONTAL], Right - Left - 1, Attribute);
	gotoxy (Right, Bottom);
	putch (Characters [BOTTOMRIGHTCORNER]);
	SetScroll (ScrollSave);
}

void BoxHorizontalBar (unsigned Left, unsigned Right, unsigned Y,
	unsigned BorderType, unsigned char Attribute)
{
	char *Characters = BorderArray [min (MAXBORDER, BorderType)];
	int ScrollSave = SetScroll (0);

	gotoxy (Left, Y);
	putch (Characters [LEFTJUNCTION]);
	RepeatChar (Characters [HORIZONTAL], Right - Left - 1, Attribute);
	gotoxy (Right, Y);
	putch (Characters [RIGHTJUNCTION]);
	SetScroll (ScrollSave);
}

unsigned GetWindowHeight (struct text_info *TheWindow)
{
	return TheWindow->winbottom - TheWindow->wintop + 1;
}

unsigned GetWindowWidth (struct text_info *TheWindow)
{
	return TheWindow->winright - TheWindow->winleft + 1;
}

void CenterString (char *String, unsigned Y)
{
	int	StringLength = strlen (String),
		WindowWidth = GetWindowWidth (CurrentWindow);

	gotoxy (max (1, (WindowWidth - StringLength) / 2), Y);
	cputs (String);
}

struct ScreenInfo *SaveScreen (void)
{
	struct ScreenInfo *TheInfo = malloc (sizeof (struct ScreenInfo));

	if (!TheInfo) return 0;
	gettextinfo (&TheInfo->TextInfo);
	if (!(TheInfo->ScreenMatrix = malloc (TheInfo->TextInfo.screenheight
					* TheInfo->TextInfo.screenwidth
					* 2)))
	{
		free (TheInfo);
		return 0;
	}
	gettext (1, 1,
		TheInfo->TextInfo.screenwidth,
		TheInfo->TextInfo.screenheight,
		TheInfo->ScreenMatrix);
#ifdef	DEBUG_SCREEN
	pf ("winleft=%d wintop=%d winright=%d winbottom=%d screenheight=%d screenwidth=%d mode=%d\n",
		TheInfo->TextInfo.winleft, TheInfo->TextInfo.wintop,
		TheInfo->TextInfo.winright, TheInfo->TextInfo.winbottom,
		TheInfo->TextInfo.screenheight, TheInfo->TextInfo.screenwidth,
		TheInfo->TextInfo.currmode);
#endif
	return TheInfo;
}

void RestoreScreen (struct ScreenInfo *TheInfo)
{
	SetTextInfo (&TheInfo->TextInfo);
	puttext (1, 1,
		TheInfo->TextInfo.screenwidth,
		TheInfo->TextInfo.screenheight,
		TheInfo->ScreenMatrix);
	free (TheInfo->ScreenMatrix);
	free (TheInfo);
}

void SetUpScreen (int FirstTimeThrough)
{
	unsigned CodeBorder;

	if (FirstTimeThrough)
	{
		screensave = SaveScreen ();
		CurrentWindow = &screensave->TextInfo;
	}
	memcpy (&StatusLineWindow, &screensave->TextInfo,
		sizeof (struct text_info));
	StatusLineWindow.wintop
	= StatusLineWindow.winbottom;
	StatusLineWindow.curx
	= StatusLineWindow.cury
	= 1;
	StatusLineWindow.attribute = STATUSLINECOLOR;
	SetWindow (&StatusLineWindow);
	memcpy (&CommandLineWindow, &screensave->TextInfo,
		sizeof (struct text_info));
	CommandLineWindow.winbottom--;
	CommandLineWindow.curx = CommandLineWindow.cury = 1;
	CommandLineWindow.attribute = COMMANDOUTPUTCOLOR;
	SetWindow (&CommandLineWindow);
	clrscr ();
	if (UseWindows)
	{
		REGISTERTOP = 2;
		DrawBox (CommandLineWindow.winleft,
			CommandLineWindow.wintop,
			CommandLineWindow.winright,
			CommandLineWindow.winbottom,
			DOUBLELINEBORDER, BORDERCOLOR);
		CenterString (" CPU Registers ", REGISTERTOP - 1);
		BoxHorizontalBar (CommandLineWindow.winleft,
			CommandLineWindow.winright,
			REGISTERBOTTOM + 1,
			DOUBLELINEBORDER, BORDERCOLOR);
		CenterString (" Memory Display ", CODETOP - 1);
		BoxHorizontalBar (CommandLineWindow.winleft,
			CommandLineWindow.winright,
			CodeBorder = (GetWindowHeight (&CommandLineWindow)
				- CODETOP) * 2 / 3 + CODETOP - 2,
			DOUBLELINEBORDER, BORDERCOLOR);
		CenterString (" Command Line ", CodeBorder);
	}
	else REGISTERTOP = 1;
	SetWindow (&StatusLineWindow);
	if (FirstTimeThrough) clrscr ();
	if (UseWindows)
	{
		CommandLineWindow.wintop++;
		CommandLineWindow.winleft++;
		CommandLineWindow.winright--;
		CommandLineWindow.winbottom--;
		memcpy (&RegisterWindow, &CommandLineWindow, sizeof (struct text_info));
		memcpy (&DisassemblyWindow, &CommandLineWindow, sizeof (struct text_info));
		CommandLineWindow.wintop = CodeBorder + 1;
		RegisterWindow.winbottom = REGISTERBOTTOM;
		DisassemblyWindow.wintop = CODETOP;
		DisassemblyWindow.winbottom = CodeBorder - 1;
		RegisterWindow.attribute
			= DisassemblyWindow.attribute
			= CommandLineWindow.attribute
			= COMMANDOUTPUTCOLOR;
	}
	else
	{
		memcpy (&RegisterWindow, &CommandLineWindow, sizeof (struct text_info));
		memcpy (&SmallCommandLineWindow, &CommandLineWindow, sizeof (struct text_info));
		RegisterWindow.winbottom = REGISTERBOTTOM;
		SmallCommandLineWindow.wintop = COMMANDTOP;
	}
	SetWindow (&CommandLineWindow);
	gotoxy (1,1);
	if (UseWindows && !FirstTimeThrough)
	{
		DISPLAYCODE;
		DISPLAYREGISTERS;
	}
}

void SelectLargeCommandLineWindow (void)
{
	int CurY = wherey ();

	if (UseWindows) return;
	if (CurrentWindow == &CommandLineWindow) return;
	CommandLineWindow.cury = CurY + REGISTERBOTTOM;
	SetWindow (&CommandLineWindow);
}

void SelectSmallCommandLineWindow (void)
{
	int CurY = wherey ();

	if (UseWindows) return;
	if (CurrentWindow == &SmallCommandLineWindow) return;
	SmallCommandLineWindow.cury = CurY < COMMANDTOP ? 1 : CurY - REGISTERBOTTOM;
	SetWindow (&SmallCommandLineWindow);
}

static int do_window (int argc, char **argv)
{
	if (argc > 2)
	{
		pf ("usage: window [ON | OFF]\n");
		return (-1);
	}
	if (argc == 2)
	{
		if (!stricmp ("ON", argv [1]))
		{
			UseWindows = 1;
			SetUpScreen (0);
		}
		else if (!stricmp ("OFF", argv [1]))
		{
			UseWindows = 0;
			SetUpScreen (0);
		}
		else
		{
			pf ("Illegal parameter: %s\nParameter must be 'on' or 'off'\n",
				argv [1]);
			return -1;
		}
	}
	pf ("Windowed user interface is %s\n",
		UseWindows ? "enabled" : "disabled");
	return 0;
}

static unsigned GetSigStatus (void)
{
	unsigned retval = get_sigstatus ();

	if (retval & TARGETBRKPT)
	{
		if (StepBreakpoint)
		{
			nobr (StepBreakpoint - 1);
			retval = StepBreakpoint = 0;
		}
		if (UseWindows)
		{
			DISPLAYREGISTERS;
			DisplayCode (GETREG (REG_PC), OPTIMALREDISPLAY);
		}
	}
	return retval;
}

int column = 0;
struct LogBuffer OutputBuffer = {0};

LONG ProgStart, DriverOffset = 0L;
int SysCallEnabled = 0;

char ExecPath [MAXPATH];
FILE *LogFile = 0;
char LogFileName [MAXCMD];

void LogPutch (int c)
{
	if (OutputBuffer.BuffUsed < LOGBUFFLEN - 2)
		OutputBuffer.stuff [OutputBuffer.BuffUsed++] = c;
}

void LogPuts (char *string)
{
	while (*string) LogPutch (*string++);
}

void OutputLogBuffer (void)
{
	OutputBuffer.stuff [OutputBuffer.BuffUsed] = '\0';
	if (LogFile) fputs (OutputBuffer.stuff, LogFile);
	OutputBuffer.BuffUsed = 0;
}

char last_cmd [MAXCMD];

void CloseLog (void)
{
	if (LogFile)
	{
		pf ("closing logfile %s\n", LogFileName);
		fclose (LogFile);
		LogFile = 0;
		show_stat (1);
	}
}

int do_log (int argc, char **argv)
{
	*last_cmd = '\0';
	if (argc == 1)
	{
		if (LogFile) pf ("Commands logged to file '%s'\n", LogFileName);
		else pf ("Command logging is OFF\n");
		return 0;
	}
	if (argc != 2)
	{
		pf ("usage: log [OFF | filename]\n");
		return -1;
	}
	CloseLog ();
	if (!stricmp (argv [1], "OFF")) return 0;
	if (!(LogFile = fopen (argv [1], "w")))
	{
		pf ("error opening logfile %s\n", argv [1]);
		return -1;
	}
	strcpy (LogFileName, argv [1]);
	pf ("Logging to file %s\n", LogFileName);
	show_stat (1);
	return 0;
}

int pauselinecount, pause_flag = 1;

int GetPauseKey (void)
{
	while (!CtrlBreakCount)
	{
		show_stat (0);
		if (key_ready ()) return gc (0);
	}
	if (key_ready ()) gc (0);
	return 0x1b;
}

int pause_check (void)
{
	int key;

	if (!CtrlBreakCount && !pause_flag) return 0;
	if (++pauselinecount == GetWindowHeight (CurrentWindow) - 1)
	{
		textattr (PROMPTCOLOR);
		pf ("--More--");
		textattr (COMMANDOUTPUTCOLOR);
		key = GetPauseKey ();
		pf ("\r        \r");
		pauselinecount = 0;
		if (key == 0x1b) return 1;
	}
	return 0;
}

void pc (char);

int pf (char *fmt, ...)
{
	va_list marker;
	int ret;
	char buffer [MAXCMD],*cptr;

	va_start( marker, fmt );
	ret = vsprintf( buffer, fmt, marker );
	va_end( marker );
	for (cptr = buffer; *cptr;)
		pc (*cptr++);
	return ret;
}

void _show_stat (char *workbuf)
{
	struct text_info *Info;
	int ScrollSave = SetScroll (0);

	Info = CurrentWindow;
	SetWindow (&StatusLineWindow);
	textattr (STATUSLINECOLOR);
	gotoxy (1,1);
	while (*workbuf)
	{
		if (*workbuf == 0x01)
		{
			textattr (STATUSLINECOLOR);
			workbuf++;
		}
		else if (*workbuf == 0x02)
		{
			textattr (STATUSITEMCOLOR);
			workbuf++;
		}
		else putch (*workbuf++);
	}
        clreol ();
	SetWindow (Info);
	SetScroll (ScrollSave);
}

void beep (void)
{
	sound (1000);
	delay (50);
	nosound ();
}

void show_stat (int force)
{
	char *modename, *Logname, PortName [40], workbuf [80];
	int port, speed;
	static unsigned last, last_stat = 0;
	unsigned mask, sigstat;

	if (GetPortInfo (PortName, &port, &speed))
	{
		if (force || last) _show_stat ("                            V" VERSTRING " (C) 1990-94 Scott Howard                      ");
		last = 0;
		return;
	}
	sigstat = GetSigStatus ();
	mask = get_statmask ();
	if (force || !last || (last_stat != sigstat))
	{
		last = 1;
		last_stat = sigstat & ~TARGETBRKPT;
		if (sigstat & TARGETINVALID) modename = "???????";
		else if (sigstat & TARGETBRKPT)
		{
			modename = "BRKPNT ";
			beep ();
		}
		else modename = sigstat & TARGETSTOPPED ? "STOPPED" : "RUNNING";
		if (LogFile) Logname = "Log \x02ON\x01";
		else Logname = "      ";
		sprintf (workbuf," MCU: \x02%s\x01 Port: \x02%s%d\x01 Halt: \x02%c\x01 Reset: \x02%c\x01 %s V" VERSTRING " (C) 1990-94 Scott Howard",
			modename, PortName, port,
			(!(sigstat & TARGETINVALID) && (mask & TARGETHALT)) ? (sigstat & TARGETHALT ? '1' : '0') : '?',
			(!(sigstat & TARGETINVALID) && (mask & TARGETRESET)) ? (sigstat & TARGETRESET ? '1' : '0') : '?',
			Logname);
		_show_stat (workbuf);
	}
}

void HandleColumn (char c)
{
	switch (c)
	{
		case	'\b':
			if (column) column--;
			break;

		case '\t':
			column += column % 8;
			break;

		case	'\n':
			column = 0;
			break;

		default:
			column++;
	}
}

void backspace (void)
{
	int row, col;

	col = wherex (); row = wherey ();
	if (col != 1) col--;
	if (OutputBuffer.BuffUsed) OutputBuffer.BuffUsed--;
	if (ThrowAway) return;
	gotoxy (col, row);
	cputs (" ");
	gotoxy (col, row);
}

void tab (void)
{
	int col;
	static char tab_buf [] = "        ";

	col = wherex ();
	if (!ThrowAway)
		cputs (tab_buf + col % 8);
	LogPuts (tab_buf + col % 8);
}

void ot (char c)
{
	if (c == '\b') backspace ();
	else if (c == '\t') tab ();
	else if (c == '\n')
	{
		LogPutch (c);
		OutputLogBuffer ();
		if (!ThrowAway)
		{
			putch ('\r');
			putch ('\n');
		}
	}
	else
	{
		if (!ThrowAway)
			putch (c);
		LogPutch (c);
	}
}

void pc (char c)
{
	ot (c);
	HandleColumn (c);
/*	if (c != '\r' && !(column % 80) && !buffer) clreol (); */
}

int cmd_size (char *cmd)
{
	if (toupper (*cmd) == 'B') return BYTESIZE;
	if (toupper (*cmd) == 'W') return WORDSIZE;
	if (toupper (*cmd) == 'L') return LONGSIZE;
	return WORDSIZE;
}

/* general subroutines
ntoh makes hex character out of nibble */

int ntoh (BYTE n)
{
	n &= 0xf;
	if (n > 9) n += ('A' - 10);
	else n += '0';
	return n;
}

/* btoh prints hex value of byte */

void btoh (LONG word)
{
	pc (ntoh (word >> 4));
	pc (ntoh (word));
}

/* wtoh prints hex value of word */

void wtoh (LONG word)
{
	btoh (word >> 8);
	btoh (word);
}


/* ltoh prints hex value of long */

void ltoh (LONG word)
{
	wtoh (word >> 16);
	wtoh (word);
}

LONG (*getmem[]) (LONG)  = {GETBYTE,GETWORD,GETLONG};
void (*putmem[]) (LONG, LONG) = {PUTBYTE,PUTWORD,PUTLONG};
void (*fillmem[]) (LONG, LONG) = {FILLBYTE,FILLWORD,FILLLONG};
void (*printhex[]) (LONG) = {btoh,wtoh,ltoh};

/* skipwhite moves pointer to first non-white space character */

char *skipwhite (char **s)
{
	int c;

	for (;;)
	{
		c = **s;
		if (!isspace (c)) break;
		(*s)++;
	}
	return *s;
}

/* htoc returns binary value of hex character */

int htoc (char c)
{
	c = toupper (c);
	if (c > '9') c -= ('A' - 10);
	else c -= '0';
	return c;
}

/* gethex converts text string at where to long at data
 * string must be expression as evaluated by eval ()
 * returns 0 if converted OK, otherwise non-zero */

int gethex (char *where, LONG *data)
{

	*data = eval (where);
	return eval_error;
}

void print (char *s)
{
	while (*s) pc (*s++);
}

char *set_opt (char *string)
{
	cmd_option = string;
	opsize = cmd_size (cmd_option);
	while (*string && !isspace (*string)) string++;
	return string;
}

void parsecmd (char *string, int *count, char **ptrs, int maxparam)
{
	for ( *count = 0; *count < maxparam;)
	{
		skipwhite (&string);
		if (!*string) break;
		if (*string == OPTION_CHAR)
		{
			*string++ = '\0';
			string = set_opt (string);
			continue;
		}
		ptrs [(*count)++] = string;
		while (*string && !isspace (*string))
		{
			if (*string == OPTION_CHAR)
			{
				*string++ = '\0';
				string = set_opt (string);
			}
			else string++;
		}
		if (!*string) break;
		*string++ = '\0';
	}
}

void read_config (void);
void SetUpPort (void);

#pragma argsused
void init (int argc, char **argv)
{
	ctrlbrk (HandleCtrlBreak);
	harderr (handler);
	strcpy (ExecPath, argv [0]);
	macro = (FILE *) NULL;
	SetUpScreen (1);
	DisassemblyAddress = malloc (sizeof (LONG) * GetWindowHeight (&DisassemblyWindow));
	if (UseWindows) DisassemblyCount = GetWindowHeight (&DisassemblyWindow);
	else DisassemblyCount = GetWindowHeight (&CommandLineWindow) / 2;
	read_config ();
	SetUpPort ();
	pf (signon);
	if (setjmp (cmd_jmp))
	{
		SetWindow (&CommandLineWindow);
		goto skip;
	}
	show_stat (1);
skip:
	return;
}

FILE *path_open (char *filename, char *mode, char *alt_path)
{
	char *ptr, fn [MAXCMD];
	FILE *result;

	if (!(result = fopen (filename, mode)))
		if (alt_path)
		{
			strcpy (fn, alt_path);
			if ((ptr = strrchr (fn, '\\')) != 0)
			{
				*(ptr+1) = '\0';
				strcat (fn, filename);
				result = fopen (fn,"r");
			}
		}
	return result;
}

#pragma argsused

void read_config (void)
{
	FILE *config;
	char err;
	int dummy, ColourCount;

	if ((config = path_open (CFG_FILE, "r", ExecPath)) == NULL)
	{
		pf ("No configuration file found\n");
		return;
	}
	err = (fscanf (config,"%3s%d\n%d\n",ConfigPortName,&ConfigPort,&ConfigSpeed) != 3);
	if (!err)
		for (ColourCount = 0; ColourCount < ATTRIBUTECOUNT; )
			if (fscanf (config, "%d\n", &dummy) == 1)
			{
				Attributes [ColourCount] = dummy;
				ColourCount++;
			}
			else ColourCount = ATTRIBUTECOUNT;
	fclose (config);
	if (err)
	{
		pf ("Format error in '%s'\n",CFG_FILE);
		return;
	}
}

void SetUpPort (void)
{
	if (setjmp (cmd_jmp))
	{
		SetWindow (&CommandLineWindow);
		goto skip;
	}
	if (!init_serial (ConfigPortName,ConfigPort,ConfigSpeed))
	{
		if (UseWindows)
		{
			DISPLAYREGISTERS;
			DisplayCode (GETREG (REG_PC), REDISPLAY);
		}
	}
	else
skip:
		pf ("Can't open port named in config file " CFG_FILE "\n");
}

#pragma argsused
int quit (int argc, char **argv)
{
	CloseLog ();
	deinit_serial ();
	RestoreScreen (screensave);
	exit (0);
        return 0;
}

void show_error (char *string)
{
	int count = eval_ptr - string;

	pf ("%s\n",string);
	while (count--) pc ('-');
	pf ("^\n");
}

void report_error (int type, char *string)
{
	switch (type)
	{
		case    OP_ERROR:
			pf ("Invalid operator\n");
			break;

		case    SYM_ERROR:
			pf ("Undefined symbol: '%s'\n", last_symbol);
			break;

		case    CHAR_ERROR:
			show_error (string);
			pf ("Illegal character\n");
	}
}

void closemacro (void)
{
	fclose (macro);
	macro = (FILE *) NULL;
}

main (argc,argv)
int argc;
char **argv;
{
	char *response, cmd [MAXCMD];
	int ctr;

	init (argc,argv);
	if (setjmp (cmd_jmp))
	{
		cmd_error = -99;
		SetWindow (&CommandLineWindow);
		goto serial_error;
	}
	*last_cmd = *cmd = '\0';
	for (ctr = 1; ctr < argc; ctr++)
	{
		strcat (cmd,argv [ctr]);
		strcat (cmd," ");
	}
	if (argc > 1) execute (cmd);
	else show_settings ();

	for (cmd_error = 0;;)
	{
serial_error:
		if (macro && (CtrlBreakCount || cmd_error))
		{
			if (cmd_error) pf ("Error code %d\n", cmd_error);
			for (response = " "; !strchr ("yYnN",*response);)
			{
				pf ("Continue 'DO' Execution (Y/N)? ");
				gs (cmd);
				response = cmd;
				skipwhite (&response);
			}
			if (toupper (*response) == 'N')
			{
				fclose (macro);
				macro = NULL;
				*last_cmd = '\0';
			}
		}
		textattr (PROMPTCOLOR);
		pf (uprompt);
		textattr (COMMANDINPUTCOLOR);
		getline (cmd);
		textattr (COMMANDOUTPUTCOLOR);
		CtrlBreakCount = 0;
		cmd_error = execute (cmd);
	}
}

int execute (char *line)
{
	int counter,argc,retval;
	char *exptr,*cptr,*argv [MAXARG], msgname [MAXCMD];
	FILE *msgfile;

	skipwhite (&line);
	if (!*line) line = " ";
	else if (*line == '*') return 0;
	for (retval = 0; !retval && *line;)
	{
		show_stat (0);
		for (cptr = commandline;*line && *line != SEP_CHAR;)
			*cptr++ = *line++;
		*cptr = '\0';
		cmd_option = "";
		opsize = WORDSIZE;
		if (*line == SEP_CHAR) line++;
		exptr = commandline;
		skipwhite (&exptr);
		if (!*exptr) strcpy (commandline,last_cmd);
		else strcpy (last_cmd, commandline);
		parsecmd (commandline,&argc,argv,MAXARG);
		if (!argc) continue;
		trash_cache ();
		for (counter = 0; commands [counter].name; counter++)
		{
			if (!stricmp (commands[counter].name,argv[0]))
			{
				retval = (*commands[counter].function) (argc,argv);
				break;
			}
		}
		if (!commands[counter].name)
		{
			if ((retval = ipd (argc, argv, PROG_EXT, 1)) == PROG_LOADERR)
			{
				pf ("Undefined command: %s\n",argv [0]);
				*last_cmd = '\0';
			}
			if (retval > 0)
			{
				strcpy (msgname, argv [0]);
				strcat (msgname, MSG_EXT);
				if ((msgfile = path_open (msgname, "r", ExecPath))!= 0)
				{
					for (counter = 1 + retval; counter; counter--)
						if (fgets (msgname, MAXCMD, msgfile) != msgname)
							break;
					if (!counter) pf (msgname);
					fclose (msgfile);
				}
			}
		}
	}
	return retval;
}

#define PC_QUIT         0       /* quit execution, return to BD32 */
#define PC_PUTS         1       /* put string to console */
#define PC_PUTCHAR      2       /* put single character to console */
#define PC_GETS         3       /* get \n-terminated string from console */
#define PC_GETCHAR      4       /* get single character from console */
#define PC_GETSTAT      5       /* return non-zero if char waiting at console */
#define PC_FOPEN        6       /* open disk file */
#define PC_FCLOSE       7      /* close disk file */
#define PC_FREAD        8       /* read disk file */
#define PC_FWRITE       9       /* write disk file */
#define PC_FTELL        10      /* report current file position */
#define PC_FSEEK        11      /* seek disk file to new position */
#define PC_FGETS        12      /* read \n-terminated string from file */
#define PC_FPUTS        13      /* write \n-terminated string to file */
#define PC_EVAL         14      /* evaluate expression in null-erm. string */
#define PC_FREADSREC    15      /* read S-record from file into buffer */

#define SREC_EOF        1       /* end of file before S9 record */
#define SREC_S9         2       /* S9 record reached */
#define SREC_CHECKSUM   3       /* checksum error in record */
#define SREC_FORMAT     4       /* S-record format error */

#define PC_ARG_MAX      10      /* max no. of command arguments for prog. commands */

void pc_getstring (char *dest, LONG src)
{
	int c;

	c = *dest++ = GETBYTE (src++);
	while (c)
		c = *dest++ = GETBYTE (src++);
}

void pc_putstring (LONG dest, char *src)
{
	int c;

	PUTBYTE (dest++,c = *src++);
	while (c) FILLBYTE (dest++, c = *src++);
}

void pc_getbuff (char *dest, LONG src, size_t count)
{
	if (!count) return;
	*dest++ = GETBYTE (src++);
	while (--count) *dest++ = DUMPBYTE (src++);
}

void pc_putbuff (LONG dest, char *src, size_t count)
{
	if (!count) return;
	PUTBYTE (dest++, *src++);
	while (--count) FILLBYTE (dest++, *src++);
}

void send_srec (srecord *data, LONG address)
{
	BYTE ctr = data->reclen, *where = (BYTE *) data->bytes;
	LONG record_address = data->address;

	PUTBYTE (address++, data->rectype - '0');
	FILLBYTE (address++, data->reclen);
	FILLBYTE (address++, (BYTE) (record_address >> 24));
	FILLBYTE (address++, (BYTE) (record_address >> 16));
	FILLBYTE (address++, (BYTE) (record_address >> 8));
	FILLBYTE (address++, (BYTE) record_address);
	for (ctr -= 4; ctr; ctr--)
		FILLBYTE (address++, *where++);
}

LONG SysCall (int *quitflag)
{
	LONG LongTemp, a0, d0, d1, d2, retval = 0;
	size_t actual, bytecount;
	char *ok, buff [MAXCMD], buff1 [MAXCMD];
	srecord sbuff;

	set_fc ();
	d0 = GETREG (REG_D0);
	switch (d0)
	{
		case PC_QUIT:
		retval = GETREG (REG_D1);
		*quitflag = 1;
		break;

		case PC_PUTS:
		pc_getstring (buff, GETREG (REG_A0));
		pf (buff);
		PUTREG (REG_D0, 0L);
		break;

		case PC_PUTCHAR:
		pc (GETREG (REG_D1));
		PUTREG (REG_D0, 0L);
		break;

		case PC_GETS:
		getline (buff);
		buff [min (MAXCMD-1,((int) d0))] = '\0';
		pc_putstring (GETREG (REG_A0), buff);
		PUTREG (REG_D0, 0L);
		break;

		case PC_GETCHAR:
		PUTREG (REG_D0, (LONG) gc (1));
		break;

		case PC_GETSTAT:
		PUTREG (REG_D0,(LONG) key_ready ());
		break;

		case PC_FOPEN:
		pc_getstring (buff, GETREG (REG_A0));
		pc_getstring (buff1, GETREG (REG_A1));
		PUTREG (REG_D0, (LONG) fopen (buff, buff1));
		break;

		case PC_FREAD:
		a0 = GETREG (REG_A0);
		d1 = GETREG (REG_D1);
		LongTemp = d2 = GETREG (REG_D2);
		while (d2)
		{
			bytecount = min (MAXCMD, (short) d2);
			actual = fread (buff, sizeof (BYTE),
				bytecount, (FILE *) d1);
			if (actual) pc_putbuff (a0, buff, actual);
			a0 += actual;
			d2 -= actual;
			if (actual != bytecount) break;
		}
		PUTREG (REG_D0, LongTemp - d2);
		break;

		case PC_FWRITE:
		a0 = GETREG (REG_A0);
		d1 = GETREG (REG_D1);
		LongTemp = d2 = GETREG (REG_D2);
		while (d2)
		{
			bytecount = min (MAXCMD, (short) d2);
			pc_getbuff (buff, a0, bytecount);
			actual = fwrite (buff, sizeof (BYTE),
				bytecount, (FILE *) d1);
			a0 += actual;
			d2 -= actual;
			if (actual != bytecount) break;
		}
		PUTREG (REG_D0, LongTemp - d2);
		break;

		case PC_FTELL:
		PUTREG (REG_D0, (LONG) ftell ((FILE *)GETREG (REG_D1)));
		break;

		case PC_FSEEK:
		PUTREG (REG_D0, (LONG) fseek ((FILE *)GETREG (REG_D1),
			GETREG (REG_D2), (short) GETREG (REG_D3)));
		break;

		case PC_FCLOSE:
		PUTREG (REG_D0, (LONG) fclose ((FILE *)GETREG (REG_D1)));
		break;

		case PC_FGETS:
		ok = fgets (buff, MAXCMD, (FILE *)GETREG (REG_D1));
		if (ok) pc_putstring (GETREG (REG_A0), buff);
		PUTREG (REG_D0, (LONG) !ok);
		break;

		case PC_FPUTS:
		pc_getstring (buff, GETREG (REG_A0));
		PUTREG (REG_D0,(LONG) fputs (buff, (FILE *)GETREG (REG_D1)));
		break;

		case PC_EVAL:
		pc_getstring (buff, GETREG (REG_A0));
		PUTREG (REG_D0, (LONG) gethex (buff, &d1));
		PUTREG (REG_D1, d1);
		break;

		case PC_FREADSREC:
		d0 = (LONG) do_srec (&sbuff, (FILE *) GETREG (REG_D1));
		if (!d0 || d0 == SREC_S9)
			send_srec (&sbuff,GETREG (REG_A0));
		PUTREG (REG_D0, d0);
		break;

		default:
		DISPLAYREGISTERS;
		pf ("*** Target Driver Halted ***\nPress any key to continue, <esc> to abort: ");
		retval = (gc (1) == '\x1b');
		pf ("\n");
		if (retval)
		{
			retval = -97;
			*quitflag = 1;
		}
	}
	restore_fc ();
	return retval;
}

LONG do_prog (LONG address)
{
	LONG retval = 0;
	int quitflag = 0;

	run_chip (address);
	while (!quitflag)
	{
		while (!(GetSigStatus () & TARGETSTOPPED))
			if (check_esc ())
			{
				stop_chip ();
				pc ('\n');
				quitflag = 1;
				retval = -1;
				break;
			}
		if (quitflag) break;
		retval = SysCall (&quitflag);
		if (!quitflag) run_chip (0L);
	}
	return retval;
}

int do_driver (int argc, char **argv)
{
	LONG NewOffset;

	if (argc > 2)
	{
		pf ("usage: driver [<expression>]\n");
		return (-1);
	}
	if (argc == 2)
	{
		if (gethex (argv [1], &NewOffset))
		{
			report_error (eval_error, argv [1]);
			return eval_error;
		}
		DriverOffset = NewOffset;
	}
	pf ("Driver load address is $%lX\n", DriverOffset);
	return 0;
}

int do_syscall (int argc, char **argv)
{
	if (argc > 2)
	{
		pf ("usage: syscall [ON | OFF]\n");
		return (-1);
	}
	if (argc == 2)
	{
		if (!stricmp ("ON", argv [1])) SysCallEnabled = 1;
		else if (!stricmp ("OFF", argv [1])) SysCallEnabled = 0;
		else
		{
			pf ("Illegal parameter: %s\nParameter must be 'on' or 'off'\n",
				argv [1]);
			return -1;
		}
	}
	pf ("Syscalls are %s during trace\n",
		SysCallEnabled ? "enabled" : "disabled");
	return 0;
}

int do_cload (int argc, char **argv)
{
	int retval = ipd (argc - 1, argv + 1, 0, 0);
	LONG ExecStart;

	if (retval) return retval;
	PUTREG (REG_PC, ExecStart = ProgStart + GETLONG (ProgStart));
	pf ("'%s.d32' command driver loaded\nDriver offset $%lX\nExecution address $%lX\n",
		argv [1], ProgStart, ExecStart);
	if (UseWindows) DISPLAYCODE;
	return 0;
}

int ipd (int argc, char **argv, char *extension, int run)
{
	char buff [MAXCMD];
	LONG strings, array;
	int limit,ctr;

	strcpy (buff, argv [0]);
	if (!extension)
		extension = PROG_EXT;
	strcat (buff, extension);
	if (!(infile = path_open (buff, "r", ExecPath))) return PROG_LOADERR;
	fclose (infile);		/* don't leave file open in case */
					/* of error downloading strings */
	stop_chip ();
	set_fc ();
	strings = PTR_SIZE * PC_ARG_MAX + (array = DriverOffset);
	PUTREG (REG_D0, (LONG) (limit = min (argc, PC_ARG_MAX)));
	PUTREG (REG_A0, array);
	PUTREG (REG_SR, (LONG) 0x2700);
	for (ctr = 0; ctr < limit; ctr++)
	{
		pc_putstring (strings, argv [ctr]);
		PUTLONG (array, strings);
		array += PTR_SIZE;
		strings += 1 + strlen (argv [ctr]);
	}
	restore_fc ();
	ProgStart = strings;
	if (ProgStart & 1) ProgStart++;
	pf ("($%lX)", ProgStart);
	if (!(infile = path_open (buff, "r", ExecPath))) return PROG_LOADERR;
	if (_do_load (ProgStart, infile))
	{
		pf ("Can't load file %s\n", buff);
		fclose (infile);
		return PROG_LOADERR;
	}
	fclose (infile);
	PUTREG (REG_A5, ProgStart);
	if (run) return (short) do_prog (ProgStart + GETLONG (ProgStart));
	if (UseWindows)
	{
		DISPLAYREGISTERS;
		DISPLAYCODE;
	}
	return 0;
}

#pragma argsused
int do_cls (int argc, char **argv)
{
	clrscr ();
	return 0;
}

int do_go (int argc, char **argv)
{
	LONG PCSave = GETREG (REG_PC), where;

	*last_cmd = '\0';
	if (!(GetSigStatus () & TARGETSTOPPED))
	{
		print ("Processor already running - Type 'Stop' to halt\n");
		return 0;
	}
	if (argc == 1) where = PCSave;
	else if (gethex (argv [1], &where))
	{
		report_error (eval_error, argv [1]);
		return eval_error;
	}
	where &= ~(LONG) 1;
	if (UseWindows) DisplayCode (PCSave, BLANKPC);
	if (match_bp (where))
	{
		step_chip ();
		run_chip (0L);
	}
	else run_chip (where);
	return 0;
}

void CollectArguments (char *cmd, int argc, char **argv)
{
	int ctr;

	*cmd = '\0';
	for (ctr = 0; ctr < argc; ctr++)
	{
		strcat (cmd,argv [ctr]);
		strcat (cmd," ");
	}
}

int RepeatUntilTrue (int (*function) (int, char **), int argc, char **argv)
{
	char EvalString [MAXCMD];
	LONG EvalResult;

	CollectArguments (EvalString, argc - 1, &argv [1]);
	while (1)
	{
		(*function) (0, 0);
		if (gethex (EvalString, &EvalResult))
		{
			report_error (eval_error, EvalString);
			return -1;
		}
		if (EvalResult)
		{
			pf ("Test condition TRUE - execution stopped\n");
			break;
		}
		if (check_esc ())
		{
			pf ("<ESC> pressed - execution stopped\n");
			break;
		}
	}
	beep ();
	return 0;
}

void TraceRegisterDisplay (void)
{
	SetWindow (&RegisterWindow);
	gotoxy (1,1);
	DISPLAYREGISTERS;
	SetWindow (&SmallCommandLineWindow);
}

int do_trace (int argc, char **argv)
{
	int step = 1, dummy;
	LONG PCSave;

	SelectSmallCommandLineWindow ();
	if (argc > 1)
	{
		pf ("Tracing until condition evaluates TRUE (non-zero) - press <ESC> to halt\n");
		return RepeatUntilTrue (do_trace, argc, argv);
	}
	stop_chip ();
	PCSave  = GETREG (REG_PC);
	if (SysCallEnabled)
		if (GETWORD (PCSave) == BGND)
		{
			SysCall (&dummy);
			PUTREG (REG_PC, PCSave + 2L);
			step = 0;
		}
	if (step) step_chip ();
	if(UseWindows)
	{
		if (TraceRedisplayNeeded)
		{
			DisplayCode (GETREG (REG_PC), REDISPLAY);
			TraceRedisplayNeeded = 0;
		}
		else
		{
			DisplayCode (PCSave, BLANKPC);
			DisplayCode (GETREG (REG_PC), OPTIMALREDISPLAY);
			DISPLAYREGISTERS;
		}
	}
	else
	{
		TraceRegisterDisplay ();
		Disassemble (GETREG (REG_PC), 1, 1, 0);
	}
	return 0;
}

struct SubOpTable
{
	LONG Opcode, Mask;
	unsigned ExtraBytes;
} SubroutineOps [] =
{
	{0x61000000L, 0xffff0000L, 4},	/* bsr, word offset */
	{0x61ff0000L, 0xffff0000L, 6},	/* bsr, long offset */
	{0x61000000L, 0xff000000L, 2},	/* bsr, byte offset */
	{0x4e900000L, 0xfff80000L, 2},	/* jsr (An) */
	{0x4ea80000L, 0xfff80000L, 4},	/* jsr d16(An) */
	{0x4eb00000L, 0xfff80100L, 4},	/* jsr d8(An,Xn.[wl]*scale) */
	{0x4eb00110L, 0xfff80130L, 4},	/* jsr 0(An,Xn.[wl]*scale) */
	{0x4eb00120L, 0xfff80130L, 6},	/* jsr bd16(An,Xn.[wl]*scale) */
	{0x4eb00130L, 0xfff80130L, 8},	/* jsr bd32(An,Xn.[wl]*scale) */
	{0x4eb80000L, 0xffff0000L, 4},	/* jsr n.w */
	{0x4eb90000L, 0xffff0000L, 6},	/* jsr n.l */
	{0x4eba0000L, 0xffff0000L, 4},	/* jsr d16(PC) */
	{0x4ebb0000L, 0xffff0100L, 4},	/* jsr d8(PC,Xn.[wl]*scale) */
	{0x4ebb0110L, 0xffff0130L, 4},	/* jsr 0(PC,Xn.[wl]*scale) */
	{0x4ebb0120L, 0xffff0130L, 6},	/* jsr bd16(PC,Xn.[wl]*scale) */
	{0x4ebb0130L, 0xffff0130L, 8},	/* jsr bd32(PC,Xn.[wl]*scale) */
	{0x4e4f0000L, 0xffffffffL, 4},	/* syscall inchr */
	{0x4e4f0001L, 0xffffffffL, 4},	/* syscall instat */
	{0x4e4f0002L, 0xffffffffL, 4},	/* syscall inln */
	{0x4e4f0003L, 0xffffffffL, 4},	/* syscall readstr */
	{0x4e4f0004L, 0xffffffffL, 4},	/* syscall readln */
	{0x4e4f0005L, 0xffffffffL, 4},	/* syscall chkbrk */
	{0x4e4f0020L, 0xffffffffL, 4},	/* syscall outchr */
	{0x4e4f0021L, 0xffffffffL, 4},	/* syscall outstr */
	{0x4e4f0022L, 0xffffffffL, 4},	/* syscall outln */
	{0x4e4f0023L, 0xffffffffL, 4},	/* syscall write */
	{0x4e4f0024L, 0xffffffffL, 4},	/* syscall writeln */
	{0x4e4f0025L, 0xffffffffL, 4},	/* syscall writdln */
	{0x4e4f0026L, 0xffffffffL, 4},	/* syscall pcrlf */
	{0x4e4f0027L, 0xffffffffL, 4},	/* syscall erasln */
	{0x4e4f0028L, 0xffffffffL, 4},	/* syscall writd */
	{0x4e4f0029L, 0xffffffffL, 4},	/* syscall sndbrk */
	{0x4e4f0040L, 0xffffffffL, 4},	/* syscall tm_ini */
	{0x4e4f0041L, 0xffffffffL, 4},	/* syscall tm_str0 */
	{0x4e4f0042L, 0xffffffffL, 4},	/* syscall tm_rd */
	{0x4e4f0043L, 0xffffffffL, 4},	/* syscall delay */
	{0x4e4f0063L, 0xffffffffL, 4},	/* syscall return */
	{0x4e4f0064L, 0xffffffffL, 4},	/* syscall bindec */
	{0x4e4f0067L, 0xffffffffL, 4},	/* syscall changev */
	{0x4e4f0068L, 0xffffffffL, 4},	/* syscall strcmp */
	{0x4e4f0069L, 0xffffffffL, 4},	/* syscall mulu32 */
	{0x4e4f006aL, 0xffffffffL, 4},	/* syscall divu32 */
	{0x4e400000L, 0xfff00000L, 2},	/* trap #n */
	{0x4e760000L, 0xffff0000L, 2},	/* trapv */
	{0x50fa0000L, 0xf0ff0000L, 4},	/* trapcc word */
	{0x50fb0000L, 0xf0ff0000L, 6},	/* trapcc long */
	{0x50fc0000L, 0xf0ff0000L, 2},	/* trapcc <none> */
	{0, 0, 0 }
};

int OpSkipCount (LONG Address)
{
	LONG Opword = GETLONG (Address);
	struct SubOpTable *Op = SubroutineOps;

	while (Op->ExtraBytes)
		if ((Opword & Op->Mask) == Op->Opcode) return Op->ExtraBytes;
		else Op++;
	return 0;
}

int do_step (int argc, char **argv)
{
	int count, ByteCount;
	LONG PCSave;

	SelectSmallCommandLineWindow ();
	if (argc > 1)
	{
		pf ("Stepping until condition evaluates TRUE (non-zero) - press <ESC> to halt\n");
		return RepeatUntilTrue (do_step, argc, argv);
	}
	stop_chip ();
	if (!(ByteCount = OpSkipCount (PCSave = GETREG (REG_PC))))
		return do_trace (argc, argv);
	for (count = 0; count < BreakpointCount ();count++)
		if (! getmask (count)) break;
	if (count == BreakpointCount ())
	{
		pf ("Can't set any more breakpoints (limit is %d)\n", count);
		return -4;
	}
	StepBreakpoint = 1 + br (count, PCSave + ByteCount, DefaultBreakpointMask);
	if (UseWindows) DisplayCode (PCSave, BLANKPC);
	run_chip (0L);
	return 0;
}

int port_usage (void)
{
	pf ("Usage: port <port> <speed>\n");
	return -1;
}

int do_port (int argc, char **argv)
{
	int port;
	LONG speed;
	char name [40], *ptr;

	*last_cmd = '\0';
	if (argc == 1)
	{
		show_settings ();
		return 0;
	}
	if (argc != 3) return port_usage ();
	for (ptr = argv [1]; *ptr; ptr++) *ptr = toupper (*ptr);
	if (sscanf (argv[1],"%3s%d",name,&port) != 2) return port_usage ();
	if (gethex (argv[2],&speed))
	{
		pf ("Invalid baud rate: %s\n", argv [2]);
		return -2;
	}
	if (init_serial (name, port, (int) speed))
	{
		pf ("Can't initialize %s%d at %ld speed\n", name, port, speed);
		return -3;
	}
	show_settings ();
	if (UseWindows)
	{
		DISPLAYREGISTERS;
		DisplayCode (GETREG (REG_PC), REDISPLAY);
	}
	return 0;
}

#pragma argsused
int do_stop (int argc, char **argv)
{
	*last_cmd = '\0';
	stop_chip ();
	if (UseWindows)
	{
		DISPLAYREGISTERS;
		DisplayCode (GETREG (REG_PC), OPTIMALREDISPLAY);
	}
	return 0;
}

#pragma argsused
int do_restart (int argc, char **argv)
{
	restart_chip ();
	if (!UseWindows)
	{
		SelectSmallCommandLineWindow ();
		TraceRegisterDisplay ();
	}
	if (UseWindows)
	{
		DISPLAYREGISTERS;
		DisplayCode (GETREG (REG_PC), OPTIMALREDISPLAY);
	}
	else Disassemble (GETREG (REG_PC), 1, 1, 0);
	return 0;
}

#pragma argsused
int do_reset (int argc, char **argv)
{
	*last_cmd = '\0';
	if (UseWindows) DisplayCode (GETREG(REG_PC), BLANKPC);
	reset_chip ();
	return 0;
}

int do_memory (int argc, char **argv)
{
	static LONG addrsave = (LONG) 0, address, stuff;
	char *cmd_ptr, commandline [MAXCMD], lastbuf;
	extern char *eval_ptr;
	char *save, *last_ptr;

	if (!stricmp (cmd_option,"DI")) return do_asm (argc,argv);
	strcpy (last_cmd, "mm");
	if (argc > 1)
	{
		if (gethex (argv [1], &address))
		{
			report_error (eval_error, argv [1]);
			return eval_error;
		}
	}
	else address = addrsave;

	if (opsize) address &= ~ (LONG) 1;      /* even addresses only for word/long */
	for (lastbuf = '\0';;)
	{
		ltoh (address);
		stuff = GETMEM (address);
		if (!(stuff & ~0xFF)
		&& (stuff & 0x7f) >= ' '
		&& (stuff & 0x7f) != 0x7f)
			pf (" '%c' ",(int) GETMEM (address) & 0x7f);
		else print ("     ");
		PRINTHEX (GETMEM (address));
		pc (' ');
		cmd_ptr = getline (commandline);
		last_ptr = cmd_ptr + strlen (cmd_ptr) - 1;
		for (save = (char *) NULL; last_ptr >= cmd_ptr; last_ptr--)
		{
			if (isspace (*last_ptr)) continue;
			save = strchr (".=^vV",*last_ptr);
			if (save)
			{
				*last_ptr = '\0';
				lastbuf = *save;
			}
			break;
		}
		if (gethex (cmd_ptr, &stuff))
		{
			if (eval_error != NOCHAR_ERROR)
			{
				report_error (eval_error, cmd_ptr);
				continue;
			}
		}
		else
		{
			PUTMEM (address,stuff);
			if (UseWindows) DisplayCode (address, UPDATEADDRESS);
		}
		if (!save) save = &lastbuf;
		if (*save == '.')
		{
			addrsave = address;
			break;
		}
		if (*save == '^')
		{
			address -= 1<<opsize;
		}
		else if (*save == '=') continue;
		else address += 1<<opsize;
	}
	return 0;
}

int key_ready (void)
{
	return kbhit ();
}

int gc (int update)
{
	int key;

	while (!key_ready ()) if (update) show_stat (0);
	key = getch ();
	ExtKey = key ? 0 : getch ();
	return key;
}

/* check_esc returns non-zero if escape pressed on keyboard */

int check_esc (void)
{
	while (key_ready ())
		if (gc (1) == ESC_KEY) return 1;
	return 0;
}

int do_dump (int argc, char **argv)
{
	static LONG startsave = (LONG) 0, countsave = (LONG) 0x100;
	LONG start,end,address,linesave,count;
	char c;
	int frozen;
	struct text_info *OldWindow = CurrentWindow;
	unsigned WinHeight, LineCounter = 0;

	enable_cache ();
	if (!stricmp (cmd_option,"DI")) return do_disasm (argc,argv);
	strcpy (last_cmd, "md");
	count = countsave;
	if (argc == 3)
	if (gethex (argv [2],&count))
	{
		report_error (eval_error, argv [2]);
		return eval_error;
	}
	if (opsize) count &= ~(LONG) 1;      /* even counts only for word/long */
	if (!count) count = 16;
	if (argc < 2) start = startsave;
	else if (gethex (argv [1], &start))
	{
		report_error (eval_error, argv [1]);
		return eval_error;
	}

	if (opsize) start &= ~(LONG) 1;      /* even addresses only for word/long */
	if (UseWindows)
	{
		SetWindow (&DisassemblyWindow);
		textattr (COMMANDOUTPUTCOLOR);
		clrscr ();
		end = start + 16 * (WinHeight = GetWindowHeight (&DisassemblyWindow));
	}
	else end = start + count;
	pauselinecount = 0;
	frozen = stop_chip ();
	set_fc ();
	if (opsize) end &= ~ (LONG) 1;
	if (opsize == LONGSIZE) end &= ~ (LONG) 3;
	for (address = start; !(address >= end && address <= end + 16);)
	{
		if (!UseWindows) if (check_esc ()) break;
		linesave = address;
		ltoh (address);
		print ("  ");
		do
		{
			PRINTHEX (GETMEM (address));
			address += 1<<opsize;
			print (" ");
		}
		while (address & 0xf);
		print ("  ");
		address = linesave;
		do
		{
			c = GETBYTE (address);
			if ((c & 0x7f) >= ' '
			&&  (c & 0x7f) != 0x7f)
				pc (c & 0x7f);
			else pc ('.');
			address++;
		}
		while (address & 0xf);
		if (UseWindows && ++LineCounter == WinHeight)
		{
			ThrowAway = 1;
			print ("\n");
			ThrowAway = 0;
		}
		else print ("\n");
		if (!UseWindows) if (pause_check ()) break;
	}
	restore_fc ();
	if (frozen) run_chip (0L);
	startsave = address;
	countsave = count;
	if (UseWindows)
	{
		SetWindow (OldWindow);
		TraceRedisplayNeeded = 1;
	}
	return 0;
}

static char *sizenames [] = {"byte", "word", "long word"};

int do_bf (int argc, char **argv)
{
	LONG start,end,address,data;
	int frozen;

	*last_cmd = '\0';
	if (argc < 4)
	{
		pf ("usage: bf start end data\n");
		return -99;
	}
	if (gethex (argv [1],&start))
	{
		report_error (eval_error, argv [1]);
		return eval_error;
	}
	if (gethex (argv [2],&end))
	{
		report_error (eval_error, argv [2]);
		return eval_error;
	}
	if (gethex (argv [3],&data))
	{
		report_error (eval_error, argv [3]);
		return eval_error;
	}
	if (end < start)
	{
		pf ("Start address (%lX) must be lower than end address (%lX)\n", start, end);
		return -98;
	}
	if (opsize) start &= ~(LONG) 1;      /* even addresses only for word/long */
	pf ("Block filling from $%lX to $%lX with %s data $%lX\n",
		start, end, sizenames [opsize], data);
	frozen = stop_chip ();
	set_fc ();
	address = start;
	PUTMEM (address, data);
	address += 1 << opsize;
	while (address <= end)
	{
		if (check_esc ()) break;
		FILLMEM (address, data);
		address += 1 << opsize;
	}
	if (UseWindows) DISPLAYCODE;
	restore_fc ();
	if (frozen) run_chip (0L);
	return 0;
}

int clear_brkpnts (void)
{
	unsigned Count;
	int retval = 0;
	LONG Address;

	for (Count = 0; Count < BreakpointCount (); Count++)
	{
		if ((Count + 1) == StepBreakpoint) continue;
		Address = getbr (Count);
		retval |= nobr (Count);
		DisplayCode (Address, UPDATEADDRESS);
	}
	return retval;
}

int setbr (char *stuff, char *mask)
{
	LONG address,maskreg;
	int count;

	if (gethex (stuff, &address))
	{
		print ("Can't set breakpoint: ");
		print (stuff);
		print ("\n");
		return -1;
	}
	if (gethex (mask, &maskreg))
	{
		print ("Can't set breakpoint mask: ");
		print (mask);
		print ("\n");
		return -2;
	}
	if (!mask)
	{
		print ("Mask must be non-zero\n");
		return -3;
	}
	for (count = 0; count < BreakpointCount ();count++)
		if (!getmask (count)) break;
	if (count == BreakpointCount ())
	{
		pf ("Can't set any more breakpoints (limit is %d)\n", count);
		return -4;
	}
	if (br (count,address,maskreg) < 0)
		pf ("Breakpoint already set at $%lx\n", address);
	else if (UseWindows) DisplayCode (address, UPDATEADDRESS);
	return 0;
}

int killbr (char *which)
{
	int retval, count;
	LONG address;

	if (gethex (which, &address))
	{
		print ("Can't clear breakpoint: ");
		print (which);
		print ("\n");
		return -1;
	}
	for (count = 0; count < BreakpointCount (); count++)
		if (getbr (count) == address) break;
	if (count == BreakpointCount ())
	{
		print ("No breakpoint set at ");
		ltoh (address);
		print ("\n");
		return -2;
	}
	retval = nobr (count);
	if (UseWindows) DisplayCode (address, UPDATEADDRESS);
	return retval;
}

void dispbr (void)
{
	int count;

	for (count = 0; count != BreakpointCount (); count++)
	if (getmask (count))
	{
		ltoh (getbr (count));
		print ("  ");
	}
	print ("\n");
}


int do_breakpoints (int argc, char **argv)
{
	*last_cmd = '\0';
	if (argc == 2) return setbr (argv [1],breakmask);
	else if (argc == 3) return setbr (argv [1],argv [2]);
	dispbr ();
	return 0;
}

int do_kill (int argc, char **argv)
{
	int count, retval;
	char commandline [MAXCMD];

	*last_cmd = '\0';
	if (argc == 1)
	{
		print ("Kill all breakpoints (Y/N)? ");
		getline (commandline);
		if (toupper (*commandline) == 'Y') retval = clear_brkpnts ();
	}
	else
	for (retval = 0, count = 1; count < argc;)
		retval |= killbr (argv [count++]);
	dispbr ();
	return retval;
}

LONG printreg (int r)
{
	LONG save;

	print (regs [r].RegName);
	print (" ");
	ltoh (save = GETREG (regs[r].RegCode));
	return save;
}

LONG ShowBinary (LONG Value, unsigned BitCount)
{
	LONG Mask = 1L << (BitCount - 1);

	if (!BitCount) return Value;
	while (Mask)
	{
		pc (Mask & Value ? '1' : '0');
		Mask >>= 1;
	}
	return Value;
}

void ScrollDisassemblyWindow (unsigned HowMany)
{
	struct text_info *OldWindow = CurrentWindow;
	int ScrollSave = SetScroll (1), Counter;
	LONG *From, *To;

	if (!HowMany) return;
	From = &DisassemblyAddress [HowMany];
	To = DisassemblyAddress;
	for (Counter = DisassemblyCount - HowMany; Counter--;)
		*To++ = *From++;
	SetWindow (&DisassemblyWindow);
	gotoxy (GetWindowWidth (&DisassemblyWindow) - 1, DisassemblyCount);
	while (HowMany--) pc ('\n');
	SetWindow (OldWindow);
	SetScroll (ScrollSave);
}

unsigned FindDisassemblyLine (LONG Address)
{
	unsigned GotOne = 0, Count;

	for (Count = 0; Count < DisassemblyCount; Count++)
		if (DisassemblyAddress [Count] == Address)
		{
			GotOne = 1;
			break;
		}
	if (GotOne) return Count + 1;
	return 0;
}

#define	BYTESPERINSTRUCTION	3

void DisplayCode (LONG Address, unsigned Mode)
{
	unsigned Blank = 0, TheLine;
	LONG NewStartAddress;

	switch (Mode)
	{
	case	REDISPLAY:
		*DisassemblyAddress
		= DisassemblyAddress [DisassemblyCount - 1]
		= ~0;

	case	OPTIMALREDISPLAY:
		if (Address < *DisassemblyAddress)
		{
			NewStartAddress = Address - DisassemblyCount
				* BYTESPERINSTRUCTION / 3;
			*DisassemblyAddress =
				(NewStartAddress > *DisassemblyAddress)
				? 0 : NewStartAddress;
RedisplayCode:		*DisassemblyAddress &= ~1L;
			DISPLAYCODE;
			break;
		}
		else if (Address > DisassemblyAddress [DisassemblyCount - 1])
		{
			if (Address == DisassemblyEndAddress)
				NewStartAddress = DisassemblyAddress [1];
			else
				NewStartAddress = Address - DisassemblyCount
				* BYTESPERINSTRUCTION * 2 / 3;
			if ((TheLine = FindDisassemblyLine (NewStartAddress)) != 0)
			{
				ScrollDisassemblyWindow (TheLine - 1);
				DisassemblyEndAddress = Disassemble (DisassemblyEndAddress,
					DisassemblyCount - (TheLine - 1), TheLine - 1, 0);
			}
			else
			{
				*DisassemblyAddress = NewStartAddress;
				goto RedisplayCode;
			}
			break;
		}
		else goto UpdateAddress;

	case	BLANKPC:
		Blank = 1;

	case	UPDATEADDRESS:
UpdateAddress:	if ((TheLine = FindDisassemblyLine (Address)) != 0)
			Disassemble (Address, TheLine - 1, 1, Blank);
		break;
	}
}

#pragma argsused
int do_rd (int argc, char **argv)
{
	int count,f_save = stop_chip ();
	int ScrollSave;
	struct text_info *OldWindow = CurrentWindow;
	LONG CurrentPC;

	if (UseWindows)
	{
		ScrollSave = SetScroll (0);
		SetWindow (&RegisterWindow);
		gotoxy (1,1);
	}
	textattr (REGISTERDISPLAYCOLOR);
	print (" D0-7 ");
	textattr (COMMANDOUTPUTCOLOR);
	for (count = 0; count <= 7; count++)
	{
		ltoh (GETREG (count + REG_D0));
		pc (' ');
	}
	if (!UseWindows) clreol ();
	pc ('\n');
	textattr (REGISTERDISPLAYCOLOR);
	print (" A0-7 ");
	textattr (COMMANDOUTPUTCOLOR);
	for (count = 0; count <= 7; count++)
	{
		ltoh (GETREG (count + REG_A0));
		pc (' ');
	}
	textattr (REGISTERDISPLAYCOLOR);
	if (!UseWindows) clreol ();
	print ("\n   PC ");
	textattr (COMMANDOUTPUTCOLOR);
	ltoh (CurrentPC = GETREG (REG_PC));
	textattr (REGISTERDISPLAYCOLOR);
	print ("      USP ");
	textattr (COMMANDOUTPUTCOLOR);
	ltoh (GETREG (REG_USP));
	textattr (REGISTERDISPLAYCOLOR);
	print ("      SFC ");
	textattr (COMMANDOUTPUTCOLOR);
	ltoh (GETREG (REG_SFC));
	textattr (REGISTERDISPLAYCOLOR);
	print ("       SR 10S--210---XNZVC");
	if (!UseWindows) clreol ();
	print ("\n  VBR ");
	textattr (COMMANDOUTPUTCOLOR);
	ltoh (GETREG (REG_VBR));
	textattr (REGISTERDISPLAYCOLOR);
	print ("      SSP ");
	textattr (COMMANDOUTPUTCOLOR);
	ltoh (GETREG (REG_SSP));
	textattr (REGISTERDISPLAYCOLOR);
	print ("      DFC ");
	textattr (COMMANDOUTPUTCOLOR);
	ltoh (GETREG (REG_DFC));
	print ("          ");
	ShowBinary (GETREG (REG_SR), 16);
	if (UseWindows) ThrowAway = 1;
	pc ('\n');
	ThrowAway = 0;
	if (UseWindows)
	{
		SetScroll (ScrollSave);
		SetWindow (OldWindow);
	}
	else
	{
		if (!argv) RepeatChar ('\xcd', 80, COMMANDOUTPUTCOLOR);
		else
		{
			disasm (CurrentPC);
			pc ('\n');
		}
	}
	if (f_save) run_chip (0L);
	return 0;
}

int do_rm (int argc, char **argv)
{
	static int last_reg = 0;
	int frozen,count;
	LONG OldValue, stuff;
	char *last_ptr, tempbuff [MAXCMD], *save,
		lastbuf, commandline [MAXCMD];

	if (argc > 3)
	{
		pf ("usage: rm [register [data]]\n");
		return -99;
	}
	if (argc < 2) count = last_reg;
	else
	{
		strcpy (tempbuff, argv [1]);
		if (strlen (tempbuff) < 3) strcat (tempbuff," ");
		for (count = 0; count < REGCOUNT;count++)
			if (!stricmp (regs [count].RegName, tempbuff)) break;
		if (count == REGCOUNT)
		{
			pf ("Illegal register name: %s\n", argv [1]);
			return -1;
		}
		if (argc == 3)
		{
			if (gethex (argv [2], &stuff))
			{
				report_error (eval_error, argv [2]);
				return -98;
			}
			OldValue = GETREG (regs[count].RegCode);
			PUTREG (regs [count].RegCode, stuff);
			if (UseWindows)
			{
				DISPLAYREGISTERS;
				DisplayCode (OldValue, UPDATEADDRESS);
				DisplayCode (stuff, UPDATEADDRESS);
			}
			return 0;
		}
	}
	frozen = stop_chip ();
	for (lastbuf = '\0';;)
	{
		printreg (count);
		print (" ");
		getline (commandline);
		last_ptr = commandline + strlen (commandline) - 1;
		for (save = (char *) NULL; last_ptr >= commandline; last_ptr--)
		{
			if (isspace (*last_ptr)) continue;
			save = strchr (".=^vV",*last_ptr);
			if (save)
			{
				*last_ptr = '\0';
				lastbuf = *save;
			}
			break;
		}
		if (gethex (commandline, &stuff))
		{
			if (eval_error != NOCHAR_ERROR)
			{
				report_error (eval_error, commandline);
				continue;
			}
		}
		else
		{
			OldValue = GETREG (regs [count].RegCode);
			PUTREG (regs [count].RegCode, stuff);
			if (UseWindows)
			{
				DISPLAYREGISTERS;
				DisplayCode (OldValue, UPDATEADDRESS);
				DisplayCode (stuff, UPDATEADDRESS);
			}
		}
		if (!save) save = &lastbuf;
		if (*save == '.')
		{
			last_reg = count;
			break;
		}
		if (*save == '^')
		{
			if (--count < 0) count = REGCOUNT-1;
		}
		else if (*save == '=') continue;
		else if (++count == REGCOUNT) count = 0;
	}
	if (frozen) run_chip (0L);
	return 0;
}

#pragma argsused
int getfilename (void)
{
	print ("Enter file name: ");
	getline (commandline);
	return strlen (commandline);
}

#define FREAD(x,y) if ((y = fgetc (x)) == EOF) return 0
#define CFREAD(x,y) if ((y = fgetc (x)) == EOF) return 0;\
			icheck = (icheck << 4) + htoc (y)
static BYTE checksum;

/* collect returns the value of the next (howmany) character pairs
 * counter is decremented for each character pair read
 */

long collect (int howmany,int *counter, FILE *infile)
{
	long stuff;
	int icheck,c;

	for (stuff = 0;howmany--;)
	{
		icheck = 0;
		CFREAD (infile,c);
		stuff = (stuff << 4) + htoc (c);
		CFREAD (infile,c);
		stuff = (stuff << 4) + htoc (c);
		(*counter)--;
#ifdef  DEBUG
		pf ("icheck = %X\n", icheck);
#endif
		checksum += icheck;
	}
	return (stuff);
}

int do_srec (srecord *where, FILE *infile)
{
	int ctr,asize,c;
	BYTE *put, s9flag = 0;

	do c = fgetc (infile);
	while (c != 'S' && c != EOF);
	if (c == EOF) return SREC_EOF;
	FREAD (infile,where->rectype);
	switch (where->rectype)
	{
		case	'0':
		asize = 2;
		break;

		case	'1':
		case '2':
		case '3':
		asize = 1 + where->rectype - '0';
		break;

		case	'7':
		case '8':
		case '9':
		asize = 11 - (where->rectype - '0');
		s9flag = 1;
		break;

		default:
		return SREC_FORMAT;
	}
	checksum = 0;
	where->reclen = (short) collect (1,&c, infile);
	where->address = 0;
	for (ctr = asize; ctr; ctr--)
	{
		where->address <<= 8;
		where->address |= collect (1,&where->reclen, infile);
	}
	put = (BYTE *) where->bytes;
	for (ctr = where->reclen-1; ctr; )
		*put++ = collect (1, &ctr, infile);
	collect (1, &where->reclen, infile);
	if (!s9flag && (where->rectype != '0')) lowest = min (where->address, lowest);
	//if (checksum != 0xff) return SREC_CHECKSUM;
	where->reclen += 4;
	return s9flag ? SREC_S9 : 0;
}

BYTE sr_fetch (BYTE **where, BYTE *ctr)
{
	BYTE temp;

	(*ctr)--;
	temp = **where;
	(*where)++;
	return temp;
}

void put_srec (srecord *data, LONG load_offset)
{
	LONG temp, address;
	BYTE reclen, *where;

	if (data->reclen < 5) return;
	reclen = data->reclen - 4;
	address = data->address;
	if (data->rectype >= '7' && data->rectype <= '9')
	{
		execute_address = address;
		return;
	}
	if (data->rectype == '0') return;
	where = (BYTE *) data->bytes;
	if (reclen && (load_offset + address) & 1)
	{
		PUTBYTE (load_offset + address, sr_fetch (&where, &reclen));
		address++;
	}
	else if (reclen >= 2)
	{
		temp = sr_fetch (&where, &reclen);
		PUTWORD (load_offset + address, (WORD) ((temp << 8) + sr_fetch (&where, &reclen)));
		address += 2;
	}
	else if (reclen)
	{
		PUTBYTE (load_offset + address, sr_fetch (&where, &reclen));
		address++;
	}
	while (reclen >= 4)
	{
		temp = sr_fetch (&where, &reclen);
		temp = (temp << 8) + sr_fetch (&where, &reclen);
		temp = (temp << 8) + sr_fetch (&where, &reclen);
		FILLLONG (load_offset + address, (LONG) ((temp << 8) + sr_fetch (&where, &reclen)));
		address += 4;
	}
	if (reclen & 2)
	{
		temp = sr_fetch (&where, &reclen);
		FILLWORD (address, (WORD) ((temp << 8) + sr_fetch (&where, &reclen)));
		address += 2;
	}
	if (reclen)
	{
		FILLBYTE (address, sr_fetch (&where, &reclen));
		address++;
	}
}

void put_srecf (srecord *data, LONG load_offset)
{
	LONG temp, address;
	BYTE reclen, *where;

	if (data->reclen < 5) return;
	reclen = data->reclen - 4;
	address = data->address;
	if (data->rectype >= '7' && data->rectype <= '9')
	{
		execute_address = address;
		return;
	}
	if (data->rectype == '0') return;
	where = (BYTE *) data->bytes;

	while(reclen > 0) {
		temp = sr_fetch (&where, &reclen);
		PUTBYTE (load_offset + 0x555, 0xaa);
		PUTBYTE (load_offset + 0x2aa, 0x55);
		PUTBYTE (load_offset + 0x555, 0xa0);
		PUTBYTE (address, temp);
		address++;
	}
}

int _do_load (LONG, FILE *);

int do_load (int argc, char **argv)
{
	char *filename;
	int error;
	LONG load_offset = 0L;

	filename = argv[1];
	if (argc == 3) if (gethex (argv [2],&load_offset))
	{
		report_error (eval_error, argv [2]);
		return eval_error;
	}
	if (argc < 2)
	{
		if (!getfilename ()) return -1;
		filename = commandline;
	}
	if ((infile = fopen (filename,"r")) == NULL)
	{
		pf ("Cannot open file %s\n",filename);
		return -2;
	}
	error = _do_load (load_offset, infile);
	fclose (infile);
	if (UseWindows) DISPLAYCODE;
	return error;
}

int _do_load (LONG load_offset, FILE *infile)
{
	srecord buff;
	int error, rcount;

	execute_address = 0;
	lowest = 0xffffffffL;
	*last_cmd = '\0';
	stop_chip ();
	set_fc ();
	for (error = rcount = 0; !error; rcount++)
	{
		if (check_esc ()) break;
		error = do_srec (&buff, infile);
		if (!error || error == SREC_S9) put_srec (&buff,load_offset);
		pc ('.');
	}
	pc ('\n');
	switch (error)
	{
		case SREC_EOF:
		pf ("EOF Reached before S9 record on line %d\n", rcount);
		break;

		case SREC_CHECKSUM:
		pf ("Checksum error on line %d\n", rcount);
		break;

		case SREC_S9:
		pf ("Download completed OK - %d records read\n", rcount);
		error = 0;
		break;

		case SREC_FORMAT:
		pf ("Format error on line %d - file probably not S-records\n",
			rcount);
		break;

		default:
		pf ("Internal error - do_srec returned %d on line %d\n",
			error, rcount);
	}
	restore_fc ();
	if (execute_address) PUTREG (REG_PC, execute_address + load_offset);
	lowest += load_offset;
	return error;
}

int _do_loadf (LONG, FILE *);

int do_loadf (int argc, char **argv)
{
	char *filename;
	int error;
	LONG load_offset = 0L;

	filename = argv[1];
	if (argc == 3) if (gethex (argv [2],&load_offset))
	{
		report_error (eval_error, argv [2]);
		return eval_error;
	}
	if (argc < 2)
	{
		if (!getfilename ()) return -1;
		filename = commandline;
	}
	if ((infile = fopen (filename,"r")) == NULL)
	{
		pf ("Cannot open file %s\n",filename);
		return -2;
	}
	error = _do_loadf (load_offset, infile);
	fclose (infile);
	if (UseWindows) DISPLAYCODE;
	return error;
}

int _do_loadf (LONG load_offset, FILE *infile)
{
	srecord buff;
	int error, rcount;

	execute_address = 0;
	lowest = 0xffffffffL;
	*last_cmd = '\0';
	stop_chip ();
	set_fc ();
	for (error = rcount = 0; !error; rcount++)
	{
		if (check_esc ()) break;
		error = do_srec (&buff, infile);
		if (!error || error == SREC_S9) put_srecf (&buff,load_offset);
		pc ('+');
	}
	pc ('\n');
	switch (error)
	{
		case SREC_EOF:
		pf ("EOF Reached before S9 record on line %d\n", rcount);
		break;

		case SREC_CHECKSUM:
		pf ("Checksum error on line %d\n", rcount);
		break;

		case SREC_S9:
		pf ("Download completed OK - %d records read\n", rcount);
		error = 0;
		break;

		case SREC_FORMAT:
		pf ("Format error on line %d - file probably not S-records\n",
			rcount);
		break;

		default:
		pf ("Internal error - do_srec returned %d on line %d\n",
			error, rcount);
	}
	restore_fc ();
	if (execute_address) PUTREG (REG_PC, execute_address + load_offset);
	lowest += load_offset;
	return error;
}

int _do_erasef (LONG, int);

int do_erasef (int argc, char **argv)
{
	LONG sector;
	int error;
	LONG load_offset = 0L;

	if (argc == 3) if (gethex (argv [2],&load_offset))
	{
		report_error (eval_error, argv [2]);
		return eval_error;
	}

	if (argc == 2) if (gethex (argv [2],&sector))
	{
		report_error (eval_error, argv [2]);
		return eval_error;
	}

	error = _do_erasef (load_offset, (int)sector);

	if (UseWindows) DISPLAYCODE;
	return error;
}

int _do_erasef (LONG load_offset, int sector)
{
	int error;

	PUTBYTE (load_offset + 0x555, 0xaa);
	PUTBYTE (load_offset + 0x2aa, 0x55);
	PUTBYTE (load_offset + 0x555, 0x80);
	PUTBYTE (load_offset + 0x555, 0xaa);
	PUTBYTE (load_offset + 0x2aa, 0x55);
	if(sector < 0) {
	  PUTBYTE (load_offset + 0x555, 0x10);
	} else {
	  PUTBYTE (load_offset + ((sector&0xf)<<16), 0x30);
	}
	return error;
}

/* do_macro tries to open macro file;
 * records entry in macro structure if successful
 */
int do_macro (int argc, char **argv)
{
	FILE *save;

	*last_cmd = '\0';
	if (argc < 2)
	{
		pf ("No macro filename given\n");
		return -1;
	}
	save = macro;
	if (!(macro = fopen (argv [1],"r")))
	{
		pf ("Error opening macro file %s\n",argv [1]);
		macro = save;
		return -2;
	}
	if (save)
	{
		pf ("Closing old macro file\n");
		fclose (save);
	}
	return 0;
}

int gs (char *where)
{
	int count;
	char c;

	for (count = c = 0;;)
	{
		while (!key_ready ()) show_stat (0);
		switch (c = gc (1))
		{
			case    '\b':
			if (count)
			{
				count--;
				where--;
				print ("\b \b");
			}
			break;

			case    '\r':
			pc ('\n');
			*where = '\0';
			return count;

			default:
			if (isprint (c))
			{
				pc (*where++ = c);
				count++;
			}
		}
	}
}

char *getline (char *where)
{
	char *ptr;

	if (macro)
	{
		*where = '\0';
		if (fgets (where,MAXCMD,macro))
		{
			pf (where);
			if ((ptr = strrchr (where,'\n')) != (char *) 0) *ptr = '\0';
			skipwhite (&where);
			return where;
		}
		closemacro ();
	}
	gs (where);
	skipwhite (&where);
	return where;
}

int do_dos (int argc, char **argv)
{
	int retval,portinstalled,portsave,baudsave;
	char cmd [MAXCMD], PortName [MAXCMD];
	struct ScreenInfo *BD32Screen = SaveScreen ();
	char *CmdProcessor = getenv ("COMSPEC");

	if (!CmdProcessor) CmdProcessor = "command";
	*last_cmd = '\0';
	*cmd = '\0';
	RestoreScreen (screensave);
	portinstalled = !GetPortInfo (PortName, &portsave, &baudsave);
	deinit_serial ();
	if (argc == 1)
	{
		print ("Type 'exit' to return to BD32\n");
		retval = system (CmdProcessor);
	}
	else
	{
		CollectArguments (cmd, argc - 1, &argv [1]);
		retval = system (cmd);
	}
	screensave = SaveScreen ();
	textattr (LIGHTGRAY);
	gotoxy (1, screensave->TextInfo.screenheight);
	clreol ();
	if (retval) pf ("Command returned %d; ",retval);
	print ("Press any key to return to BD32:");
	gc (0);
	RestoreScreen (BD32Screen);
	if (portinstalled) init_serial (PortName, portsave, baudsave);
	show_stat (1);
	return retval;
}

int do_eval (int argc, char **argv)
{
	char buff [MAXCMD], *ptr;
	int i;
	LONG answer,mask;
	extern eval_error;
	extern char *showmin_1 ();

	if (argc < 2)
	{
		pf ("Usage: eval <expression>\n");
		return -1;
	}
	strcpy (buff,argv [1]);
	for (i = 2; i < argc; i++)
	{
		strcat (buff, " ");
		strcat (buff, argv [i]);
	}
	answer = eval (buff);
	if (eval_error)
	{
		report_error (eval_error, buff);
		return eval_error;
	}
	for (mask = 0x80000000L, ptr = buff; mask; mask >>= 1)
		*ptr++= (mask & answer) ? '1' : '0';
	*ptr = '\0';
	pf ("%s %ld %s\n",showmin_1 (answer),answer,buff);
	return 0;
}

extern struct symtab *symadd (char *, LONG);
extern int issymbol (char **, char *);

int do_set (int argc, char **argv)
{
	LONG value;
	char buff [MAXCMD];

	*last_cmd = '\0';
	if (argc != 3)
	{
		print ("Usage: set symbol value\n");
		return -1;
	}
	if (gethex (argv [2],&value))
	{
		report_error (eval_error, argv [2]);
		return eval_error;
	}
	if (!issymbol (&argv [1],buff))
	{
		pf ("Not a valid symbol: %s\n", argv [1]);
		return -2;
	}
	if (!symadd (buff,value))
	{
		pf ("No memory for new symbol: %s\n", buff);
		return -3;
	}
	DisplayCode (value, UPDATEADDRESS);
	return 0;
}

int do_fc (int argc, char **argv)
{
	LONG value;

	*last_cmd = '\0';
	if (argc == 1)
	{
		pf ("Function code for debugger memory access is %d\n", fc);
		return 0;
	}
	if (argc != 2)
	{
		print ("Usage: fc [value] (value in range 0-7)\n");
		return -1;
	}
	if (gethex (argv [1],&value))
	{
		report_error (eval_error, argv [2]);
		return eval_error;
	}
	if (value > 7)
	{
		print ("Usage: fc [value] (value in range 0-7)\n");
		return -2;
	}
	fc = (short) value;
	return 0;
}

/* setext checks the extension of the string given and makes it
   the default given
   an extension is 0-3 alnums or '_' at end of string, preceeded by '.'
   default extension must include period when this function called
 */

char *findext ();
void setext (s,e)
char *s, *e;

{
	char *p;

	if (!(p = findext (s))) strcat (s, e);
	else strcpy (p, e);
}

 /* findext returns pointer to start of extension ('.')
    or NULL if extension not found
 */

char *findext (s)
char *s;

{
	char *end;
	int i;

	end = s + strlen (s);
	for (i = 1; i <= 3; i++)
	{
		if (*--end == '.') return end;
		if (isalnum (*end) || *end == '_') continue;
		return NULL;	/* non-alnum ended scan */
	}
	if (*--end == '.') return end;
	return NULL;
}

int DisplayDriverHelp (char *Path, int DisplayHeader)
{
	int ok, PrintedHeader = 0, done;
	struct ffblk dirblock;
	char *ext, namebuff [MAXCMD], workbuff [MAXCMD];
	FILE *msgfile;

	if (DisplayHeader < 0) return DisplayHeader;
	strcpy (namebuff, Path);
	if ((ext = strrchr (namebuff, '\\')) != (char *) 0) strcpy (ext + 1, "*");
	else
	{
		if (!DisplayHeader) return 0;
		strcpy (namebuff, "*");
	}
	strcat (namebuff, PROG_EXT);
	done = findfirst (namebuff, &dirblock, 0);
	if (!done && DisplayHeader)
	{
		pf ("************************ Target-Resident Commands ************************\n");
		PrintedHeader = 1;
		if (pause_check ()) return -1;
	}
	for (; !done; done = findnext(&dirblock))
	{
		ok = 1;
		strcpy (namebuff, dirblock.ff_name);
		setext (namebuff, MSG_EXT);
		if ((msgfile = fopen (namebuff, "r")) != (FILE *) 0)
		{
			if (fgets (workbuff, MAXCMD, msgfile) == workbuff)
				pf (workbuff);
			else ok = 0;
			fclose (msgfile);
		}
		else ok = 0;
		if (!ok)
		{
			if ((ext = findext (namebuff)) != (char *) 0) *ext = '\0';
			pf ("%s\t                         {description not available}\n", namebuff);
		}
		if (pause_check ()) return -1;
	}
	return !PrintedHeader;
}

#pragma argsused
int do_help (int argc, char **argv)
{
	int counter;

	SelectLargeCommandLineWindow ();
	pauselinecount = 0;
	for (counter = 0; commands [counter].name; counter++)
	{
		pf ("%s\t%s\n",commands [counter].name,commands [counter].description);
		if (pause_check ()) return 0;
	}
	DisplayDriverHelp (ExecPath, DisplayDriverHelp ("", 1));
	return 0;
}

int do_pause (int argc, char **argv)
{
	if (argc == 1)
	{
		pf ("Pause %s\n", pause_flag ? "enabled" : "disabled");
		return 0;
	}
	if (argc != 2)
	{
		pf ("Usage: 'Pause on' | 'Pause off'\n");
		return 1;
	}
	if (!stricmp (argv [1], "ON"))
	{
		pause_flag = 1;
		pf ("Pause enabled\n");
		return 0;
	}
	if (!stricmp (argv [1], "OFF"))
	{
		pause_flag = 0;
		pf ("Pause disabled\n");
		return 0;
	}
	pf ("Pause: illegal parameter '%s'\n\
parameter must be either 'on' or 'off'\n", argv [1]);
	return 1;
}

#pragma argsused
int do_comment (int argc, char **argv)
{
	*last_cmd = '\0';
	return 0;
}

struct cmd_table commands [] = {

{ "*",     "<comment text>           'Do' File Comment", do_comment},
{ "ASM",   "[<address>]              Line-by-line assembler/disassembler", do_asm},
{ "BF",	 "start end data           Block Fill Memory with Data", do_bf},
{ "BR",    "<address> [<mask>]]      Set/Display Breakpoint", do_breakpoints},
{ "CLOAD", "<Name> [<Parms>]         Load Target Resident Command", do_cload},
{ "CLS",   "                         Clear Screen", do_cls},
{ "DASM",  "[<address> [<count>]]    Display Disassembly (ESC halts)", do_disasm},
{ "DO",    "<filename>               Execute commands from macro file", do_macro},
{ "DOS",   "[<command line>]         Execute DOS from within BD32", do_dos},
{ "DRIVER","[<expression>]           Set load address for Target Drivers", do_driver },
{ "EF",    "[<sector>] [<offset>]    Erase Flash", do_erasef},
{ "EVAL",  "<expression>             Evaluate Arithmetic Expression", do_eval},
{ "EXIT",  "                         End BD32, return to DOS", quit},
{ "FC",    "[<expression>]           Set FC0-2 for Debugger Memory Access", do_fc},
{ "GO",    "[<address>]              Start Target MCU", do_go},
{ "HELP",  "                         Display This Command Summary", do_help},
{ "LO",    "<filename> [<offset>]    Load S-record file from disk", do_load},
{ "LF",    "<filename> [<offset>]    Load S-record file from disk to Flash", do_loadf},
{ "LOG",   "[OFF | <filename>]       Log BD32 output to disk file", do_log},
{ "MD",    "[<address> [<count>]]    Memory Display (;b, ;w, ;l, ;di) (ESC halts)", do_dump},
{ "MM",    "[<address>]              Memory Modify  (;b, ;w, ;l, ;di)", do_memory},
{ "NOBR",  "[<address>]              Remove Breakpoint", do_kill},
{ "PAUSE", "[ON | OFF]               Enable/Disable pause in screen display", do_pause},
{ "PORT",  "[<port> <baudrate>]      Select I/O Port (LPT1-3/ICD1-3)/COM1-2", do_port},
{ "QUIT",  "                         End BD32, return to DOS", quit},
{ "RD",    "                         Register Display", do_rd},
{ "RESET", "                         Reset Target MCU", do_reset},
{ "RESTART", "                 Trace Program from Reset", do_restart},
{ "RM",    "[<register name> [data]] Register Modify", do_rm},
{ "S",     "[expr]                   Step Over Subroutines till expr true", do_step},
{ "SD",    "[<start> [<end>]]        Display Symbols between 'Start' and 'End'", do_sd},
{ "SET",   "<symbol> <expression>    Assign Expression Value to Symbol", do_set},
{ "STOP",  "                         Stop Target MCU", do_stop},
{ "SYSCALL", "[ON | OFF]       Enable/Disable Systems calls in Trace", do_syscall},
{ "T",     "[expr]                   Trace Into Subroutines till expr true", do_trace},
{ "WINDOW","[ON | OFF]               Enable/Disable Windowed User Interface", do_window},
{(char *) NULL,(char *) NULL, NULL }
};
