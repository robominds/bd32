* simregs.s - define addresses of SIM registers in 68331/332
* adjust the setting of simmcr to reflect modmap bit

simmcr  equ     $fffffa00       sim module configuratio register
simtr   equ     simmcr+2        sim test register
syncr   equ     simmcr+4        clock synthesizer control
rsr     equ     simmcr+7        reset status register
simtre  equ     simmcr+8        sim module test e
porte   equ     simmcr+$11      port e data register
ddre    equ     simmcr+$15      port e data direction register
pepar   equ     simmcr+$17      port e pin assignment register
portf   equ     simmcr+$19      port f data register
ddrf    equ     simmcr+$1d      port f data direction register
pfpar   equ     simmcr+$1f      port f pin assignment register
sypcr   equ     simmcr+$21      system protection control
picr    equ     simmcr+$22      periodic interrupt control
pitr    equ     simmcr+$24      periodic interrupt timer
swsr    equ     simmcr+$27      watchdog software service register
tstmsra equ     simmcr+$30      test master shift register a
tstmsrb equ     simmcr+$32      test master shift register b
tstsc   equ     simmcr+$34      test shift count register
tstrc   equ     simmcr+$36      test repetition count register
creg    equ     simmcr+$38      test module control
dreg    equ     simmcr+$3a      test distributed control
portc   equ     simmcr+$41      port c data register
cspar0  equ     simmcr+$44      chip select pin assignment register 0
cspar1  equ     simmcr+$46      chip select pin assignment register 1
csbarbt equ     simmcr+$48      CSBOOT base address register
csorbt  equ     simmcr+$4a      CSBOOT option register
csbar0  equ     simmcr+$4c      chip select 0 base address register
csor0   equ     simmcr+$4e      chip select 0 option register
csbar1  equ     simmcr+$50      chip select 0 base address register
csor1   equ     simmcr+$52      chip select 0 option register
csbar2  equ     simmcr+$54      chip select 0 base address register
csor2   equ     simmcr+$56      chip select 0 option register
csbar3  equ     simmcr+$58      chip select 0 base address register
csor3   equ     simmcr+$5a      chip select 0 option register
csbar4  equ     simmcr+$5c      chip select 0 base address register
csor4   equ     simmcr+$5e      chip select 0 option register
csbar5  equ     simmcr+$60      chip select 0 base address register
csor5   equ     simmcr+$62      chip select 0 option register
csbar6  equ     simmcr+$64      chip select 0 base address register
csor6   equ     simmcr+$66      chip select 0 option register
csbar7  equ     simmcr+$68      chip select 0 base address register
csor7   equ     simmcr+$6a      chip select 0 option register
csbar8  equ     simmcr+$6c      chip select 0 base address register
csor8   equ     simmcr+$6e      chip select 0 option register
csbar9  equ     simmcr+$70      chip select 0 base address register
csor9   equ     simmcr+$72      chip select 0 option register
csbar10 equ     simmcr+$74      chip select 0 base address register
csor10  equ     simmcr+$76      chip select 0 option register
