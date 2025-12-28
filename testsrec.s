* testsrec.s - test s-record read function

		opt 		nol
		include   ipd.inc
                include   orion.inc
		opt       l

* offset to start of driver

        dc.l    start

s9rec   equ     2                       code returned to indicate eof record
userr   equ     1                       usage error code
openerr equ     2                       file open error
lasterr equ     2                       add this to s-record error

start   move.l  a0,a1                   put argv in a1
        move.l  d0,d2                   get argc in d2
        lea.l   signon(a5),a0           print signon message
        move.l  #BD_PUTS,d0
        bgnd
        cmpi    #1,d2                   any arguments?
        bne     argfound
        move.l  #1,d1                   no - quit with error code
quit    moveq.l #BD_QUIT,d0
        bgnd
argfound
        addq.l  #4,a1                   discard command name
        move.l  #BD_FOPEN,d0            open the file
        move.l  (a1),a0                 point to file name string
        lea.l   filemode(a5),a1         point to file mode string ("r")
        bgnd
        move.l  d0,filehandle(a5)       save file handle
        bne     do_load                 OK if handle non-zero
        move.l  #openerr,d1             no - error code
        bra     quit

* read S-record into memory till error, then closes file and returns

do_load lea.l   retbuff(a5),a6          point to buffer recording return codes
        clr     rcnt(a5)                counter = 0
        lea     readbuff(a5),a0
loop    moveq.l #BD_FREADSREC,d0        read S-record
        move.l  filehandle(a5),d1       get file handle
        bgnd
        addi    #1,rcnt(a5)             increment record count
        move.l  d0,(a6)+                store return code
        bne     close
        cmpi.b  #1,readbuff(a5)         S1 record?
        bne     loop                    no - continue
close   move.l  filehandle(a5),d1       now close the file
        move.l  #BD_FCLOSE,d0
        bgnd
        tst     d0                      close OK?
        beq     done
        move.l  d0,d1                   no - return result of close
done    move.l  -4(a6),d1               yes - get result of last read
        cmpi    #s9rec,d1               end of file?
        bne     done1                   no - return the error
        clr     d1                      yes - ignore it
        bra     quit
done1   addq    #lasterr,d1             fix up error code
        bra     quit

signon  dc.b    'S record tester'
CRLF    dc.b    13,10,0
filemode
        dc.b    'r',0
        even
filehandle
        ds.l    1
rcnt    ds.w    1                       count of records read
readbuff
        ds.b    50                      room for the S-Record
retbuff
* buffer for return codes from each read
        end
