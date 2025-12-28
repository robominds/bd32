*****************************************************************************
* 'prog' Resident Command Driver for Orion
* Source file: 'prog.s'
* Object file: 'prog.p32'
* Object file format: Motorola S-records
* execute as: prog <filename> [<start_address> [<end_address>]]
* Beta Version 0.1
* Scott Howard, February 1992
* programming algorithm written and tested by Mark Maiolani, Motorola EKB
*****************************************************************************

                opt     nol
                include ipd.inc
                include orion.inc
                opt     l
VERSION         equ     1

UsageError      equ     1                       Usage: ...
FileError       equ     2                       can't open file...
EvalError1      equ     3                       error evaluating start address
EvalError2      equ     4                       error evaluating end address
SRecError       equ     4                       starting value for SRec errors
SRecEOFError    equ     5                       reached EOF on input file
SRecS9Error     equ     6                       S9 read (not an error)
SRecChecksum    equ     7                       checksum error in record
SRecFormat      equ     8                       format error in S-record file
ProgError       equ     9                       error programming data

SRecS9          equ     2                       S9 Record - end of file

ErasedValue     equ     $ffff                   erased state of EEPROM
array1          equ     $4000                   hardwire array addresses
array2          equ     $c000

* Vector table to start driver

                dc.l    Prog                    main line
buffer          ds.b    40                      space for S-record from host
addr_list       ds.l    10                      EEPROM addresses
                ds.l    20                      Stack area
stack
MinAddress      dc.l    0                       min address (default 0)
MaxAddress      dc.l    -1                      max address (default $FFFFFFFF)
FilePtr         ds.l    1                       file pointer
FileName        ds.b    64                      file name
Error           ds.w    1                       error code

*****************************************************************************
* Prog in mainline routine for PROG command
* psuedocode
* ==========

* initialize
* get options
* open input file
* for (;;)
*       read S record
*       if (error)
*               exit for
*       for (i = 0 to byte count)
*               if (count == 1) or (address is odd)
*                       program byte
*                       subtract 1 from count
*               else
*                       program word
*                       subtract 2 from count
*               endif
*       end for
* end for
* close file
* done

FileMode        dc.b    'r',0                   read mode for file open

* start of driver - print signon message and initialize

Prog            lea.l   stack(A5),a7            set up stack
                move.l  a0,a2                   get argv into a2
                move.l  d0,d2                   get argc into d2
                bsr     Print                   print signon message
                dc.b    'Orion Flash EEPROM Programer Version '
                dc.b    '0'+VERSION,13,10,0
                even
                bsr     Initialize              init hardware and address list
                tst     d0
                bne     Prog_end

* get options from command line and open file

                cmpi    #2,d2                   argc < 2?
                bcs     Prog_0
                cmpi    #4,d2                   argc > 4?
                bls     Prog_1

Prog_0          move    #UsageError,Error(A5)   arg count is wrong
                bra     Prog_end

* get filename, open file, check if OK
Prog_1          addq.l  #4,a2                   skip over program name
                move.l  (a2)+,a0                get file name of S records
                lea.l   FileMode(A5),a1         read mode - "r"
                bsr     fopen
                move.l  d0,FilePtr(A5)          save file pointer
                bne     Prog_11                 continue if OK
                move    #FileError,Error(A5)    can't open input file
                bra     Prog_end

* if third parameter on command line, evaluate it for min address

Prog_11         cmpi    #3,d2                   argv >= 3 ?
                bcs     ProgStart
                move.l  (a2)+,a0                evaluate start address parameter
                bsr     Eval
                beq     Prog_12
                move    #EvalError1,Error(A5)
                bra     Prog_3                  close file and exit
Prog_12         move.l  d1,MinAddress(A5)       got first param

* if fourth parameter on command line, evaluate it for max address

                cmpi    #4,d2                   argc == 4?
                bne     ProgStart
                move.l  (a2)+,a0                evaluate start address parameter
                bsr     Eval
                beq     Prog_13
                move    #EvalError2,Error(A5)
                bra     Prog_3
Prog_13         move.l  d1,MaxAddress(A5)       got end address

* perform blank check on EEPROM memory

ProgStart       bsr     BlankCheck              EEPROM blank ?
                tst     d0
                bne     Prog_end                no - bail out

* read an S-Record, check for errors

