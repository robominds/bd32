/* icd.c - routines to talk to CPU32 target system
 * using P&E's M68ICD board
 */

#include	<stdio.h>
#include	<stdarg.h>
#include    "didefs.h"
#include	"trgtstat.h"
#include	"trgtinfo.h"
#include	"regnames.h"
#include	"bdm-cmds.h"

#define         BGND    0x4afa          /* BGND opcode */

#define		BIOSAddress	((unsigned far *) 0x408)
#define		MAXLPT		3
#define		MAX_Breakpoint	4

#define	bdm_ctl         0               /* offset of control port from base    */
#define	dsi             1               /* data shift input - PC->MCU          */
#define	dsclk           2               /* data shift clock/breakpoint pin     */
#define	step_out        4               /* set low to force breakpoint	       */
#define	rst_out         8               /* set low to force reset on MCU       */
#define	oe		0x10		/* set to a 1 to enable DSI 	       */
#define	force_berr	0x40		/* set to a 1 to force BERR on target  */

#define	bdm_stat        1               /* offset of status port from base     */
#define	freeze		0x40		/* FREEZE asserted when MCU stopped    */
#define	dso             0x80            /* data shift output - MCU->PC         */

#define	waitcnt         0xffff          /* no of loops to wait for response    */
#define	go_cmd          0xc00           /* GO command                          */
#define	nop_cmd         0               /* null (NOP) command                  */
#define	CommandBitCount	17		/* number of bits in BDM command */

/* #define	C_CLOCK_ROUTINE 	/* uncomment to use C version of bdk_clk  */

#define	BDM_FREEZE	1		/* return status - freeze asserted     */

extern void DriverError (char *Message);

static unsigned ICD_ValidPorts (void);
static int ICD_Init (unsigned Port, unsigned Speed);
static void ICD_DeInit (void);
static unsigned ICD_GetStatus (void);
static int ICD_BR (int which, LONG where, LONG mask);
static int ICD_NoBR (int which);
static LONG ICD_GetBR (int which);
static LONG ICD_GetMask (int which);
static void ICD_HitBreakpoint (LONG where);
static int ICD_StopChip (void);
static void ICD_StepChip (void);
static void ICD_ResetChip (void);
static void ICD_RestartChip (void);
static void ICD_RunChip (LONG where);
static LONG ICD_GetByte (LONG Where);
static LONG ICD_GetWord (LONG Where);
static LONG ICD_GetLong (LONG Where);
static LONG ICD_DumpByte (LONG Where);
static LONG ICD_DumpWord (LONG Where);
static LONG ICD_DumpLong (LONG Where);
static void ICD_PutByte (LONG Where, BYTE Data);
static void ICD_PutWord (LONG Where, WORD Data);
static void ICD_PutLong (LONG Where, LONG Data);
static void ICD_FillByte (LONG Where, BYTE Data);
static void ICD_FillWord (LONG Where, WORD Data);
static void ICD_FillLong (LONG Where, LONG Data);
static LONG ICD_GetReg (unsigned which);
static void ICD_PutReg (unsigned which, LONG Data);
static void delay (unsigned count);

struct TargetInfo ICDInfo =
{
	"ICD",
	0,
	0,
	0,
	MAX_Breakpoint,
	TARGETSTOPPED,
	ICD_ValidPorts,
	ICD_Init,
	ICD_DeInit,
	ICD_GetStatus,
	ICD_BR,
	ICD_NoBR,
	ICD_GetBR,
	ICD_GetMask,
	ICD_HitBreakpoint,
	ICD_StopChip,
	ICD_StepChip,
	ICD_ResetChip,
	ICD_RestartChip,
	ICD_RunChip,
	ICD_GetByte,
	ICD_GetWord,
	ICD_GetLong,
	ICD_DumpByte,
	ICD_DumpWord,
	ICD_DumpLong,
	ICD_PutByte,
	ICD_PutWord,
	ICD_PutLong,
	ICD_FillByte,
	ICD_FillWord,
	ICD_FillLong,
	ICD_GetReg,
	ICD_PutReg
};

