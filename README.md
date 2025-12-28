               Release Info for BD32 Version 1.22
               ==================================

Legalese
========

These files are Copyright (C) 1990-1994 by Scott Howard, all rights
reserved.  You are licenced to use this software without charge, and to
distribute this software to others provided that you do not charge any fee
for doing so.

All users of this software acknowledge the following disclaimer:

"All contents of this disk and the files contained therein are provided
as is, without warrantees of any kind.  The author (Scott Howard)
disclaims all warranties, expressed or implied, including, without
limitation, the warranties of merchantability and of fitness of these
programs and files for any purpose.  The author (Scott Howard) assumes
no liability for damages, direct or consequential, which may result from
the use of these files and programs.  Further, Motorola Canada Ltd. and
Motorola Inc. are similiarly released from all responsibility for this
software."

What is BD32?
=============

BD32 is an assembly language debugger ("<B>ackground mode <D>ebugger") for the
Motorola 68300 series of microcontrollers.  The debugger runs on PC compatible
computers and controls the microcontroller through its Background Debug Mode
(BDM) serial port.  This is achieved either through Motorola's BCCDI board
(Business Card Computer Development Interface) or directly by the PC, either
through Motorola's ICD interface card (part no. M68ICD32) or a special interface
circuit which the user must construct.  The interface circuit is controlled
directly by the PC, through one of its parallel printer ports (LPT1 or LPT2).
The schematic diagram for this simple interface circuit (total circuitry is two
74HC type logic devices) is contained in a disk file within this archive.

This archive should contain (as a minimum) the following files:

BD32.EXE                executable version of the debugger
BD32.DOC                documentation on how to use to debugger
TEST.S, TEST.D32        sample Target-Resident Command Driver
IPD.INC                 include file to define BD32 function calls for
                        Target-Resident Command Drivers
BDM-IF.SCH              Orcad (TM) schematic of the parallel interface cct
BDM-IF.PRN              Printer-ready version of schematic
READ.ME                 This file

The file BDM-IF.PRN is provided for those users without access to the
Orcad (TM) schematic editor program.  This file contains the printer
output from Orcad, formatted for Epson-compatible printers.  To create
a hard copy of the schematic, use the PC-DOS COPY command to dump the
file to your printer.

This is free software, and it was written as a spare-time project.  As
such, it is given away freely, but there is no facility for Motorola or myself
to be able to provide support to users.  If you need help, I will try to do my
best to support you, but please understand that the amount of time for this work
is limited and may only be available at irregular intervals.  If any problems
with this software are found,  please report these to me as you discover them,
and I will correct them in future releases.

Current Software Update Status
==============================

Version 1.22 includes the following bug fixes & changes:

- 'LO' command (without filename on command line) no longer trashs the filename
entered by the user.

- Printer port is re-initialized properly when returning from DOS shell
(DOS command) and is restored to its original state when BD32 terminates
and exits to DOS.

- symbol table size increased to 1000 symbols.

- User documentation ('BD32.DOC') clarifies the use of the delay parameter
in the 'PORT LPTx <n>' command, for fast processors like 486's and Pentiums
(Pentia?).

Scott Howard
Motorola Semiconductor Products, Canada
Vancouver, B.C., Canada
POST:           SCN088
Internet:       Howard-SCN088_Scott@amail.mot.com
Fax:            +1-604-241-6290
May 1994
