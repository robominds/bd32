/* sio.c - Scott's own interrupt driven serial I/O for the PC */

#include	<stdint.h>
#include	<stdio.h>
#include	<alloc.h>
#include	"sio.h"
#include	"subs.h"

#define	INT_REG		0x20		/* interrupt controller acknowledge reg */
#define	INT_ACK		0x20		/* value required to acknowledge int */
#define	INT_CTLR		0x21		/* address of 8259 mask register */

#define	IER_OFFSET	1		/* offset within 8250 of int enable reg */
#define	LCR_OFFSET	3         /* offset within 8250 of line control reg */
#define	MCR_OFFSET	4         /* offset within 8250 of modem control enable reg */
#define	LSR_OFFSET	5         /* offset within 8250 of line status reg */
#define	DLL_OFFSET	0         /* offset within 8250 of divisor latch lsb */
#define	DLM_OFFSET	1         /* offset within 8250 of divisor latch msb */
#define	DATA_OFFSET	0         /* offset within 8250 of data I/O reg */

#define	BUFFSIZE		256
#define	CRYSTAL		1843200L	/* 8250 crystal frequency */
#define	MAXPORTS	4	/* max number of ports we can handle */
#define	BIOSAddress	((uint64_t *) 0x400)
#define	PORTCOUNT	4

struct ModemStat
{
	unsigned char 	ier,		/* interrupt enable register */
				lcr,		/* line control register */
				mcr,		/* modem control register */
				dll,		/* divisor latch - lsb */
				dlm;		/* divisor latch - msb */
};

static void COM1Handler (void), COM2Handler (void),
			COM3Handler (void), COM4Handler (void);

static struct sio_data
{
	unsigned 	PortAddress,
			Vector,
			IntMask,
			Available,
			Initted;
	void (*OurHandler)();
	volatile unsigned	GetPtr,
					PutPtr,
					ByteCount;
	void (*OldVector)();
	char *InBuffer;
	struct ModemStat OldStat;
} Ports [] =
{
	{0,	0,	0,	0,	0,	COM1Handler},
	{0, 	0,	0,	0,	0,	COM2Handler},
	{0, 	0,	0,	0,	0,	COM3Handler},
	{0,	0,	0,	0,	0,	COM4Handler}
};

static struct
{
	unsigned PortAddress, InterruptVector, InterruptMask;
} PortInfo [] =
{
	{0x3f8,	0xc,	0x10},
	{0x2f8, 0xb,	0x8},
	{0x3e8, 0xc,	0x10},
	{0x2e8, 0xb,	0x8},
	{0,	0,	0}
};

unsigned ComPortsInstalled (void)
{
	uint64_t *BIOSInfo = BIOSAddress;
	unsigned scan, result = 0, mask = 1, index = 0;

	while (index < PORTCOUNT)
	{
		if (*BIOSInfo)
			for (scan = 0; PortInfo [scan].PortAddress; scan++)
				if (PortInfo [scan].PortAddress == *BIOSInfo)
				{
					result |= mask;
					Ports [index].PortAddress = PortInfo [scan].PortAddress;
					Ports [index].Vector = PortInfo [scan].InterruptVector;
					Ports [index].IntMask = PortInfo [scan].InterruptMask;
					Ports [index].Available = 1;
				}
		BIOSInfo++;
		mask <<= 1;
		index++;
	}
	return result;
}

static int GetFlags (void)
{
	static int Flags;

	__asm__("pushf\n\t"
		"pop ax\n\t"
		"mov Flags, ax\n\t");
	return Flags;
}

static void SetFlags (int Flags)
{
	static int NewFlags;

	NewFlags = Flags;
	__asm__("mov ax, NewFlags\n\t"
		"push ax\n\t"
		"popf\n\t");
}

static void SaveModemStatus (struct sio_data *where)
{
	unsigned Base;
	int Flags = GetFlags ();
	struct ModemStat *Save = &where->OldStat;

	disable ();
	Base = where->PortAddress;
	Save->ier = inportb (Base + IER_OFFSET);
	Save->lcr = inportb (Base + LCR_OFFSET);
	Save->mcr = inportb (Base + MCR_OFFSET);
	outportb (Base + LCR_OFFSET, Save->lcr | 0x80);
	Save->dll = inportb (Base + DLL_OFFSET);
	Save->dlm = inportb (Base + DLM_OFFSET);
	outportb (Base + LCR_OFFSET, Save->lcr);
	SetFlags (Flags);
}

static void SetModem (unsigned Base,
			unsigned char ier,
			unsigned char lcr,
			unsigned char mcr,
			unsigned char dll,
			unsigned char dlm)
{
	int Flags = GetFlags ();

	disable ();
	outportb (Base + IER_OFFSET, ier);
	outportb (Base + LCR_OFFSET, lcr);
	outportb (Base + MCR_OFFSET, mcr);
	outportb (Base + LCR_OFFSET, lcr | 0x80);
	outportb (Base + DLL_OFFSET, dll);
	outportb (Base + DLM_OFFSET, dlm);
	outportb (Base + LCR_OFFSET, lcr);
	SetFlags (Flags);
}

