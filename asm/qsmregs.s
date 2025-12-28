* qsmregs.s - define addresses of registers in Queued Serial Module
* adjust the value of qmcr to reflect setting of modmap bit in sim mcr

qmcr    equ     $fffffc00       use full 32-bit address for sign extended addresses
qtest   equ     qmcr+2          qsm test register
qilr    equ     qmcr+4          qsm interrupt level register
qivr    equ     qmcr+5          qsm interrupt vector register
sccr0   equ     qmcr+6          sci control register 0
sccr1   equ     qmcr+8          sci control register 1
scsr    equ     qmcr+$a         sci status register
scdr    equ     qmcr+$c         sci data register
qpdr    equ     qmcr+$15        qsm port data register
qpar    equ     qmcr+$16        qsm pin assignment register
qddr    equ     qmcr+$17        qsm data direction register
spcr0   equ     qmcr+$18        qspi control register 0
spcr1   equ     qmcr+$1a        qspi control register 1
spcr2   equ     qmcr+$1c        qspi control register 2
spcr3   equ     qmcr+$1e        qspi control register 3
spsr    equ     qmcr+$20        qspi status register
qrxd    equ     qmcr+$100       qspi receive data buffer start
qtxd    equ     qmcr+$120       qspi transmit data buffer start
qcmd    equ     qmcr+$140       qspi command buffer start
