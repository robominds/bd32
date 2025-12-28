/* gptable.h - define getmem/putmem/printhex routines as externals */

extern LONG (*getmem[]) (LONG);
extern void (*putmem[]) (LONG,LONG);
extern void (*printhex[]) (LONG);

/* end of gptable.h */
