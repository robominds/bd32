* chck.s - orion Flash EEPROM blank driver
* execute under VERF command in BD32
* Beta Version 0.1
* Scott Howard, July 1991

		opt 		nol
		include   ipd.inc
		include   orion.inc
		opt       l

VERSION equ         1

* Vector table to start driver

	   org     $4000

	   dc.l        do_init            	initialize
	   dc.l        buffer
	   dc.l        do_chck
	   dc.l		get_byte

* check_address returns address (in a1) of register set
* corresponding to EEPROM address passed in a0

check_address
		movem.l	a2/d0,-(a7)	 	push working reg for now
		lea		buffer,a2	   		point to buffer
		clr		d0
ca_1		cmpa.l	4(a2,d0.w*4),a0	within range?
		bcs		ca_2				nope - try next
		cmpa.l	8(a2,d0.w*4),a0	test high end of range
		bhi		ca_2				too high - try next
		andi		#2,d0			clear upper bits
		move.l	4(a2,d0.w*4),a1	get pointer to register
		bra		ca_4
ca_2		addq		#2,d0			next entry in table
		cmpi		#8,d0			done all four?
		bne		ca_1				no - loop till done
ca_3		lea		0,a1				no good - return null pointer
ca_4		movem.l	(a7)+,a2/d0
		rts

* do_chck does the comparision of the byte of data to memory

do_chck	move.l	#stack,a7			set up stack
		bsr		check_address		get register address
		tst.l	a1				address OK?
		bne		vr_0				no - bomb out
		move		#1,d1			flag address error
		bra		vr_1
vr_0		cmp.b	(a0),d0            	all bits ok ?
		sne		d1				register d1 contains result (0 = match)
vr_1		move.l	#BD_QUIT,d0
		bgnd

* get_byte returns the byte of EEPROM addressed by a0
* only run if do_chck returns non-zero (error); no need to check addresses

get_byte	clr.l	d1
		move.b	(a0),d1			very simple in this case
		move.l	#BD_QUIT,d0
		bgnd

do_init	move		#template,a0		move template into buffer
		move		#buffer,a1
		moveq	#loopcnt,d0
iloop	move.l	(a0)+,(a1)+		do the move
		dbf		d0,iloop			... quickly!
		move.l	FEEBAH+FER_1,d0
		move.l	d0,memory
		add.l	#FEE_SIZE_1-1,d0
		move.l	d0,memory+4
		move.l	FEEBAH+FER_2,d0
		move.l	d0,memory+8
		add.l	#FEE_SIZE_2-1,d0
		move.l	d0,memory+12
		move		#-1,memory+16
		move.l	#BD_QUIT,d0
		clr.l	d1
		bgnd
template	dc.l		4,FER_1,FER_1+FER_REGSZ-1,FER_2,FER_2+FER_REGSZ-1
loopcnt	equ		((*-template)/4)

buffer	ds.b		256
buff1    	equ		buffer+4
memory	equ		buffer+20
		ds.l		20
stack
		end