static char scratch [40];
static LONG last_addr;
static char last_rw;
static unsigned bdm_port;

static struct {
LONG where,mask;
WORD op;
} oldcode [MAX_Breakpoint];

static unsigned InstalledPorts = 0;
static unsigned OldDataPort, OldControlPort;

/* low-level drivers for parallel port */

static unsigned bdm_getstat (void);
static void bdm_stop (void);
static void bdm_step (void);
static LONG bdm_clk (WORD, int);
static char frozen;
static void bdm_clrerror (void);

/* ICD_ValidPorts returns unsigned integer where each bit position
 * indicates an installed LPT port (Bit 0 = LPT1, bit 1 = LPT2, etc)
 */

static unsigned ICD_ValidPorts (void)
{
	unsigned int64 *BIOSInfo = BIOSAddress;
	unsigned result = 0, mask = 1, index = 0;

	while (index < MAXLPT)
	{
		if (*BIOSInfo) result |= mask;
		BIOSInfo++;
		mask <<= 1;
		index++;
	}
	return InstalledPorts = result;
}

/* ICD_DeInit is called before program quits or goes to DOS
 * un-do any bad things which have been done to talk to target
 */

static void ICD_DeInit (void)
{
	if (ICDInfo.DriverInstalled)
	{
		outportb (bdm_port, OldDataPort);
		outportb (bdm_port + bdm_ctl, OldControlPort);
		ICDInfo.DriverInstalled = 0;
	}
}

/* ICD_Init initializes parallel port to talk to target */

static int ICD_Init (unsigned port, unsigned baud)
{
	unsigned far *BIOSInfo = BIOSAddress;

	ICD_DeInit ();
	memset (oldcode, 0, sizeof (oldcode));
	if (!(InstalledPorts & (1 << (port - 1)))) return -1;
	bdm_port = BIOSInfo [port - 1];
	if (!bdm_port) return -1;
	OldDataPort = inportb (bdm_port);
	OldControlPort = inportb (bdm_port + bdm_ctl);
	outportb (bdm_port+bdm_ctl,step_out | dsclk | rst_out);
	ICD_GetStatus ();
	ICDInfo.CurrentPort = port;
	ICDInfo.CurrentSpeed = baud;
	ICDInfo.DriverInstalled = 1;
	return 0;
}

/* bdm_error handles call to DriverError if error detected by bdm routines */

static char *err_types [] =
{ "Unknown fault",
  "Target power failed",
  "Cable disconnected",
  "No MCU response to Breakpoint",
  "Can't clock MCU when in reset",
  "Port not installed"};

#define	BDM_FAULT_UNKNOWN	0
#define	BDM_FAULT_POWER		1
#define	BDM_FAULT_CABLE		2
#define	BDM_FAULT_RESPONSE	3
#define	BDM_FAULT_RESET		4
#define	BDM_FAULT_PORT		5
#define BDM_FAULT_BERR		6

static void bdm_error (int type)
{
	if ((type > BDM_FAULT_BERR) || (type < 0)) type = 0;
	if (type < BDM_FAULT_BERR) DriverError (err_types [type]);
	sprintf (scratch, "BUS ERROR occurred %s address $%lX",
		last_rw ? "reading" : "writing", last_addr);
	restore_fc ();
	DriverError (scratch);
}

static void set_bp (void)
{
	int i;

	 for (i = 0; i < MAX_Breakpoint; i++)
	 if (oldcode [i].mask)
	 {
		oldcode [i].op = (short) ICD_GetWord (oldcode [i].where);
		ICD_PutWord (oldcode [i].where, (WORD) BGND);
	 }
}

static void replace_bp (void)
{
	int i;

	 for (i = 0; i < MAX_Breakpoint; i++)
		if (oldcode [i].mask)
			ICD_PutWord (oldcode [i].where,oldcode [i].op);
}