Prog_2          bsr     ReadSRecord             get next S Record
                tst     d0
                beq     Prog_25                 continue if no error
                cmpi    #SRecS9,d0              S9 record ?
                beq     Prog_3                  yes - close normally
                addi    #SRecError,d0           otherwise flag error
                move    d0,Error(A5)
                bra     Prog_3

* program data from S-Record into EEPROM

Prog_25         bsr     ProgRecord              program data from S Record
                tst     d0
                beq     Prog_2                  loop till done
                move    #ProgError,Error(A5)    error - report it

* close input file
       
Prog_3          bsr     CloseInputFile          close file

* report any errors, exit back to BD32

Prog_end        move    Error(A5),d1            get error code
                moveq.l #BD_QUIT,d0             exit program
                bgnd

*****************************************************************************
* ReadSRecord - reads one S record from FilePtr
* returns result in D0

ReadSRecord     move.l  FilePtr(A5),d1          file pointer
                lea.l   buffer(A5),a0           point to S Record buffer
                moveq.l #BD_FREADSREC,d0
                bgnd
                rts                             done

*****************************************************************************
* CloseInputFile closes FilePtr
* does not affect Error

CloseInputFile  move.l  FilePtr(A5),d0
                moveq.l #BD_FCLOSE,d0
                bgnd
                rts

*****************************************************************************
* Eval evaluates string passed in a0
* returns result in D1, error flag in D0

Eval            moveq.l #BD_EVAL,d0
                bgnd
                tst     d0
                rts

*****************************************************************************
* fopen performs file open routine
* filename pointer in A0
* file mode pointer in A1
* returns file pointer in D0

fopen           moveq.l #BD_FOPEN,d0            file open
                bgnd
                rts

*****************************************************************************
* FindStrEnd searches the ASCII string pointed to by A0 for the end of
* string marker (a '0' byte)
* returns a0 pointing to that marker

FindStrEnd      move.w  d0,-(a7)                push temp register
                moveq   #-1,d0                  max loop count 1st time thru
FSE_1           tst.b   (a0)+                   byte == 0?
                dbeq    d0,FSE_1                uses loop mode
                bne     FSE_1                   loop till test true
			 subq.l  #1,a0                   decrement address reg.
                move.w  (a7)+,d0                restore register
                rts

*****************************************************************************
* ntoh prints hex value of register D0 least sig nibble to screen

ntoh            movem.l d0/d1,-(a7)
                move.b  d0,d1
                andi.w  #$f,d1
                addi.b  #'0',d1
                cmpi.b  #10+'0',d1
                bcs     nt_1
                addi.b  #'A'-'9'-1,d1
nt_1            moveq   #BD_PUTCHAR,d0
                bgnd
                movem.l (a7)+,d0/d1
                rts

*****************************************************************************
* btoh prints hex value of byte register D0 to screen

btoh            ror.b   #4,d0
                bsr     ntoh
                ror.b   #4,d0
                bsr     ntoh
                rts

*****************************************************************************
* wtoh prints hex value of word register D0 to screen

wtoh            ror.w   #8,d0
                bsr     btoh
                ror.w   #8,d0
                bsr     btoh
                rts

*****************************************************************************
* ltoh prints hex value of long word register D0 to screen

ltoh            swap    d0
                bsr     wtoh
                swap    d0
                bsr     wtoh
                rts

*****************************************************************************
* Print prints constant string in code
* returns to program at first even location after string

Print           movem.l a0/d0,-(a7)             save registers
                move.l  8(a7),a0                get address of string
                moveq.l #BD_PUTS,d0             function call
                bgnd
                bsr     FindStrEnd              get end of ASCII string
                move.l  a0,d0                   test for odd address
                addq.l  #1,d0                   skip past end of string
                btst    #0,d0
                beq     Print_1
                addq.l  #1,d0                   it's odd - return to next addr
Print_1         move.l  d0,8(a7)                put address on stack
                movem.l (a7)+,d0/a0             get back registers
                rts                             done

*****************************************************************************
* crlf prints carriage return, line feed combo

crlf            bsr     Print                   carriage return, line feed
                dc.b    13,10,0
                even
                rts

******************************************************************************
* getchar returns character typed by user

getchar         moveq.l #BD_GETCHAR,d0
                bgnd
                rts

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
                lea.l   addr_list(A5),a2        point to buffer area
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
ca_3            lea.l   0,a1                    no good - return null pointer
ca_4            movem.l (a7)+,a2/d0

debug_ch
                rts

