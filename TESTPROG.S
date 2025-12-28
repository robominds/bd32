*****************************************************************************
* 'Orion' Flash EEPROM programming driver
* Source file: 'testprog.s'
* Object file: 'prog.p32'
* Object file format: Motorola S-records
* execute under PROG command in BD32
* Beta Version 0.1
* Scott Howard, February 1992
* Most of the tricky bits written and tested by Mark Maiolani, Motorola EKB
*****************************************************************************

                opt     nol
                include ipd.inc
                include orion.inc
                opt     l
VERSION         equ     1

                org     $4000
* Vector table to start driver
                dc.l    do_init                 initialize
buffer          ds.b    256                     Define pass buffer
addr_list       ds.l    10                      EEPROM addresses
addr_size       equ     (*-addr_list)/4         count of long words in list

                ds.l    20                      Stack area
stack

* some data for do_prog

Tcum            ds.w    1                       cumulative programming time
Tpulse          ds.w    1                       current pulse width

*****************************************************************************
* <usedelay> : generates delay of (d1) microseconds
*              legal values are 2 ... 65535
*              Note - all timings assume 2 clock accesses
*****************************************************************************

*               jsr     usdelay                 13
usdelay         subq    #2,d1                   2 - adjust for overhead
                asl     #1,d1                   6 - multiply count by 2 for us
loop            tst     d1                      2
                dbf     d1,loop                 6

debug_us
                rts                             12

*****************************************************************************
* <check_address> : searches through address ranges in <addr_list> to find
*                   which array is being accessed, and hence which set
*                   of control registers to use. The data in <addr_list> must
*                   be arranged as described in <do_init>.
*                   Entry : A0 = address to be programmed
*                   Exit  : A1 = start address of register bank
*                                or 0 if address not valid
*****************************************************************************

check_address   movem.l a2/d0,-(a7)             push working reg for now
                lea     addr_list,a2            point to buffer area
                clr     d0
ca_1            cmpa.l  4(a2,d0.w*4),a0         within range?
                bcs     ca_2                    nope - try next
                cmpa.l  8(a2,d0.w*4),a0         test high end of range
                bhi     ca_2                    too high - try next
                andi    #$fc,d0                 clear upper bits
                move.l  4(a2,d0.w*4),a1         get pointer to register
                bra     ca_4
ca_2            addq    #2,d0                   next entry in table
                cmpi    #8,d0                   done all four?
                bne     ca_1                    no - loop till done
ca_3            lea     0,a1                    no good - return null pointer
ca_4            movem.l (a7)+,a2/d0

debug_ch
                rts

*****************************************************************************
*   <do_prog> : Programs one byte/word of data to memory
*               Entry - Target address in A0
*                       data in D0
*                       byte flag in d5 (non-zero means byte data)
*               Exit  - Verify read data in D1
*****************************************************************************

do_prog         bsr     check_address           get register address
                moveq   #0,d7                   debug - D7 is 'try' counter

                tst.l   a1                      address OK?
                beq     pb_0                    no - bomb out

                clr     Tcum                    initialise cum. time = 0
                move    #tpmin,Tpulse           set tpulse = 10 usec
                andi.w  #$7fff,FEEMCR(a1)       make sure module not stopped

                move    #$8,FEECTL(a1)          enable verify : set VFPE
                move    #$a,FEECTL(a1)          enable latch  : set LAT

                tst     d5                      byte or word?
                beq     pb_00
                move.b  d0,(a0)                 write byte data to EEPROM
                bra     pb_1
pb_00           move.w  d0,(a0)                 write word data to EEPROM

pb_1            addq    #$1,d7
                move    Tpulse,d1               get pulse time
                add     d1,Tcum                 Tcum += Tpulse

                move    #$b,FEECTL(a1)          enable prog voltage : set ENPE
                bsr     usdelay                 wait (Tpulse) microseconds

                move    #$a,FEECTL(a1)          disable voltage : clear ENPE
                move    #tvprog,d1              delay after turning off vprog
                bsr     usdelay

                tst     d5                      byte or word?
                beq     pb_15
                tst.b   (a0)                    test for match
                bra     pb_17
