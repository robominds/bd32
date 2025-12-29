/* digp.h - get and put routines prototyped here */

#pragma once

LONG GETBYTE (LONG x);
void PUTBYTE (LONG x, LONG y);
void FILLBYTE (LONG x, LONG y);
LONG DUMPBYTE (LONG x);
LONG GETWORD (LONG x);
void PUTWORD (LONG x, LONG y);
void FILLWORD (LONG x, LONG y);
LONG GETLONG (LONG x);
void PUTLONG (LONG x, LONG y);
void FILLLONG (LONG x, LONG y);
//LONG GETREG (unsigned regname);
void PUTREG (unsigned regname, LONG data);

/* end of digp.h */
