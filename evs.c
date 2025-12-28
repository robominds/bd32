/* evs.c - routines to talk to 68332 DI board
 * this version uses interrupt driven communications
 * in sio.c for DI Version 1.13,
 */

#include	<stdio.h>
#include	<ctype.h>
#include	<conio.h>
#include	<string.h>
#include        "didefs.h"
#include	"sio.h"
#include	"trgtstat.h"
#include	"trgtinfo.h"
#include	"regnames.h"

#include	"scrninfo.h"
#include	"prog-def.h"
#include	"bd32new.p"
#include	"target.p"

#define	RUNMODECHAR	'#'
#define	STOPMODECHAR	'%'

/* #define	USE_EOL		/* un-comment to use get_eol () */

static ChipMode, echo, SerialPort;
static char scratch [40];
extern void DriverError (char *Message);

static unsigned EVS_ValidPorts (void);
static int EVS_Init (unsigned Port, unsigned Speed);
static void EVS_DeInit (void);
static unsigned EVS_GetStatus (void);
static int EVS_BR (int which, LONG where, LONG mask);
static int EVS_NoBR (int which);
static LONG EVS_GetBR (int which);
static LONG EVS_GetMask (int which);
static void EVS_HitBreakpoint (LONG where);
static int EVS_StopChip (void);
static void EVS_StepChip (void);
static void EVS_ResetChip (void);
static void EVS_RestartChip (void);
static void EVS_RunChip (LONG where);
static LONG EVS_GetByte (LONG Where);
static LONG EVS_GetWord (LONG Where);
static LONG EVS_GetLong (LONG Where);
static LONG EVS_DumpByte (LONG Where);
static LONG EVS_DumpWord (LONG Where);
static LONG EVS_DumpLong (LONG Where);
static void EVS_PutByte (LONG Where, BYTE Data);
static void EVS_PutWord (LONG Where, WORD Data);
static void EVS_PutLong (LONG Where, LONG Data);
static void EVS_FillByte (LONG Where, BYTE Data);
static void EVS_FillWord (LONG Where, WORD Data);
static void EVS_FillLong (LONG Where, LONG Data);
static LONG EVS_GetReg (unsigned which);
static void EVS_PutReg (unsigned which, LONG Data);

struct TargetInfo EVSInfo =
{
	"COM",
	0,
	0,
	0,
	4,
	TARGETRESET | TARGETHALT | TARGETSTOPPED,
	EVS_ValidPorts,
	EVS_Init,
	EVS_DeInit,
	EVS_GetStatus,
	EVS_BR,
	EVS_NoBR,
	EVS_GetBR,
	EVS_GetMask,
	EVS_HitBreakpoint,
	EVS_StopChip,
	EVS_StepChip,
	EVS_ResetChip,
	EVS_RestartChip,
	EVS_RunChip,
	EVS_GetByte,
	EVS_GetWord,
	EVS_GetLong,
	EVS_DumpByte,
	EVS_DumpWord,
	EVS_DumpLong,
	EVS_PutByte,
	EVS_PutWord,
	EVS_PutLong,
	EVS_FillByte,
	EVS_FillWord,
	EVS_FillLong,
	EVS_GetReg,
	EVS_PutReg
};

static unsigned EVS_ValidPorts (void)
{
	return ComPortsInstalled ();
}

/* EVS_DeInit Deinitializes serial I/O to DI board */

static void EVS_DeInit (void)
{
	if (EVSInfo.DriverInstalled) ComClose (SerialPort);
	EVSInfo.DriverInstalled = 0;
}

/* EVS_Init initializes serial port to proper parameters */

void di_command (char *);

