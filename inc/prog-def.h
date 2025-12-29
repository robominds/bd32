/* prog_def.h - definitions for programming drivers */

#pragma once

struct prog_cmd
{
	char *name, *description;
	struct prog_cmd *next;
};

#define	PTR_SIZE		4			/* how many bytes per entry in table */
#define	INT_SIZE		4			/* how many bytes for int in target */
#define	INIT_OFFSET	0			/* init pointer location */
#define	BUFFER_OFFSET	1*PTR_SIZE	/* buffer pointer location within table */
#define	RUN_OFFSET	2*PTR_SIZE	/* command execution pointer */

#define	RANGE_OFFSET	0			/* range offset in buffer */
#define	FIRST_RANGE	1*INT_SIZE	/* first range in buffer */

/* define a structure for easy access to S-record info */

#define	SREC_MAXCOUNT		32

typedef struct {
	int rectype;
	int reclen;
	LONG address;
	char bytes [SREC_MAXCOUNT];
} srecord;

/* end */
