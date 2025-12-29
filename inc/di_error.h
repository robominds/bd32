/* di_error.h - error handler for DOS critical error function */

#pragma once

extern void (*abort_func) ();

/* Error window and message display */
char error_win(char *msg);

/* DOS critical error handler */
int handler(int errval, int ax, int bp, int si);
