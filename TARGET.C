/* target.c - routines to talk to target system
 * this file provides high-level functions which
 * interface to SIO.C and LPT.C
 */

#include	<setjmp.h>
#include	<stdio.h>
#include	<conio.h>
#include	<string.h>
#include	"trgtinfo.h"
#include	"regnames.h"
#include	"target.h"
#include	"trgtstat.h"

#include	<conio.h>
#include	"scrninfo.h"
#include	"prog-def.h"
#include	"bd32new.p"
#include	"target.p"

#define         MAX_ACCESSED    16
#define         DEF_FC          5       /* default function code to use */
#define		TARGETDRIVERNOTINSTALLED	DriverError (NotInstalledMessage)

static char NotInstalledMessage []
  = "Target port not configured - Use 'Port <port> <speed>' command";
extern jmp_buf cmd_jmp;
extern (*abort_func) ();
extern FILE *infile;
extern struct TargetInfo EVSInfo, LPTInfo, ICDInfo;
extern struct text_info CommandLineWindow;

static int current = 0;
int cache_active = 0;
static struct mem_cache {
	LONG address;
	BYTE data;
	char used;
	} accessed [MAX_ACCESSED] =
{
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},
{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}
};

int fc = DEF_FC, fc_set = 0;
unsigned LastTargetStatus = 0;

static old_sfc, old_dfc;

static struct TargetInfo NullTarget = {"???", 0, 0, 0, 0, 0},
			 *Targets [] = {&EVSInfo, &LPTInfo, &ICDInfo, 0};
static struct TargetInfo *CurrentTarget = &NullTarget;

void show_settings (void)
{
	if (!CurrentTarget->DriverInstalled)
		pf ("*** %s ***\n", NotInstalledMessage);
	else pf ("Device %s%d at speed %u\n",
		CurrentTarget->DeviceName,
		CurrentTarget->CurrentPort,
		CurrentTarget->CurrentSpeed);
}

void DriverError (char *Message)
{
	SetWindow (&CommandLineWindow);
	pf ("\n*** Fault on Port %s%d ***\n*** %s ***\n",
		CurrentTarget->DeviceName,
		CurrentTarget->CurrentPort,
		Message);
	if (infile) fclose (infile);
	infile = NULL;
	longjmp (cmd_jmp,1);
}

/* deinit_serial is called before program quits or goes to DOS
 * un-do any bad things which have been done to talk to target
 */

void deinit_serial (void)
{
	if (CurrentTarget->DriverInstalled) (*CurrentTarget->DeInit) ();
	CurrentTarget = &NullTarget;
}

/* init_serial initializes serial port to proper parameters */

int init_serial (char *name, int port, int speed)
{
	struct TargetInfo **NextTarget;
	int retval;

	for (NextTarget = &Targets [0]; *NextTarget; NextTarget++)
		if (!stricmp ((*NextTarget)->DeviceName, name))
		{
			if (!((*(*NextTarget)->ValidPorts) () & (1 << (port - 1))))
				return -2;
			deinit_serial ();
			if (!(retval = (*(*NextTarget)->Init) (port, speed)))
				CurrentTarget = *NextTarget;
			return retval;
		}
	return -3;
}

int GetPortInfo (char *name, int *port, int *speed)
{
	if (!CurrentTarget->DriverInstalled) return -1;
	strcpy (name, CurrentTarget->DeviceName);
	*port = CurrentTarget->CurrentPort;
	*speed = CurrentTarget->CurrentSpeed;
	return 0;
}

int BreakpointCount (void)
{
	return CurrentTarget->MaxBreakpoints;
}

int match_bp (LONG where)
{
	int i, maxbr = BreakpointCount ();

	for (i = 0; i < maxbr; i++)
		if (getmask (i) && (getbr (i) == where))
			return i + 1;
	return 0;
}

static int handle_stop (void)
{
	int retval = 0;
	LONG where = GETREG (REG_PC);

	if (match_bp (where - 2L))
	{
		retval = 1;
		(*CurrentTarget->HitBreakpoint) (where);
	}
	return retval;
}

unsigned get_sigstatus (void)
{
	unsigned retval;

	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	retval = (*CurrentTarget->GetStatus) ();
	if ((retval & TARGETSTOPPED) && !(LastTargetStatus & TARGETSTOPPED))
		if (handle_stop ()) retval |= TARGETBRKPT;
	LastTargetStatus = retval;
	return retval;
}

unsigned get_statmask (void)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	return CurrentTarget->StatMask;
}

int set_fc (void)
{
	if (fc_set) return 0;
	old_sfc = (int) GETREG (REG_SFC);
	old_dfc = (int) GETREG (REG_DFC);
	PUTREG (REG_SFC, fc);
	PUTREG (REG_DFC, fc);
	return fc_set = 1;
}

int restore_fc (void)
{
	if (!fc_set) return 0;
	PUTREG (REG_SFC, old_sfc);
	PUTREG (REG_DFC, old_dfc);
	fc_set = 0;
	return 1;
}

int br (int which, LONG where, LONG mask)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	if (match_bp (where)) return -1;
	return (*CurrentTarget->BR) (which, where, mask);
}