pb_15           tst.w   (a0)                    test for match
pb_17           beq     pb_2
                move    Tcum,d1                 no - increase prog time by Tcum/4
                asr     #2,d1
                add     d1,Tpulse
                cmpi    #tmax,Tcum              over max time ?
                bls     pb_1                    no - continue
                clr     FEECTL(a1)              yes - turn off programming
                clr.l   d1
                tst     d5                      byte or word?
                beq     pb_18
                move.b  (a0),d1                 get faulty data
                bra     pb_end
pb_18           move.w  (a0),d1                 get faulty data

* programmed OK - now program a little more to give us some margin

pb_2            move    Tpulse,d1               prog time is Tpulse * margin value
                ext.l   d1                      make 32 bits
                mulu.l  #marg_val,d1            fractional multiply
                swap    d1                      ... so get integer part
                move    #$b,FEECTL(a1)          set ENPE

                bsr     usdelay                 ... and program for that long

                move    #$a,FEECTL(a1)          clear ENPE
                move    #tvprog,d1              delay a while after turning off Vprog
                bsr     usdelay

                clr     FEECTL(a1)
                clr.l   d1
                tst     d5                      byte or word?
                beq     pb_25
                move.b  (a0),d1                 get value programmed
                rts
pb_25           move.w  (a0),d1                 get value programmed
pb_end          rts

pb_0            moveq   #-1,d1
                bra     pb_end

*****************************************************************************
*   <do_init> : initialise routine is called by BD32 before any programming
*               This routine sets up the area <addr_list> with a list of all
*               valid flash EEPROM address ranges. These ranges consist of
*               the fixed address register blocks and the moveable arrays
*               defined by the flash registers FEEBAH/L.
*
*               The format is:-
*               n             (number of ranges to follow, long word)
*               saddr1        (start address of range 1, long word)
*               eaddr1        (end     ,,    ,,   ,,  1, long word)
*                 .
*               saddrn        (start address of range n, long word)
*               eaddrn        (end     ,,    ,,   ,,  n, long word)
*               $xx           (value of erased byte)
*
*               The ranges must be in the order:- register range 1, array
*               range 1 ... reg. range n, array range n
*               This allows the programming software to easily determine
*               which register bank should be used to program a particular
*               array
*****************************************************************************

do_init         move.l  #stack,a7               set up stack
                movea   #addr_list,a1           ready to fill buffer
                move.l  #rangeno,(a1)+          start with no. of ranges

*               ARRAY 1 ranges
defrange1       move.l  #FER_1,(a1)+            start of registers
                move.l  #FER_1+FER_REGSZ-1,(a1)+  end of registers
                move.l  FEEBAH+FER_1,d0         read array1 start address
                move.l  d0,(a1)+                ..put into addr_list
                add.l   #FEE_SIZE_1-1,d0        calculate end addresses
                move.l  d0,(a1)+                ..put into addr_list

*               ARRAY 2 ranges
defrange2       move.l  #FER_2,(a1)+            repeat for array 2...
                move.l  #FER_2+FER_REGSZ-1,(a1)+
                move.l  FEEBAH+FER_2,d0
                move.l  d0,(a1)+
                add.l   #FEE_SIZE_2-1,d0
                move.l  d0,(a1)+

* address list constructed - now move it into buffer

                lea     buffer,a0
                lea     addr_list,a1
                moveq   #addr_size,d0
init_2          move.l  (a1)+,(a0)+             fast move to buffer
                dbf     d0,init_2

