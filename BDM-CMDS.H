/* bdm-cmds.h - define BDM commands for CPU32 */

#define         BDM_RDREG       0x2180
#define         BDM_WRREG       0x2080
#define         BDM_RSREG       0x2580
#define         BDM_WSREG       0x2480
#define         BDM_READ        0x1900
#define         BDM_WRITE       0x1800
#define         BDM_DUMP        0x1D00
#define         BDM_FILL        0x1C00
#define         BDM_GO          0x0C00
#define         BDM_CALL        0x0800
#define         BDM_RST         0x0400
#define         BDM_NOP         0x0000

#define         BDM_BYTESIZE    0x00
#define         BDM_WORDSIZE    0x40
#define         BDM_LONGSIZE    0x80
#define         BDM_RDBIT       0x100

#define         BDM_RPC         0x0
#define         BDM_PCC         0x1
#define         BDM_SR          0xb
#define         BDM_USP         0xc
#define         BDM_SSP         0xd
#define         BDM_SFC         0xe
#define         BDM_DFC         0xf
#define         BDM_ATEMP       0x8
#define         BDM_FAR         0x9
#define         BDM_VBR         0xa

#define         BDM_NOTREADY    0x10000L
#define         BDM_BERR        0x10001L
#define         BDM_ILLEGAL     0x1FFFFL
#define         BDM_CMDCMPLTE   0x0FFFFL

/* end of bdm-cmds.h */