static last_stat = 0;

static unsigned ICD_GetStatus (void)
{
	unsigned c = bdm_getstat ();
	unsigned sigstat = (c & BDM_FREEZE) ? TARGETSTOPPED : 0;

	if ((c & BDM_FREEZE) && !(last_stat & BDM_FREEZE)) replace_bp ();
	last_stat = c;
	return sigstat;
}

static void bdm_clrerror (void)
{
	bdm_clk (BDM_NOP, CommandBitCount);
	bdm_clk (BDM_NOP, CommandBitCount);
	bdm_clk (BDM_NOP, CommandBitCount);
	bdm_clk (BDM_NOP, CommandBitCount);
	while (bdm_clk (0, 1)) ;
	while (!bdm_clk (0, 1)) ;
	bdm_clk (0, 15);
}

static LONG bdm_read (LONG addr, int pcount, WORD cmd, ...)
{
	va_list arg;
	int err,count;
	LONG response,result;

	last_addr = addr;
	last_rw = 1;
	result = 0L;
	for (err = 3; err; err--)
	{
		count = pcount;
		va_start (arg,cmd);
		response = bdm_clk (cmd, CommandBitCount);
		while (count--)
			if ((response = bdm_clk (va_arg(arg,WORD), CommandBitCount)) > BDM_NOTREADY)
			{
				if (response == BDM_BERR) bdm_error (BDM_FAULT_BERR);
				continue;
			}
		for (count = (cmd & BDM_LONGSIZE) ? 2 : 1; count; count--)
		{
			while ((response = bdm_clk (BDM_NOP, CommandBitCount)) == BDM_NOTREADY) ;
			if (response < BDM_NOTREADY)
			{
				result <<= 16;
				result |= response;
			}
			else
			{
				if (response == BDM_BERR) bdm_error (BDM_FAULT_BERR);
				break;
			}
		}
		if (response > BDM_NOTREADY)
		{
			if (response == BDM_BERR) bdm_error (BDM_FAULT_BERR);
			bdm_clrerror ();
			continue;
		}
		va_end (arg);
		break;
	}
	response = bdm_clk (BDM_NOP, CommandBitCount);
	if (response == BDM_BERR) bdm_error (BDM_FAULT_BERR);
	return result;
}

static void bdm_write (LONG addr, int pcount, WORD cmd, ...)
{
	va_list arg;
	int err,count;
	LONG response;

	last_addr = addr;
	last_rw = 0;
	for (err = 3; err; err--)
	{
		count = pcount;
		va_start (arg,cmd);
		response = bdm_clk (cmd, CommandBitCount);
		while (count--)
			if ((response = bdm_clk (va_arg(arg,WORD), CommandBitCount)) > BDM_NOTREADY)
			{
				if (response == BDM_BERR) bdm_error (BDM_FAULT_BERR);
				bdm_clrerror ();
				continue;
			}
		while ((response = bdm_clk (BDM_NOP, CommandBitCount)) == BDM_NOTREADY) ;
		if (response == BDM_CMDCMPLTE) break;
		else if (response == BDM_BERR) bdm_error (BDM_FAULT_BERR);

	}
	va_end (arg);
	response = bdm_clk (BDM_NOP, CommandBitCount);
	if (response == BDM_BERR) bdm_error (BDM_FAULT_BERR);
}

static int ICD_BR (int which, LONG where, LONG mask)
{
	int frozen = ICD_StopChip ();

	where &= ~ (LONG) 1;
	oldcode[which].where = where;
	oldcode[which].mask = mask;
	if (frozen) ICD_RunChip (0L);
	return which;
}

static int ICD_NoBR (int which)
{
	int frozen = ICD_StopChip ();

	oldcode [which].mask = 0L;
	if (frozen) ICD_RunChip (0L);
	return 0;
}

static LONG ICD_GetBR (int which)
{
	return oldcode[which].where;
}

static LONG ICD_GetMask (int which)
{
	return oldcode[which].mask;
}

