/* lpt.c - routines to talk to CPU32 target system
 * using Scott's magic cable
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

#define	bdm_ctl         2               /* offset of control port from base */
#define	step_out        8               /* set low to gate IFETCH onto BKPT */
#define	dsi             4               /* data shift input - PC->MCU       */
#define	rst_out         2               /* set high to force reset on MCU   */
#define	reset           2               /* low when RESET asserted          */
#define	dsclk           1               /* data shift clock/breakpoint pin  */

#define	bdm_stat        1               /* offset of status port from base     */
#define	nc              0x80            /* not connected - low when unplugged  */
#define	pwr_dwn         0x40            /* power down - low when Vcc failed    */
#define	dso             0x20            /* data shift output - MCU->PC         */
#define	freeze          8               /* FREEZE asserted when MCU stopped    */

#define	waitcnt         0xffff          /* no of loops to wait for response    */
#define	go_cmd          0xc00           /* GO command                          */
#define	nop_cmd         0               /* null (NOP) command                  */
#define	CommandBitCount	17		/* number of bits in BDM command */

#define	BDM_FREEZE	1		/* return status - freeze asserted     */
#define	BDM_RESET	2		/* return status - reset asserted      */
#define	BDM_NC		4		/* return status - cable not connected */
#define	BDM_PWR		8		/* return status - target not powered  */

extern void DriverError (char *Message);

static unsigned LPT_ValidPorts (void);
static int LPT_Init (unsigned Port, unsigned Speed);
static void LPT_DeInit (void);
static unsigned LPT_GetStatus (void);
static int LPT_BR (int which, LONG where, LONG mask);
static int LPT_NoBR (int which);
static LONG LPT_GetBR (int which);
static LONG LPT_GetMask (int which);
static void LPT_HitBreakpoint (LONG where);
static int LPT_StopChip (void);
static void LPT_StepChip (void);
static void LPT_ResetChip (void);
static void LPT_RestartChip (void);
static void LPT_RunChip (LONG where);
static LONG LPT_GetByte (LONG Where);
static LONG LPT_GetWord (LONG Where);
static LONG LPT_GetLong (LONG Where);
static LONG LPT_DumpByte (LONG Where);
static LONG LPT_DumpWord (LONG Where);
static LONG LPT_DumpLong (LONG Where);
static void LPT_PutByte (LONG Where, BYTE Data);
static void LPT_PutWord (LONG Where, WORD Data);
static void LPT_PutLong (LONG Where, LONG Data);
static void LPT_FillByte (LONG Where, BYTE Data);
static void LPT_FillWord (LONG Where, WORD Data);
static void LPT_FillLong (LONG Where, LONG Data);
static LONG LPT_GetReg (unsigned which);
static void LPT_PutReg (unsigned which, LONG Data);
static void delay (unsigned count);