*******************************************************************************
*   <do_prog> : Programs one byte/word of data to memory
*               Entry - Target address in A0
*                       data in D0
*                       byte flag in d5 (non-zero => program byte data)
*               Exit  - d0 contains difference between data to be programmed
*                       and data which actually resides in target location
*                       a0 and d5 are unchanged
*
* This is a dummy routine which displays the address and value being programmed
*******************************************************************************
Tcum            ds.w    1                       cumulative program time
Tpulse          ds.w    1                       programming voltage pulsewidth

do_prog         bsr     check_address           get register address
                moveq   #0,d7                   debug - D7 is 'try' counter

                tst.l   a1                      address OK?
                beq     pb_fail                 no - bomb out

                clr     Tcum                    initialise cum. time = 0
                move    #tpmin,Tpulse           set tpulse = 10 usec
                andi.w  #$7fff,FEEMCR(a1)       make sure module not stopped

                move    #$8,FEECTL(a1)          enable verify : set VFPE
                move    #$a,FEECTL(a1)          enable latch  : set LAT

                tst     d5                      byte or word?
                beq     pb_w1
                move.b  d0,(a0)                 write byte data to EEPROM
                bra     pb_1
pb_w1           move.w  d0,(a0)                 write word data ro EEPROM

pb_1            addq    #$1,d7
                move    Tpulse,d1               get pulse time
                add     d1,Tcum                 Tcum += Tpulse

                move    #$b,FEECTL(a1)          enable prog voltage : set ENPE
                bsr     usdelay                 wait (Tpulse) microseconds

                move    #$a,FEECTL(a1)          disable voltage : clear ENPE
                move    #tvprog,d1              delay after turning off vprog
                bsr     usdelay

                tst     d5                      byte or word?
                beq     pb_w2
                tst.b   (a0)                    all bits programmed ?
                bra     pb_w25
pb_w2           tst.w   (a0)                    all bits programmed ?
pb_w25          beq     pb_2
                move    Tcum,d1                 no - increase prog time by Tcum/4
                asr     #2,d1
                add     d1,Tpulse
                cmpi    #tmax,Tcum              over max time ?
                bls     pb_1                    no - continue
                clr     FEECTL(a1)              yes - turn off prog. voltage
                bra     pb_getdata              return programmed data to caller

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
                bra     pb_getdata              return programmed data to caller

pb_fail         moveq   #-1,d1
                bra     pb_end

pb_getdata      clr.l   d1
pb_end          tst     d5
                beq     pb_w3
                move.b  (a0),d1                 get faulty data
                eor.b   d1,d0
                rts
pb_w3           move.w  (a0),d1
                eor.w   d1,d0
                rts

*****************************************************************************
*   <Initialize> : initialise routine is called by BD32 before any programming
*
*               - initialize list of valid EEPROM address in addr_list
*               - initialize global variables
*
*               returns non-zero in D0 if can't continue with programming
*
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
* Most of this routine written and tested by Mark Maiolani, Motorola EKB
*****************************************************************************

rangeno         equ     $04

Initialize      lea.l   addr_list(A5),a1        ready to fill buffer
                move.l  #rangeno,(a1)+          start with no. of ranges

*               ARRAY 1 ranges
defrange1       move.l  #FER_1,(a1)+            start of registers
                move.l  #FER_1+FER_REGSZ-1,(a1)+  end of registers
                move.l  #array1,d0              read array1 start address
                move.l  d0,(a1)+                ..put into addr_list
                add.l   #FEE_SIZE_1-1,d0 calculate end addresses
                move.l  d0,(a1)+                ..put into addr_list

*               ARRAY 2 ranges
defrange2       move.l  #FER_2,(a1)+            repeat for array 2...
                move.l  #FER_2+FER_REGSZ-1,(a1)+
                move.l  #array2,d0
                move.l  d0,(a1)+
                add.l   #FEE_SIZE_2-1,d0
                move.l  d0,(a1)+

* now initialize globals

                clr.l   d0                      set up for return
                move    d0,Error(A5)            no error
                rts                             done - return no error

*****************************************************************************
* BlankCheck checks EEPROM array for all one's
* if any non-blank locations found, asks user if OK to continue
* returns -1 in D0 if he says no
* if all is OK, returns 0 in D0
*****************************************************************************

* first check range 1