static void ICD_HitBreakpoint (LONG where)
{
	ICD_PutReg (REG_PC, where - 2L);
}

static int ICD_StopChip (void)
{
	bdm_stop ();
	if (!frozen) return 0;
	replace_bp ();
	return 1;
}

static void ICD_StepChip (void)
{
	ICD_StopChip ();
	bdm_step ();
}

static void ICD_ResetChip (void)
{
	ICD_StopChip ();
	set_bp ();
	outportb (bdm_port + bdm_ctl, dsclk | step_out);
	delay (waitcnt);
	outportb (bdm_port + bdm_ctl, dsclk | rst_out | step_out);
}

static void ICD_RestartChip (void)
{
	ICD_StopChip ();
	outportb (bdm_port + bdm_ctl, dsclk);
	delay (waitcnt);
	bdm_stop ();
}

static void ICD_RunChip (LONG where)
{
	ICD_StopChip ();
	set_bp ();
	if (where)
		bdm_write (where, 2, BDM_WSREG + BDM_RPC, (WORD) (where >> 16), (WORD) where);
	bdm_clk (BDM_GO, CommandBitCount);
	last_stat &= ~BDM_FREEZE;
}

static LONG ICD_GetByte (LONG x)
{
	int frozen,save;
	LONG result;

	frozen = ICD_StopChip ();
	save = set_fc ();
	result = bdm_read (x, 2, BDM_READ, (WORD) (x >> 16), (WORD) x);
	if (save) restore_fc ();
	if (frozen) ICD_RunChip (0L);
	return result;
}

static void ICD_PutByte (LONG x, BYTE y)
{
	int frozen,save;

	frozen = ICD_StopChip ();
	save = set_fc ();
	bdm_write (x, 3, BDM_WRITE, (WORD) (x >> 16), (WORD) x, (WORD) y);
	if (save) restore_fc ();
	if (frozen) ICD_RunChip (0L);
}

static void ICD_FillByte (LONG x, BYTE y)
{
	int frozen,save;

	frozen = ICD_StopChip ();
	save = set_fc ();
	bdm_write (x, 1, BDM_FILL, (WORD) y);
	if (save) restore_fc ();
	if (frozen) ICD_RunChip (0L);
}

static LONG ICD_DumpByte (LONG x)
{
	return ICD_GetByte (x);
}

static LONG ICD_GetWord (LONG x)
{
	int frozen,save;
	LONG result;

	frozen = ICD_StopChip ();
	save = set_fc ();
	result = bdm_read (x, 2, BDM_READ + BDM_WORDSIZE, (WORD) (x >> 16), (WORD) x);
	if (save) restore_fc ();
	if (frozen) ICD_RunChip (0L);
	return result;
}

static void ICD_PutWord (LONG x, WORD y)
{
	int frozen,save;

	frozen = ICD_StopChip ();
	save = set_fc ();
	bdm_write (x, 3, BDM_WRITE + BDM_WORDSIZE, (WORD) (x >> 16), (WORD) x, y);
	if (save) restore_fc ();
	if (frozen) ICD_RunChip (0L);
}

static void ICD_FillWord (LONG x, WORD y)
{
	int frozen,save;

	frozen = ICD_StopChip ();
	save = set_fc ();
	bdm_write (x, 1, BDM_FILL + BDM_WORDSIZE, y);
	if (save) restore_fc ();
	if (frozen) ICD_RunChip (0L);
}

static LONG ICD_DumpWord (LONG x)
{
	return ICD_GetWord (x);
}

static LONG ICD_GetLong (LONG x)
{
	int frozen,save;
	LONG result;

	frozen = ICD_StopChip ();
	save = set_fc ();
	result = bdm_read (x, 2, BDM_READ + BDM_LONGSIZE, (WORD) (x >> 16), (WORD) x);
	if (save) restore_fc ();
	if (frozen) ICD_RunChip (0L);
	return result;
}

