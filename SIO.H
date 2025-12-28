/* sio.h - header file for serial I/O routines */

#define	NO_PARITY		0
#define	ODD_PARITY	0x8
#define	EVEN_PARITY	0x18
#define	SPACE_PARITY	0x28
#define	MARK_PARITY	0x38

#define	FIVE_DATA		0
#define	SIX_DATA		1
#define	SEVEN_DATA	2
#define	EIGHT_DATA	3

#define	ONE_STOP		0
#define	TWO_STOP		4

unsigned ComPortsInstalled (void);
int ComOpen (unsigned /* Port */, long /* Baud */, unsigned char /* Parity */, unsigned char /* Databits */, unsigned char /* Stopbits */);
int ComCharReady (unsigned /* Port */);
int ComRead (unsigned /* Port */);
int ComWrite (unsigned /* Port */, char /* Data */);
int ComClose (unsigned /* Port */);
void ComFlush (unsigned /* Port */);
