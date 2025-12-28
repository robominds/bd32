/* target.h - define function calls to target interface */

#include	"trgtinfo.h"

void show_settings (void);
void DriverError (char *Message);
struct TargetInfo *port_initted (void);
void deinit_serial (void);
int init_serial (char *name, int port, int speed);
int GetPortInfo (char *name, int *port, int *speed);
int BreakpointCount (void);
unsigned get_sigstatus (void);
unsigned get_statmask (void);
int set_fc (void);
int restore_fc (void);
int match_bp (LONG where);
int br (int which, LONG where, LONG mask);
int nobr (int which);
LONG getbr (int which);
LONG getmask (int which);
int stop_chip (void);
void step_chip (void);
void reset_chip (void);
void restart_chip (void);
void run_chip (LONG where);
void trash_cache (void);
void enable_cache (void);
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
LONG GETREG (unsigned regname);
void PUTREG (unsigned regname, LONG data);

/* end of target.h */
