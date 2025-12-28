/* dasmdefs.h - define column numbers for asm/dasm screen display */

#define DASM_WORDS      3       /* number of words of code to show in disassembly */
#define DASM_SYMBOLSIZE 9       /* largest symbol to show on same line as code */
#define DASM_ADDRSIZE   8       /* size of address field */

#define DASM_SYM_COLUMN         (2 + DASM_ADDRSIZE+1+(DASM_WORDS*5))        /* column # where symbol is placed */
#define DASM_DASM_COLUMN        (2 + DASM_SYM_COLUMN+DASM_SYMBOLSIZE+2)     /* column # where disassembled code is placed */

/* end of dasmdefs.h */