static void ICD_PutLong (LONG x, LONG y)
{
	int frozen,save;

	frozen = ICD_StopChip ();
	save = set_fc ();
	bdm_write (x, 4, BDM_WRITE + BDM_LONGSIZE, (WORD) (x >> 16), (WORD) x, (WORD) (y >> 16), (WORD) y);
	if (save) restore_fc ();
	if (frozen) ICD_RunChip (0L);
}

static void ICD_FillLong (LONG x, LONG y)
{
	int frozen,save;

	frozen = ICD_StopChip ();
	save = set_fc ();
	bdm_write (x, 2, BDM_FILL + BDM_LONGSIZE, (WORD) (y >> 16), (WORD) y);
	if (save) restore_fc ();
	if (frozen) ICD_RunChip (0L);
}

static LONG ICD_DumpLong (LONG x)
{
	return ICD_GetLong (x);
}

static WORD RegCodes [] = {
BDM_WSREG + BDM_RPC, BDM_WSREG + BDM_USP,
BDM_WSREG + BDM_SSP, BDM_WSREG + BDM_VBR,
BDM_WSREG + BDM_SR, BDM_WSREG + BDM_SFC,
BDM_WSREG + BDM_DFC, BDM_WSREG + BDM_ATEMP,

BDM_WRREG + 0, BDM_WRREG + 1, BDM_WRREG + 2, BDM_WRREG + 3,
BDM_WRREG + 4, BDM_WRREG + 5, BDM_WRREG + 6, BDM_WRREG + 7,

BDM_WRREG + 8, BDM_WRREG + 9, BDM_WRREG + 10, BDM_WRREG + 11,
BDM_WRREG + 12, BDM_WRREG + 13, BDM_WRREG + 14, BDM_WRREG + 15
};

static LONG ICD_GetReg (unsigned which)
{
	int frozen;
	LONG result;

	frozen = ICD_StopChip ();
	result = bdm_read (0L,0,BDM_RDBIT + RegCodes [which]);
	if (frozen) ICD_RunChip (0L);
	return result;
}

static void ICD_PutReg (unsigned which, LONG data)
{
	int frozen;

	frozen = ICD_StopChip ();
	bdm_write (0L, 2,RegCodes [which], (WORD) (data >> 16), (WORD) data);
	if (frozen) ICD_RunChip (0L);
}

/* bdm-stop stops MCU in preparation for sending command	 */

static void bdm_stop (void)
{
	unsigned ctr;

	frozen = 0;
	if (inportb (bdm_port + bdm_stat) & freeze) return;
	frozen = 1;
	outportb (bdm_port + bdm_ctl, dsclk + rst_out);
	for (ctr = waitcnt; ctr; ctr--)
	{
		if (inportb (bdm_port + bdm_stat) & freeze) break;
		delay (1);
	}
	if (!ctr)
	{
		outportb (bdm_port + bdm_ctl, dsclk | rst_out | force_berr);
		delay (waitcnt);
		for (ctr = waitcnt; ctr; ctr--)
		{
			if (inportb (bdm_port + bdm_stat) & freeze) break;
			delay (1);
		}
	}
	outportb (bdm_port + bdm_ctl, dsclk | rst_out | step_out);
	if (!ctr) bdm_error (BDM_FAULT_RESPONSE);
}

/* bdm_clk sends <value> to MCU for <parameter> bits, returns MCU response */