static int EVS_Init (unsigned port, unsigned baud)
{
	unsigned GotOne = 0, SpeedTest;

	if (!(ComPortsInstalled () & (1 << (port - 1)))) return -1;
	for (SpeedTest = 9600; SpeedTest >= 150; SpeedTest >>= 1)
		if (baud == SpeedTest)
		{
			GotOne = 1;
			break;
		}
	if (!GotOne) return -1;
	EVS_DeInit ();
	if (ComOpen (port, baud, NO_PARITY, EIGHT_DATA, ONE_STOP))
		return -1;
	echo = 0;
	EVSInfo.CurrentPort = port - 1;
	EVSInfo.CurrentSpeed = baud;
	EVSInfo.DriverInstalled = 1;
	di_command ("EC X\r");
/*   	di_command ("EC\r"); */
/*   	echo = 1; */
	return 0;
}

/* get_sio returns next char available from serial port
   handles timeout with longjmp to cmd_jmp */

static char get_sio (void)
{
	unsigned timer;
	int c;

	for (timer = 0xffff; timer; timer--)
		if (ComCharReady (SerialPort) > 0)
		{
			c = ComRead (SerialPort);
			if (c == RUNMODECHAR || c == STOPMODECHAR)
				ChipMode = c;
			return c;
		}
	DriverError ("Serial Port Timeout - Check Cable Connections");
	return 0;
}

/* write_serial sends string to serial port */
static void write_serial (char *c)
{
	while (*c)
	{
		ComWrite (SerialPort, *c++);
		if (echo) get_sio ();
	}
}

static unsigned EVS_GetStatus (void)
{
	int c;
	unsigned sigstat;

	write_serial ("STAT\r");
#ifdef	USE_EOL
	get_eol ();
#endif
	for (sigstat = c = 0; c != RUNMODECHAR && c != STOPMODECHAR;)
	{
		c = get_sio ();
		if (isdigit (c)) sigstat = (sigstat << 1) | (c - '0');
	}
	return sigstat;
}

static void get_prompt (void)
{
	int c;

	do c = get_sio ();
	while (c != RUNMODECHAR && c != STOPMODECHAR);
}

#ifdef	USE_EOL
static void get_eol (void)
{
	int c;

	do c = get_sio ();
	while (c != '\n');
}
#endif

static void di_command (char *string)
{
	write_serial (string);
	get_prompt ();
}

static LONG di_rd (char *string)
{
	int c;
	LONG result;
	char *cptr,capture [100];

	write_serial (string);
#ifdef	USE_EOL
	get_eol ();
#endif
	cptr = capture;
	while (!isxdigit (*cptr++ = c = get_sio ())) ;
	for (result = 0; isxdigit (c); *cptr++ = c = get_sio ())
	{
		result <<= 4;
		result += htoc (c);
	}
	get_prompt ();
	*cptr = '\0';
	return result;
}

static void di_wr (char *string)
{
	di_command (string);
}

static int EVS_BR (int which, LONG where, LONG mask)
{
	int frozen;

	where &= ~ (LONG) 1;
	frozen = EVS_StopChip ();
	sprintf (scratch,"BR %d %lX %lX\r",which,where,mask);
	di_command (scratch);
	if (frozen) run_chip (0L);
	return 0;
}

static int EVS_NoBR (int which)
{
	int frozen = EVS_StopChip ();

	br (which,0L,0L);
	sprintf (scratch,"NOBR %d\r",which);
	di_command (scratch);
	if (frozen) run_chip (0L);
	return 0;
}

static LONG EVS_GetBR (int which)
{
	sprintf (scratch,"RC %X\r",which << 2);
	return di_rd (scratch);
}

static LONG EVS_GetMask (int which)
{
	sprintf (scratch,"RC %X\r",(which << 2) + 0x10);
	return di_rd (scratch);
}

#pragma argsused
static void EVS_HitBreakpoint (LONG where)
{}

static int EVS_StopChip (void)
{
	if (ChipMode == STOPMODECHAR) return 0;
	di_command ("AB\r");
	return 1;
}

static void EVS_StepChip (void)
{
	EVS_StopChip ();
	di_command ("STE\r");
}

static void EVS_ResetChip (void)
{
	EVS_StopChip ();
	di_command ("RE\r");
}