struct TargetInfo LPTInfo =
{
	"LPT",
	0,
	0,
	0,
	MAX_Breakpoint,
	TARGETRESET | TARGETSTOPPED,
	LPT_ValidPorts,
	LPT_Init,
	LPT_DeInit,
	LPT_GetStatus,
	LPT_BR,
	LPT_NoBR,
	LPT_GetBR,
	LPT_GetMask,
	LPT_HitBreakpoint,
	LPT_StopChip,
	LPT_StepChip,
	LPT_ResetChip,
	LPT_RestartChip,
	LPT_RunChip,
	LPT_GetByte,
	LPT_GetWord,
	LPT_GetLong,
	LPT_DumpByte,
	LPT_DumpWord,
	LPT_DumpLong,
	LPT_PutByte,
	LPT_PutWord,
	LPT_PutLong,
	LPT_FillByte,
	LPT_FillWord,
	LPT_FillLong,
	LPT_GetReg,
	LPT_PutReg
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

/* LPT_ValidPorts returns unsigned integer where each bit position
 * indicates an installed LPT port (Bit 0 = LPT1, bit 1 = LPT2, etc)
 */

static unsigned LPT_ValidPorts (void)
{
	unsigned far *BIOSInfo = BIOSAddress;
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

/* LPT_DeInit is called before program quits or goes to DOS
 * un-do any bad things which have been done to talk to target
 */

static void LPT_DeInit (void)
{
	if (LPTInfo.DriverInstalled)
	{
		outportb (bdm_port, OldDataPort);
		outportb (bdm_port + bdm_ctl, OldControlPort);
		LPTInfo.DriverInstalled = 0;
	}
}

/* LPT_Init initializes parallel port to talk to target */

static int LPT_Init (unsigned port, unsigned baud)
{
	unsigned far *BIOSInfo = BIOSAddress;

	LPT_DeInit ();
	memset (oldcode, 0, sizeof (oldcode));
	if (!(InstalledPorts & (1 << (port - 1)))) return -1;
	bdm_port = BIOSInfo [port - 1];
	if (!bdm_port) return -1;
	OldDataPort = inportb (bdm_port);
	OldControlPort = inportb (bdm_port + bdm_ctl);
	outportb (bdm_port+bdm_ctl,step_out);
	LPT_GetStatus ();
	LPTInfo.CurrentPort = port;
	LPTInfo.CurrentSpeed = baud;
	LPTInfo.DriverInstalled = 1;
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
		oldcode [i].op = (WORD) LPT_GetWord (oldcode [i].where);
		LPT_PutWord (oldcode [i].where, (WORD) BGND);
	 }
}

static void replace_bp (void)
{
	int i;

	 for (i = 0; i < MAX_Breakpoint; i++)
		if (oldcode [i].mask)
			LPT_PutWord (oldcode [i].where,oldcode [i].op);
}

static last_stat = 0;

static unsigned LPT_GetStatus (void)
{
	unsigned c = bdm_getstat ();
	unsigned sigstat = ((c & BDM_RESET) ? 0 : TARGETRESET) | ((c & BDM_FREEZE) ? TARGETSTOPPED : 0) | (c & (BDM_NC | BDM_PWR) ? TARGETINVALID: 0);

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

static int LPT_BR (int which, LONG where, LONG mask)
{
	int frozen = LPT_StopChip ();

	where &= ~ (LONG) 1;
	oldcode[which].where = where;
	oldcode[which].mask = mask;
	if (frozen) LPT_RunChip (0L);
	return which;
}

static int LPT_NoBR (int which)
{
	int frozen = LPT_StopChip ();

	oldcode [which].mask = 0L;
	if (frozen) LPT_RunChip (0L);
	return 0;
}

static LONG LPT_GetBR (int which)
{
	return oldcode[which].where;
}

static LONG LPT_GetMask (int which)
{
	return oldcode[which].mask;
}

static void LPT_HitBreakpoint (LONG where)
{
	LPT_PutReg (REG_PC, where - 2L);
}

static int LPT_StopChip (void)
{
	bdm_stop ();
	if (!frozen) return 0;
	replace_bp ();
	return 1;
}

static void LPT_StepChip (void)
{
	LPT_StopChip ();
	bdm_step ();
}

static void LPT_ResetChip (void)
{
	LPT_StopChip ();
	set_bp ();
	outportb (bdm_port + bdm_ctl, rst_out + step_out);
	delay (waitcnt);
	outportb (bdm_port + bdm_ctl, step_out);
}

static void LPT_RestartChip (void)
{
	unsigned LoopCount;

	LPT_StopChip ();
	outportb (bdm_port + bdm_ctl, rst_out  | dsclk);
	delay (waitcnt);
	outportb (bdm_port + bdm_ctl, dsclk);
	delay (waitcnt);
	outportb (bdm_port + bdm_ctl, step_out | dsclk);
	for (LoopCount = 0xffff; LoopCount; LoopCount--)
		if (inportb (bdm_port + bdm_stat) & BDM_FREEZE) break;
	if (!LoopCount) bdm_error (BDM_FAULT_RESPONSE);
}

static void LPT_RunChip (LONG where)
{
	LPT_StopChip ();
	set_bp ();
	if (where)
		bdm_write (where, 2, BDM_WSREG + BDM_RPC, (WORD) (where >> 16), (WORD) where);
	bdm_clk (BDM_GO, CommandBitCount);
	last_stat &= ~BDM_FREEZE;
}

static LONG LPT_GetByte (LONG x)
{
	int frozen,save;
	LONG result;

	frozen = LPT_StopChip ();
	save = set_fc ();
	result = bdm_read (x, 2, BDM_READ, (WORD) (x >> 16), (WORD) x);
	if (save) restore_fc ();
	if (frozen) LPT_RunChip (0L);
	return result;
}

static void LPT_PutByte (LONG x, BYTE y)
{
	int frozen,save;

	frozen = LPT_StopChip ();
	save = set_fc ();
	bdm_write (x, 3, BDM_WRITE, (WORD) (x >> 16), (WORD) x, (WORD) y);
	if (save) restore_fc ();
	if (frozen) LPT_RunChip (0L);
}

static void LPT_FillByte (LONG x, BYTE y)
{
	int frozen,save;

	frozen = LPT_StopChip ();
	save = set_fc ();
	bdm_write (x, 1, BDM_FILL, (WORD) y);
	if (save) restore_fc ();
	if (frozen) LPT_RunChip (0L);
}

static LONG LPT_DumpByte (LONG x)
{
	return LPT_GetByte (x);
}

static LONG LPT_GetWord (LONG x)
{
	int frozen,save;
	LONG result;

	frozen = LPT_StopChip ();
	save = set_fc ();
	result = bdm_read (x, 2, BDM_READ + BDM_WORDSIZE, (WORD) (x >> 16), (WORD) x);
	if (save) restore_fc ();
	if (frozen) LPT_RunChip (0L);
	return result;
}

static void LPT_PutWord (LONG x, WORD y)
{
	int frozen,save;

	frozen = LPT_StopChip ();
	save = set_fc ();
	bdm_write (x, 3, BDM_WRITE + BDM_WORDSIZE, (WORD) (x >> 16), (WORD) x, y);
	if (save) restore_fc ();
	if (frozen) LPT_RunChip (0L);
}

static void LPT_FillWord (LONG x, WORD y)
{
	int frozen,save;

	frozen = LPT_StopChip ();
	save = set_fc ();
	bdm_write (x, 1, BDM_FILL + BDM_WORDSIZE, y);
	if (save) restore_fc ();
	if (frozen) LPT_RunChip (0L);
}

static LONG LPT_DumpWord (LONG x)
{
	return LPT_GetWord (x);
}

static LONG LPT_GetLong (LONG x)
{
	int frozen,save;
	LONG result;

	frozen = LPT_StopChip ();
	save = set_fc ();
	result = bdm_read (x, 2, BDM_READ + BDM_LONGSIZE, (WORD) (x >> 16), (WORD) x);
	if (save) restore_fc ();
	if (frozen) LPT_RunChip (0L);
	return result;
}

static void LPT_PutLong (LONG x, LONG y)
{
	int frozen,save;

	frozen = LPT_StopChip ();
	save = set_fc ();
	bdm_write (x, 4, BDM_WRITE + BDM_LONGSIZE, (WORD) (x >> 16), (WORD) x, (WORD) (y >> 16), (WORD) y);
	if (save) restore_fc ();
	if (frozen) LPT_RunChip (0L);
}

static void LPT_FillLong (LONG x, LONG y)
{
	int frozen,save;

	frozen = LPT_StopChip ();
	save = set_fc ();
	bdm_write (x, 2, BDM_FILL + BDM_LONGSIZE, (WORD) (y >> 16), (WORD) y);
	if (save) restore_fc ();
	if (frozen) LPT_RunChip (0L);
}

static LONG LPT_DumpLong (LONG x)
{
	return LPT_GetLong (x);
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

static LONG LPT_GetReg (unsigned which)
{
	int frozen;
	LONG result;

	frozen = LPT_StopChip ();
	result = bdm_read (0L,0,BDM_RDBIT + RegCodes [which]);
	if (frozen) LPT_RunChip (0L);
	return result;
}

static void LPT_PutReg (unsigned which, LONG data)
{
	int frozen;

	frozen = LPT_StopChip ();
	bdm_write (0L, 2,RegCodes [which], (WORD) (data >> 16), (WORD) data);
	if (frozen) LPT_RunChip (0L);
}

/* bdm-stop stops MCU in preparation for sending command	 */

static void bdm_stop (void)
{
	unsigned ctr;

	frozen = 0;
	if (inportb (bdm_port + bdm_stat) & freeze) return;
	frozen = 1;
	outportb (bdm_port + bdm_ctl, dsclk + step_out);
	for (ctr = waitcnt; ctr; ctr--)
	{
		if (inportb (bdm_port + bdm_stat) & freeze) break;
		delay (1);
	}
	if (!ctr) bdm_error (BDM_FAULT_RESPONSE);
}

/* bdm_clk sends <value> to MCU for <parameter> bits, returns MCU response */

/* #define	C_CLOCK_ROUTINE */

static LONG bdm_clk (WORD value, int count)
{
	LONG ShiftRegister = ((LONG) value) << (32 - count);
#ifdef	C_CLOCK_ROUTINE
	unsigned char DataOut;
#else
	unsigned DelayCount = LPTInfo.CurrentSpeed;
#endif
	unsigned stat = bdm_getstat ();

	if (stat & BDM_RESET)
		bdm_error (BDM_FAULT_RESET);
	if (stat & BDM_NC)
		bdm_error (BDM_FAULT_CABLE);
	if (stat & BDM_PWR)
		bdm_error (BDM_FAULT_POWER);
#ifdef	C_CLOCK_ROUTINE

	while (count--)
	{
		DataOut = (ShiftRegister & 0x80000000) ? dsi : 0;
		ShiftRegister <<= 1;
		if (!(inportb (bdm_port + bdm_stat) & dso))
			ShiftRegister |= 1;
		outportb (bdm_port + bdm_ctl, DataOut | step_out | dsclk);
		delay (LPTInfo.CurrentSpeed + 1);
		outportb (bdm_port + bdm_ctl, DataOut | step_out);
		delay ((LPTInfo.CurrentSpeed >> 1) + 1);
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
	asm	jnz	loop1	/* branch if 1 (invert data) */
	asm	stc		/* it's a zero - invert to 1 */
loop1:	asm	rcl	cx,1	/* shift data into cx */
	asm	rcl	bx,1
	asm	mov	al,step_out+dsclk
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
	asm	and	al,0xff-dsclk	/* clear clock - rising edge on cable */
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
	return ShiftRegister;
}

/* bdm_step sends GO command word, then triggers breakpoint on first fetch        */

static void bdm_step (void)
{
#define	DataOut	(go_cmd & 1 ? dsi : 0)
	unsigned stat = bdm_getstat ();

	if (stat & BDM_RESET)
		bdm_error (BDM_FAULT_RESET);
	if (stat & BDM_NC)
		bdm_error (BDM_FAULT_CABLE);
	if (stat & BDM_PWR)
		bdm_error (BDM_FAULT_POWER);
	bdm_clk (go_cmd >> 1, CommandBitCount - 1);
	outportb (bdm_port + bdm_ctl, dsclk | step_out | DataOut);
	delay (LPTInfo.CurrentSpeed + 1);
	disable ();
	outportb (bdm_port + bdm_ctl, dsclk | DataOut);
	delay (1);
	outportb (bdm_port + bdm_ctl, DataOut);
        enable ();
}

/* bdm_getstat returns status of MCU                                              */

static unsigned bdm_getstat (void)
{
	BYTE temp = inportb (bdm_port + bdm_stat);

	if (!(temp & nc)) return BDM_NC;
	if (!(temp & pwr_dwn)) return BDM_PWR;
	return 	(temp & freeze ? BDM_FREEZE : 0)
		| (inportb (bdm_port + bdm_ctl) & reset ? BDM_RESET : 0);
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
