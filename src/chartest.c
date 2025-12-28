/* chartest.c - test embedding of long data in character arrays */

#include	<stdio.h>

#define	DUP	"\x1"
#define	DROP	"\x2"
#define	SWAP	"\x3"
#define	END	"\0"

char TestArray [] = {
"\x3f"
"\x40"
DUP SWAP DROP
"This is the Real Thing!" DUP SWAP DROP END
/* (char) ((unsigned long) TestArray >> 24),
(char) ((unsigned long) TestArray >> 16),
(char) ((unsigned long) TestArray >> 8),
(char) TestArray,
'\0'*/ };

#pragma argsused
int main (int argc, char **argv)
{
	char *ArrayPtr = TestArray;

	printf ("Array Contents:");
	while (*ArrayPtr) printf (" %02X", *ArrayPtr++);
	printf ("\n");
	return 0;
}
