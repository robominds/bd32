/* logfile.h -defines structure used to log program output to disk */

#pragma once

#define	LOGBUFFLEN	256

struct LogBuffer {
	int BuffUsed;
	char stuff [LOGBUFFLEN];
	};