BlankCheck      lea.l   addr_list+12(A5),a1     don't check registers
                move.l  (a1)+,a0
                move.l  a0,d1                   get start in d1
                neg.l   d1
                add.l   (a1)+,d1                calc end-start = byte count
                addq.l  #1,d1                   adjust count
                bsr     bl_chk                  blank check range
                bne     BlCk_1                  exit if error
                addq.l  #8,a1                   step past second register set
                move.l  (a1)+,a0                get second address
                move.l  a0,d1                   get start in d1
                neg.l   d1
                add.l   (a1)+,d1                calc end-start = byte count
                addq.l  #1,d1                   adjust count
                bsr     bl_chk
                beq     BlCk_2                  return if OK
BlCk_1          bsr     not_blank               ask if OK
BlCk_2          rts

******************************************************************************
* bl_chk does blank check of EEPROM starting at address in A0
* byte count is in D1 - must be even

bl_chk:         move    #ErasedValue,d0         erased value of EEPROM
                subq.w  #1,d1                   set up for dbcc loop
bc_1:           cmp     (a0)+,d0
                dbne    d1,bc_1                 loop while equal
                bne     bc_2
                cmpi.l  #$ffff,d1               end of loop?
                bne     bc_1                    loop till done
                clr.l   d0                      signal blank check OK
bc_2            rts

*****************************************************************************
* ProgRecord programs data from S-record buffer into EEPROM
* loops through the record, retrieving each byte/word and programming it
* compares address against MinAddress and MaxAddress and excludes those
* which fall outside of this range
*
* entry parameters: none; assumes S Record is in 'buffer'
* on exit:      d0 is difference between data and EEPROM location
*               (this will be 0 if programmed successfully)
*               a0 will contain address at which program failed
*               d5 will be non-zero if byte program, 0 if word program
*****************************************************************************

ProgRecord      movem.l a1/a2/d6,-(a7)          save working registers
                lea.l   buffer(A5),a2           point to S-record buffer
                clr.l   d6
                move.b  (a2)+,d6                get record type
                beq     prog_done               record type 0 - bail out
                cmpi.b  #7,d6
                bcs     prog_start              record 1,2,3 - start programming
                bra     prog_done               end of file - don't program
prog_start      move.b  (a2)+,d6                get byte count from s-record
                subi.b  #4,d6                   remove byte count due to address
                move.l  (a2)+,a0                get address
prog_1          cmpa.l  MinAddress(A5),a0       are we >= min ?
                bcs     prog_25                 no - loop
                cmpa.l  MaxAddress(A5),a0       are we <= max ?
                bhi     prog_25
                move.l  a0,d5                   put address in d5
                andi.l  #1,d5                   mask all but bit 0
                bne     prog_2                  program byte if odd address
                cmpa.l  MaxAddress(A5),a0       are we programming last address?
                beq     prog_2                  byte program if so
                cmpi    #1,d6                   count == 1?
                bne     prog_3                  word program if not

* program byte data if address is odd or byte count is 1

prog_2          moveq   #1,d5                   flag byte write
                move.b  (a2),d0                 byte - get data
                bsr     do_prog                 program byte/word
                bne     prog_done                no - exit loop
prog_25         addq.l  #1,a0                   increment target address
                addq.l  #1,a2                   increment buffer address
                subq    #1,d6                   dec byte count
                bne     prog_1                  loop till byte count = 0
                bra     prog_done               otherwise done

* program word data if address is even and byte count not equal to 1

prog_3          move.w  (a2),d0                 byte - get data
                bsr     do_prog                 program byte/word
                bne     prog_done                no - exit loop
                addq.l  #2,a0                   increment target address
                addq    #2,a2                   increment buffer address
                subq    #2,d6                   dec byte count
                bne     prog_1                  loop till byte count = 0
prog_done       movem.l (a7)+,a1/a2/d6          restore registers
                rts                             done

******************************************************************************
* not_blank informs user of blank check error
* user enters escape to stop, any other key to continue programming

not_blank:      bsr     Print
                dc.b    'EEPROM not blank: address $',0
                even
                move.l  a0,d0                   print address
                bsr     ltoh
                bsr     Print
                dc.b    ', data $',0
                even
                move    (a0),d0                 print data at that address
                bsr     wtoh
                bsr     Print
                dc.b    13,10,'Press <esc> to stop, any other to continue:',0
                even
                bsr     getchar
                move    d0,-(a7)                save char
                bsr     crlf
                move    (a7)+,d0                get char
                andi    #$ff,d0
                cmpi    #$1b,d0                 escape?
                seq     d0                      make d0 nonzero if so
                tst     d0                      set SR for subsequent test
                rts
                end
