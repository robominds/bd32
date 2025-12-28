* ramregs.s - define addresses of RAM registers in 68332
* adjust the setting of rammcr to reflect modmap bit

rammcr  equ     $fffffb00       ram module configuration register
ramtst  equ     rammcr+2        ram module test register
rambar  equ     rammcr+4        ram base address register
