/* gpdefine.h - define calls into get/put tables */

#define GETMEM(x)       ((*getmem[opsize]) (x))
#define PUTMEM(x,y)     ((*putmem[opsize]) (x,y))
#define FILLMEM(x,y)	((*fillmem[opsize]) (x,y))
#define PRINTHEX(x)     ((*printhex[opsize]) (x))

/* end of gpdefine.h */