int nobr (int which)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	return (*CurrentTarget->NoBR) (which);
}

LONG getbr (int which)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	return (*CurrentTarget->GetBR) (which);
}

LONG getmask (int which)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	return (*CurrentTarget->GetMask) (which);
}

int stop_chip (void)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;

	return (*CurrentTarget->StopChip) ();
}

void step_chip (void)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	(*CurrentTarget->StepChip) ();
}

void reset_chip (void)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	(*CurrentTarget->ResetChip) ();
}

void restart_chip (void)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	(*CurrentTarget->RestartChip) ();
}

void run_chip (LONG where)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	(*CurrentTarget->RunChip) (where);
	LastTargetStatus &= ~TARGETSTOPPED;
}

void trash_cache (void)
{
	int c;

    cache_active = 0;
	for (c = 0; c < MAX_ACCESSED; c++)
	accessed[c].used = 0;
}

void enable_cache (void)
{
    cache_active = 1;
}

static int check_accessed (LONG where)
{
	int c;

    if (!cache_active) return 0;
	for (c = 0; c < MAX_ACCESSED; c++)
		if (accessed[c].used && (accessed[c].address == where)) return c+1;
	return 0;
}

static void put_stuff (LONG where, int stuff)
{
	int c;

    if (!cache_active) return;
	if (!(c = check_accessed (where)))
	{
		c = current;
		current = (current + 1) % MAX_ACCESSED;
	}
	else c--;
	accessed[c].data = stuff;
	accessed[c].address = where;
	accessed[c].used = 1;
}

LONG GETBYTE (LONG x)
{
	int c;
	LONG result;

	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	if ((c = check_accessed (x)) != 0)
		return accessed[c-1].data;
	result = (*CurrentTarget->GetByte) (x);
	put_stuff (x, (short) result);
	return result;
}

void PUTBYTE (LONG x, LONG y)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	if (check_accessed (x)) put_stuff (x,(short) y);
	(*CurrentTarget->PutByte) (x, y);
}

void FILLBYTE (LONG x, LONG y)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	if (check_accessed (x)) put_stuff (x,(short) y);
	(*CurrentTarget->FillByte) (x, y);
}

LONG DUMPBYTE (LONG x)
{
	int c;
	LONG result;

	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	if ((c = check_accessed (x)) != 0)
		return accessed[c-1].data;
	result = (*CurrentTarget->DumpByte) (x);
	put_stuff (x, (short) result);
	return result;
}

LONG GETWORD (LONG x)
{
	int c,c1;
	LONG result;

	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	if (((c = check_accessed (x)) != 0) && ((c1 = check_accessed (x+1)) != 0))
		return ((LONG) accessed[c-1].data << 8) + accessed[c1-1].data;
	result = (*CurrentTarget->GetWord) (x);
	put_stuff (x,(int) result >> 8);
	put_stuff (x+1,(int) result);
	return result;
}

void PUTWORD (LONG x, LONG y)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	if (check_accessed (x)) put_stuff (x, (short) (y >> 8));
	if (check_accessed (x + 1)) put_stuff (x+1,(short) y);
	(*CurrentTarget->PutWord) (x, (WORD) y);
}

void FILLWORD (LONG x, LONG y)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	if (check_accessed (x)) put_stuff (x,(short) (y >> 8));
	if (check_accessed (x + 1)) put_stuff (x+1,(short) y);
	(*CurrentTarget->FillWord) (x, (short) y);
}

LONG GETLONG (LONG x)
{
	int c,c1,c2,c3;
	LONG result;

	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	if (((c = check_accessed (x)) != 0) && ((c1 = check_accessed (x+1)) != 0)
	 && ((c2 = check_accessed (x+2)) != 0) && ((c3 = check_accessed (x+3)) != 0))
	return
	((LONG) accessed[c-1].data << 24) + ((LONG) accessed[c1-1].data << 16)
	+ ((LONG) accessed[c2-1].data << 8) + accessed[c3-1].data;
	result = (*CurrentTarget->GetLong) (x);
	put_stuff (x,(int) (result >> 24));
	put_stuff (x+1,(int) (result >> 16));
	put_stuff (x+2,(int) (result >> 8));
	put_stuff (x+3,(int) result);
	return result;
}

void PUTLONG (LONG x, LONG y)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	put_stuff (x,(short) (y >> 24));
	put_stuff (x+1,(short) (y >> 16));
	put_stuff (x+2,(short) (y >> 8));
	put_stuff (x+3,(short) y);
	(*CurrentTarget->PutLong) (x, y);
}

void FILLLONG (LONG x, LONG y)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	put_stuff (x,(short) (y >> 24));
	put_stuff (x+1,(short) (y >> 16));
	put_stuff (x+2,(short) (y >> 8));
	put_stuff (x+3,(short) y);
	(*CurrentTarget->FillLong) (x, y);
}

LONG GETREG (unsigned regname)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	return (*CurrentTarget->GetReg) (regname);
}

void PUTREG (unsigned regname, LONG data)
{
	if (!CurrentTarget->DriverInstalled) TARGETDRIVERNOTINSTALLED;
	(*CurrentTarget->PutReg) (regname, data);
}
