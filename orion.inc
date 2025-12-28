* orion.inc - define orion addresses

FER_1       equ      $FFFFF800      registers for array #1 - 16k bytes
FEE_SIZE_1  equ      16384
FER_2       equ      $FFFFF820      registers for array #2 - 48k bytes
FEE_SIZE_2  equ      49152
FER_REGSZ	  equ	 $20			size of register block

* register offsets

FEEMCR      equ      0              mod config register
FEETST      equ      2              test register
FEEBAH      equ      4              base address reg - high word
FEEBAL      equ      6              base address reg - low word
FEECTL      equ      8              program control reg
FEEBS0      equ      $10            bootstrap info 0
FEEBS1      equ      $12            bootstrap info 1
FEEBS2      equ      $14            bootstrap info 2
FEEBS3      equ      $16            bootstrap info 3

* bit assignments

STOP        equ      $8000
FRZ         equ      $4000
BOOT        equ      $1000
LOCK        equ      $800
ASPC1       equ      $200
ASPC0       equ      $100
WAIT1       equ      $80
WAIT0       equ      $40

FSTE        equ      $80
GADR        equ      $40
HVT         equ      $20
BTST        equ      $10
STRE        equ      2
MWPF        equ      1

VFPE        equ      8
ERAS        equ      4
LAT         equ      2
ENPE        equ      1

* flash EEPROM programming constants

tpmin       equ      10             min pulse width for programming (usec)
temin       equ      10             min pulse width for erasing (usec)
tvprog      equ      10             min wait period after turning off Vprog
tvbulk	  equ      10             min wait period after turning off Vprog
tmax        equ      100            max prog. time per location
marg_val    equ      $20000         factor to mult. times tpulse
*                                   for extra prog time after verify
*                                   radix point on marg_val is between bit 15/16
marg_delay  equ      10             usec delay between locations in EEPROM

* end of orion.inc
