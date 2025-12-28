* tpuregs.s - define addresses of TPU registers in 68332
* adjust the setting of tpumcr to reflect modmap bit

tpumcr  equ     $fffffe00       tpu module control register
tpucfg  equ     tpumcr+2        configuration register
dscr    equ     tpumcr+4        development support control register
dssr    equ     tpumcr+6        development support status register
tpuicr  equ     tpumcr+8        interrupt configuration register
tpuier  equ     tpumcr+$a       interrupt enable register
cfsr0   equ     tpumcr+$c       channel function select register 0
cfsr1   equ     tpumcr+$e       channel function select register 1
cfsr2   equ     tpumcr+$10      channel function select register 2
cfsr3   equ     tpumcr+$12      channel function select register 3
hsr0    equ     tpumcr+$14      host sequence register 0
hsr1    equ     tpumcr+$16      host sequence register 1
hsrr0   equ     tpumcr+$18      host service request register 0
hsrr1   equ     tpumcr+$1a      host service request register 1
cpr0    equ     tpumcr+$1c      channel priority register 0
cpr1    equ     tpumcr+$1e      channel priority register 1
tpuisr  equ     tpumcr+$20      interrupt status register
link    equ     tpumcr+$22
sglr    equ     tpumcr+$24      service grant latch register
dcnr    equ     tpumcr+$26      decoded channel number register
tpupram equ     tpumcr+$100     start of tpu parameter RAM