static void EVS_RestartChip (void)
{
	EVS_ResetChip ();
	EVS_StopChip ();
}

static void EVS_RunChip (LONG where)
{
	EVS_StopChip ();
	if (where) sprintf (scratch,"GO %lX\r",where);
	else strcpy (scratch, "GO\r");
	di_command (scratch);
}

LONG EVS_GetByte (LONG x)
{
	int frozen,save;
	LONG result;

	frozen = EVS_StopChip ();
	save = set_fc ();
	sprintf (scratch, "MR B %lX\r",x);
	result =  di_rd (scratch);
	if (save) restore_fc ();
	if (frozen) run_chip (0L);
	return result;
}

LONG EVS_DumpByte (LONG x)
{
	return EVS_GetByte (x);
}

void EVS_PutByte (LONG x, BYTE y)
{
	int frozen,save;

	frozen = EVS_StopChip ();
	save = set_fc ();
	sprintf (scratch, "MW B %lX %X\r",x,y);
	di_wr (scratch);
	if (save) restore_fc ();
	if (frozen) run_chip (0L);
}

void EVS_FillByte (LONG x, BYTE y)
{
	EVS_PutByte (x, y);
}

LONG EVS_GetWord (LONG x)
{
	int frozen,save;
	LONG result;

	frozen = EVS_StopChip ();
	save = set_fc ();
	sprintf (scratch, "MR W %lX\r",x);
	result = di_rd (scratch);
	if (save) restore_fc ();
	if (frozen) run_chip (0L);
	return result;
}

LONG EVS_DumpWord (LONG x)
{
	return EVS_GetWord (x);
}

void EVS_PutWord (LONG x, WORD y)
{
	int frozen,save;

	frozen = EVS_StopChip ();
	save = set_fc ();
	sprintf (scratch, "MW W %lX %X\r",x,y);
	di_wr (scratch);
	if (save) restore_fc ();
	if (frozen) run_chip (0L);
}

void EVS_FillWord (LONG x, WORD y)
{
	EVS_PutWord (x, y);
}

LONG EVS_GetLong (LONG x)
{
	int frozen,save;
	LONG result;

	frozen = EVS_StopChip ();
	save = set_fc ();
	sprintf (scratch, "MR L %lX\r",x);
	result = di_rd (scratch);
	if (save) restore_fc ();
	if (frozen) run_chip (0L);
	return result;
}

LONG EVS_DumpLong (LONG x)
{
	return EVS_GetLong (x);
}

void EVS_PutLong (LONG x, LONG y)
{
	int frozen,save;

	frozen = EVS_StopChip ();
	save = set_fc ();
	sprintf (scratch, "MW L %lX %lX\r",x,y);
	di_wr (scratch);
	if (save) restore_fc ();
	if (frozen) run_chip (0L);
}

void EVS_FillLong (LONG x, LONG y)
{
	EVS_PutLong (x, y);
}

static char *RegNames [] =
{
	"S0", "SC", "SD", "SA", "SB", "SE", "SF", "S9",
	"D0" "D1", "D2", "D3", "D4", "D5", "D6", "D7",
	"A0" "A1", "A2", "A3", "A4", "A5", "A6", "A7"
};


static LONG EVS_GetReg (unsigned which)
{
	int frozen;
	LONG result;

	if (which > REG_MAX)
		DriverError ("Internal Error - Illegal Register Accessed");
	frozen = EVS_StopChip ();
	sprintf (scratch, "RD %s\r", RegNames [which]);
	result = di_rd (scratch);
	if (frozen) run_chip (0L);
	return result;
}

static void EVS_PutReg (unsigned which, LONG data)
{
	int frozen;

	if (which > REG_MAX)
		DriverError ("Internal Error - Illegal Register Accessed");
	frozen = EVS_StopChip ();
	sprintf (scratch, "WR %s %lX\r", RegNames [which], data);
	di_wr (scratch);
	if (frozen) run_chip (0L);
}
