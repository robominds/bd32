/* disp-mac.h - define display macros */

extern do_rd (int, char **);
extern do_disasm (int, char **);

#define DISPLAYREGISTERS	do_rd (1, (char **) 0)
#define	DISPLAYCODE		do_disasm (0, (char **) 0)

#define	REDISPLAY		0	/* full redisplay of disassembly window */
#define	OPTIMALREDISPLAY	1	/* track PC */
#define	UPDATEADDRESS		2	/* display only the line displaying address */
#define	BLANKPC			3	/* redraw without highlight */

void DisplayCode (LONG Address, unsigned Mode);
LONG Disassemble (LONG CurrentPC, unsigned TheLine, unsigned Count, unsigned Blank);

extern int UseWindows;			/* non-zero means use windowed UI */

/* end of disp-mac.h */
