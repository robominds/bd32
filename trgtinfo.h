/* Trgtinfo.h - define TargetInfo structure */

#ifndef	TRGTINFO_H
#define	TRGTINFO_H

#include	"didefs.h"

struct TargetInfo
{
	char *DeviceName;
	int DriverInstalled;
	int CurrentPort;
	int CurrentSpeed;
	int MaxBreakpoints;
	unsigned StatMask;
	unsigned (*ValidPorts) (void);
	int (*Init) (unsigned Port, unsigned Speed);
	void (*DeInit) (void);
	unsigned (*GetStatus) (void);
	int (*BR) (int which, LONG where, LONG mask);
	int (*NoBR) (int which);
	LONG (*GetBR) (int which);
	LONG (*GetMask) (int which);
	void (*HitBreakpoint) (LONG where);
	int (*StopChip) (void);
	void (*StepChip) (void);
	void (*ResetChip) (void);
	void (*RestartChip) (void);
	void (*RunChip) (LONG where);
	LONG (*GetByte) (LONG Where);
	LONG (*GetWord) (LONG Where);
	LONG (*GetLong) (LONG Where);
	LONG (*DumpByte) (LONG Where);
	LONG (*DumpWord) (LONG Where);
	LONG (*DumpLong) (LONG Where);
	void (*PutByte) (LONG Where, BYTE Data);
	void (*PutWord) (LONG Where, WORD Data);
	void (*PutLong) (LONG Where, LONG Data);
	void (*FillByte) (LONG Where, BYTE Data);
	void (*FillWord) (LONG Where, WORD Data);
	void (*FillLong) (LONG Where, LONG Data);
	LONG (*GetReg) (unsigned which);
	void (*PutReg) (unsigned which, LONG Data);
};

#endif