* initialization is done - now blank check
* first range 1
                lea.l   addr_list+8,a1          don't check registers
                move.l  (a1)+,a0
                move.l  (a1)+,d1                byte count
                bsr     bl_chk                  blank check range
                bne     init_1                  exit if error
                lea.l   8(a1),a1                step past second register set
                move.l  (a1)+,a0                get second address
                move.l  (a1)+,d1                ... and byte count
                bsr     bl_chk
init_1:         move.l  d0,d1                   put result in d1
                moveq   #BD_QUIT,d0             return result to BD32
                bgnd

* if execution gets to this point, then we have an S-record to program
* loop through the record, retrieving each byte/word and programming it
* record format is same as defined for FREADSREC system call
* we don't need to test for S-record type
* since BD32 will do this automatically ('prog' and 'verf' commands only)

prog:           lea.l   buffer+1,a2             point to S-record buffer
                clr.l   d6                      init byte count
                move.b  (a2)+,d6                get byte count from s-record
                move.l  (a2)+,a0                get address
prog_1          move.l  a0,d5                   put address in d5
                andi.l  #1,d5                   mask all but bit 0
                bne     prog_2                  address even?
                cmpi    #1,d6                   its even - count == 1?
                bne     prog_3

* program byte data if address is odd or byte count is 1

prog_2          moveq   #1,d5                   flag byte write
                move.b  (a2),d0                 byte - get data
                bsr     do_prog                 program byte/word
                move.b  (a2),d3
                eor.b   d3,d1                   programmed ok?
                bne     prog_10                 no - exit loop
                addq.l  #1,a0                   increment target address
                addq    #1,a2                   increment buffer address
                subq    #1,d6                   dec byte count
                bne     prog_1                  loop till byte count = 0
                bra     prog_10                 otherwise done

* program word data if address is even and byte count not equal to 1

prog_3          move.w  (a2),d0                 byte - get data
                bsr     do_prog                 program byte/word
                move.w  (a2),d3
                eor.w   d3,d1                   programmed ok?
                bne     prog_10                 no - exit loop
                addq.l  #2,a0                   increment target address
                addq    #2,a2                   increment buffer address
                subq    #2,d6                   dec byte count
                bne     prog_1                  loop till byte count = 0

prog_10         moveq   #BD_QUIT,d0
                bgnd
                bra     prog                    do next record

* bl_chk does blank check of EEPROM starting at address in A0
* byte count is in D1 - must be even

bl_chk:         move    #$ffff,d0               erased value
bc_1:           cmp     (a0)+,d0
                dbeq    d1,bc_1                 loop while equal
                beq     bc_2
                bsr     not_blank               not blank - inform user
bc_2            rts

not_blank:      movem.l a0/d1,-(a7)             save registers
                lea.l   nb_msg,a0               ask if ok to continue
                moveq   #BD_PUTS,d0
                bgnd
                lea.l   getbuff,a0              get response
                moveq   #BD_GETS,d0
                bgnd
                lea.l   crlf,a0
                moveq   #BD_PUTS,d0
                bgnd
                lea.l   getbuff,a0              get first char
                bsr     skipwhite               skip white space
                move.b  (a0),d0
                bsr     toupper                 make upper case
                subi.b  #'Y',d0
                movem.l (a7)+,d1/a0             get registers back
                rts

skipwhite       cmpi.b  #' ',(a0)               character == space ?
                beq     sw_1
                cmpi.b  #8,(a0)                 no - is it a tab ?
                beq     sw_1
                rts                             no - done
sw_1            addq    #1,a0                   increment pointer
                bra     skipwhite

toupper         cmpi.b  #'a',d0                 is D0 lower case ?
                bcs     tu_1                    no - lower than 'a'
                cmpi.b  #'z',d0
                bhi     tu_1
                addi.b  #'a'-'A',d0
tu_1            rts

nb_msg          dc.b    'EEPROM not blank - continue? (Y/N)'
                dc.b    0
crlf            dc.b    13,10,0
getbuff         ds.b    128

rangeno         equ     $04
                end
