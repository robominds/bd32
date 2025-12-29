/* trgtstat.h - define target status bits */

#pragma once

#define	TARGETRESET	1
#define	TARGETHALT	2
#define	TARGETSTOPPED	4
#define	TARGETINVALID	8
#define	TARGETCHANGED	0x40
#define	TARGETBRKPT	0x80

/* end of trgtstat.h */