static LONG bdm_clk (WORD value, int count)
{
	LONG ShiftRegister = ((LONG) value) << (32 - count);
#ifdef	C_CLOCK_ROUTINE
	unsigned char DataOut;
#else
	unsigned DelayCount = ICDInfo.CurrentSpeed;
#endif

#ifdef	C_CLOCK_ROUTINE

	while (count--)
	{
		DataOut = (ShiftRegister & 0x80000000) ? dsi : 0;
		ShiftRegister <<= 1;
		if (!(inportb (bdm_port + bdm_stat) & dso))
			ShiftRegister |= 1;
		outportb (bdm_port + bdm_ctl, DataOut | rst_out | oe | step_out);
		delay (ICDInfo.CurrentSpeed + 1);
		outportb (bdm_port + bdm_ctl, DataOut | rst_out | oe | step_out | dsclk);
		delay ((ICDInfo.CurrentSpeed >> 1) + 1);
	}
#else
	asm	push	ax
	asm	push	bx	/* msw of shift register */
	asm	push	cx	/* loop count */
	asm	push	dx	/* port address */
	asm	push	si	/* lsw of shift register */
	asm	push	di	/* delay count */
	asm	mov	bx,word ptr ShiftRegister+2	/* high order */
	asm	mov	si,count
	asm 	mov	dx,bdm_port
	asm	add	dx,bdm_stat
	asm	mov	cx,word ptr ShiftRegister	/* low order */
clk1:	asm	in	al,dx	/* get next bit of input */
	asm	test	al,dso	/* clear carry, test data from MCU */
	asm	jnz	loop1	/* branch if 1 */
	asm	stc		/* it's a 0 - invert to a 1 */
loop1:	asm	rcl	cx,1	/* shift data into cx */
	asm	rcl	bx,1
	asm	mov	al,step_out+rst_out+oe
	asm	jnb	loop2	/* skip if carry == 0 */
	asm	or	al,dsi	/* otherwise or in a '1' */
loop2:	asm	add	dx,bdm_ctl-bdm_stat	/* point to control port */
	asm	out	dx,al	/* output to port */
	asm	mov	di,DelayCount	/* delay for signal to settle */
	asm	inc	di
loop3:	asm	nop
	asm	nop
	asm	dec	di
	asm	jnz	loop3
	asm	or	al,dsclk	/* set dsclk - rising edge on cable */
	asm	out	dx,al
	asm	mov	di,DelayCount	/* another delay... */
	asm	inc	di
loop4:	asm	nop
	asm	nop
	asm	dec	di
	asm	jnz	loop4
	asm	sub	dx,bdm_ctl - bdm_stat
	asm	dec	si
	asm	jnz	clk1
	asm	mov	word ptr ShiftRegister+2,bx
	asm	mov	word ptr ShiftRegister,cx
	asm	pop	di
	asm	pop	si
	asm	pop	dx
	asm	pop	cx
	asm	pop	bx
	asm	pop	ax
#endif
	outportb (bdm_port + bdm_ctl, dsclk | step_out | rst_out);
	return ShiftRegister;
}

/* bdm_step sends GO command word, then triggers breakpoint on first fetch        */

static void bdm_step (void)
{
#define	DataOut	(go_cmd & 1 ? dsi : 0)

	bdm_clk (go_cmd >> 1, CommandBitCount - 1);
	outportb (bdm_port + bdm_ctl, oe | step_out | DataOut | rst_out);
	delay (ICDInfo.CurrentSpeed + 1);
	disable ();
	outportb (bdm_port + bdm_ctl, oe | DataOut | dsclk | rst_out);
	outportb (bdm_port + bdm_ctl, dsclk | rst_out);
	enable ();
}

/* bdm_getstat returns status of MCU                                              */

static unsigned bdm_getstat (void)
{
	BYTE temp = inportb (bdm_port + bdm_stat);

	return 	(temp & freeze ? BDM_FREEZE : 0);
}

/* Delay waits before returning to the calling routine				  */

static void delay (unsigned count)
{
		asm	push	ax
	_AX = count;
/*									       8088   80286   80386 */
dly_0:	asm	nop				/* delay			3	3	3 */
	asm	nop				/* delay			3	3	3 */
	asm	dec	ax			/*                              3	2	2 */
	asm	jnz	dly_0			/* repeat till count goes to 0	16	18	18 clocks */
/*											==	==		== */
/*                                                                    			27	26		26 */
	asm	pop	ax
}
							/*			5.7	2.16	1.04 us/loop at	 */
							/*			4.77	12	25 MHz clock     */