static void RestoreModemStatus (struct sio_data *where)
{
	struct ModemStat *Save = &where->OldStat;

	SetModem (where->PortAddress,
			Save->ier,
			Save->lcr,
			Save->mcr,
			Save->dll,
			Save->dlm);
}

typedef void (*op_fun)(void);

//void (*foo)(void) SetInterrupt( unsigned Vector, void  (*NewVector)())
op_fun SetInterrupt( unsigned Vector, void  (*NewVector)())
{
	void (*OldVector)() = getvect (Vector);

	setvect (Vector, NewVector);
	return OldVector;
}

void ComFlush (unsigned Port)
{
	struct sio_data *ThePort;
	int Flags = GetFlags ();

	if (Port < 1 || Port > PORTCOUNT) return;
	ThePort = &Ports [Port - 1];
	if (!ThePort->Initted || !ThePort->Available) return;
	disable ();
	ThePort->GetPtr = ThePort->PutPtr = ThePort->ByteCount = 0;
	SetFlags (Flags);
}

int ComOpen (unsigned Port, long Baud, unsigned char Parity, unsigned char Databits, unsigned char Stopbits)
{
	struct sio_data *ThePort;
	unsigned Divisor = (short) (CRYSTAL / (16L * Baud));

	if (Port <1 || Port > PORTCOUNT) return -1;
	ThePort = &Ports [Port - 1];
	if (!ThePort->Available) return -1;
	disable ();
	if (!ThePort->Initted)
	{
		if (!(ThePort->InBuffer = (char *) malloc (BUFFSIZE))) return -2;
		SaveModemStatus (ThePort);
		ThePort->OldVector = SetInterrupt (ThePort->Vector, ThePort->OurHandler);
	}
	outportb (INT_CTLR, inportb (INT_CTLR) & ~ThePort->IntMask);
	SetModem (ThePort->PortAddress,
			1,							/* int enable reg - receive only */
			Parity | Databits | Stopbits,		/* line control reg */
			0xf,							/* modem control reg */
			(unsigned char) (Divisor & 0xff),	/* baud rate divisor */
			(unsigned char) (Divisor >> 8));
	ThePort->Initted = 1;
	ComFlush (Port);
	enable ();
	return 0;
}

int ComRead (unsigned Port)
{
	struct sio_data *ThePort;
	int save;

	if (Port < 1 || Port > PORTCOUNT) return -1;
	ThePort = &Ports [Port - 1];
	if (!ThePort->Initted || !ThePort->Available) return -2;
	while (!ThePort->ByteCount) ;
	disable ();
	save = ThePort->InBuffer [ThePort->GetPtr];
	if (++ThePort->GetPtr == BUFFSIZE) ThePort->GetPtr = 0;
	ThePort->ByteCount--;
	enable ();
	return save;
}

int ComWrite (unsigned Port, char Data)
{
	struct sio_data *ThePort;

	if (Port < 1 || Port > PORTCOUNT) return -1;
	ThePort = &Ports [Port - 1];
	if (!ThePort->Initted || !ThePort->Available) return -2;
	while (!(inportb (ThePort->PortAddress + LSR_OFFSET) & 0x20)) ;
	outportb (ThePort->PortAddress + DATA_OFFSET, Data);
	return 0;
}

int ComClose (unsigned Port)
{
	struct sio_data *ThePort;

	if (Port < 1 || Port > PORTCOUNT) return -1;
	ThePort = &Ports [Port - 1];
	if (!ThePort->Initted || !ThePort->Available) return -2;
	disable ();
	SetInterrupt (ThePort->Vector, ThePort->OldVector);
	outportb (INT_CTLR, inportb (INT_CTLR) | ThePort->IntMask);
	RestoreModemStatus (ThePort);
	enable ();
	ThePort->Initted = 0;
	return 0;
}

int ComCharReady (unsigned Port)
{
	struct sio_data *ThePort;

	if (Port < 1 || Port > PORTCOUNT) return -1;
	ThePort = &Ports [Port - 1];
	if (!ThePort->Initted || !ThePort->Available) return -2;
	return ThePort->ByteCount;
}

static void CharInterrupt (struct sio_data *ThePort)
{
	ThePort->InBuffer [ThePort->PutPtr] = inportb (ThePort->PortAddress + DATA_OFFSET);
	outportb (INT_REG, INT_ACK);
	if (++ThePort->PutPtr == BUFFSIZE) ThePort->PutPtr = 0;
	if (ThePort->ByteCount == BUFFSIZE) ThePort->GetPtr = ThePort->PutPtr;
	else ThePort->ByteCount++;
}

static void COM1Handler (void)
{
	CharInterrupt (&Ports [0]);
}

static void COM2Handler (void)
{
	CharInterrupt (&Ports [1]);
}

static void COM3Handler (void)
{
	CharInterrupt (&Ports [2]);
}

static void COM4Handler (void)
{
	CharInterrupt (&Ports [3]);
}
