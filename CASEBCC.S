* casebcc.s - test assembly/downloading to 68332 BCC

immed   equ     5
anyea	equ	$1234
index   equ     100

bd      equ     $1234           base displacement
od      equ     $5678           outer displacement
abs     equ     $7ABC           absolute address
longabs equ     $13579BDF       long absolute address

	section 1
        org     $5000

*       Effective address tests

	tst     d0
	tst     a0
	tst     (a0)
	tst     (a0)+
	tst     -(a0)
	tst     index(a0)
	tst     (index,a0)
	tst     abs.w
	tst     abs.l
	tst     #immed
	tst     index+*(PC)
	tst     (index+*,PC)
*
*       68020 instruction set tests
*
	section         0
        org             $4000
	abcd.b          d0,d0
	abcd            d0,d0
	abcd.b          -(a0),-(a0)
	abcd            -(a0),-(a0)
	add.b           #immed,(a4)
	add.w           #immed,(a4)
	add.l           #immed,(a4)
	add             #immed,(a4)
	add.w           anyea,d0
	add.l           anyea,d0
	add             anyea,d0
	add.b           (a6),d0
	add.b           d0,(a2)
	add.w           d0,(a2)
	add.l           d0,(a2)
	add             d0,(a2)
	adda            anyea,a0
	adda.w          anyea,a0
	adda.l          anyea,a0
	addi.b          #immed,(a4)
	addi.w          #immed,(a4)
	addi.l          #immed,(a4)
	addi            #immed,(a4)
	addq.b          #immed,(a5)
	addq.w          #immed,(a5)
	addq.l          #immed,(a5)
	addq            #immed,(a5)
	addx.b          d0,d0
	addx.w          d0,d0
	addx.l          d0,d0
	addx            d0,d0
	addx.b          -(a0),-(a0)
	addx.w          -(a0),-(a0)
	addx.l          -(a0),-(a0)
	addx            -(a0),-(a0)
	and.b           (a6),d0
	and.w           (a6),d0
	and.l           (a6),d0
	and             (a6),d0
	and.b           d0,(a2)
	and.w           d0,(a2)
	and.l           d0,(a2)
	and             d0,(a2)
	and.b           #immed,(a4)
	and.w           #immed,(a4)
	and.l           #immed,(a4)
	and             #immed,(a4)
	and.b           #immed,CCR
	and             #immed,CCR
	and.w           #immed,SR
	and             #immed,SR
	andi.b          #immed,(a4)
	andi.w          #immed,(a4)
	andi.l          #immed,(a4)
	andi            #immed,(a4)
	andi.b          #immed,CCR
	andi            #immed,CCR
	andi.w          #immed,SR
	andi            #immed,SR
	asl.w           (a2)
	asl             (a2)
	asl.b           d0,d0
	asl.w           d0,d0
	asl.l           d0,d0
	asl             d0,d0
	asl.b           #immed,d0
	asl.w           #immed,d0
	asl.l           #immed,d0
	asl             #immed,d0
	asr.w           (a2)
	asr             (a2)
	asr.b           d0,d0
	asr.w           d0,d0
	asr.l           d0,d0
	asr             d0,d0
	asr.b           #immed,d0
	asr.w           #immed,d0
	asr.l           #immed,d0
	asr             #immed,d0
	section         0
bcclabel
	bcc.b           bcclabel
	bcc.w           bcclabel
	bcc.l           bcclabel
	bcc             bcclabel
	bcs             bcclabel
	beq             bcclabel
	bge             bcclabel
	bgt             bcclabel
	bhi             bcclabel
	ble             bcclabel
	bls             bcclabel
	blt             bcclabel
	bmi             bcclabel
	bne             bcclabel
	bpl             bcclabel
	bvc             bcclabel
	bvs             bcclabel
	bra             bcclabel
	bsr             bcclabel
	bchg.l          d0,d0
	bchg            d0,d0
	bchg.l          #immed,d0
	bchg            #immed,d0
	bchg            d0,(a0)
	bchg            #immed,(a0)
	bclr            d0,d0
	bclr            #immed,d0
	bclr            d0,(a0)
	bclr            #immed,(a0)
	bgnd
	bset            d0,d0
	bset            #immed,d0
	bset            d0,(a0)
	bset            #immed,(a0)
	btst            d0,d0
	btst            #immed,d0
	btst            d0,(a6)
	btst            #immed,(a6)
*
	bkpt            #immed
	chk.w           (a6),d0
	chk.l           (a6),d0
	chk             (a6),d0
	chk2.b          (a0),a0
	chk2.w          (a0),a0
	chk2.l          (a0),a0
	chk2            (a0),a0
	chk2.b          (a0),d0
	chk2.w          (a0),d0
	chk2.l          (a0),d0
	chk2            (a0),d0
	clr.b           (a4)
	clr.w           (a4)
	clr.l           (a4)
	clr             (a4)
	cmp.b           #immed,(a6)
	cmp.w           #immed,(a6)
	cmp.l           #immed,(a6)
	cmp             #immed,(a6)
	cmp.w           anyea,d0
	cmp.l           anyea,d0
	cmp             anyea,d0
	cmp.b           (a6),d0
	cmpa.w          anyea,a0
	cmpa.l          anyea,a0
	cmpa            anyea,a0
	cmpi.b          #immed,(a6)
	cmpi.w          #immed,(a6)
	cmpi.l          #immed,(a6)
	cmpi            #immed,(a6)
	cmpm.b          (a0)+,(a0)+
	cmpm.w          (a0)+,(a0)+
	cmpm.l          (a0)+,(a0)+
	cmpm            (a0)+,(a0)+
	cmp2.b          (a0),a0
	cmp2.w          (a0),a0
	cmp2.l          (a0),a0
	cmp2            (a0),a0
	cmp2.b          (a0),d0
	cmp2.w          (a0),d0
	cmp2.l          (a0),d0
	cmp2            (a0),d0
	section         1
dbcclabel
	dbcc            d0,dbcclabel
	dbcs            d0,dbcclabel
	dbeq            d0,dbcclabel
	dbf             d0,dbcclabel
	dbge            d0,dbcclabel
	dbgt            d0,dbcclabel
	dbhi            d0,dbcclabel
	dble            d0,dbcclabel
	dbls            d0,dbcclabel
	dblt            d0,dbcclabel
	dbmi            d0,dbcclabel
	dbne            d0,dbcclabel
	dbpl            d0,dbcclabel
	dbt             d0,dbcclabel
	dbvc            d0,dbcclabel
	dbvs            d0,dbcclabel
	dbra            d0,dbcclabel
	divs.w          (a6),d0
	divs            (a6),d0
	divs.l          (a6),d0
	divs.l          (a6),d0:D1
	divsl.l         (a6),d0:D1
	divsl           (a6),d0:D1
	divu.w          (a6),d0
	divu            (a6),d0
	divu.l          (a6),d0
	divu.l          (a6),d0:D1
	divul.l         (a6),d0:D1
	divul           (a6),d0:D1
	eor.b           d0,(a4)
	eor.w           d0,(a4)
	eor.l           d0,(a4)
	eor             d0,(a4)
	eor.b           #immed,(a4)
	eor.w           #immed,(a4)
	eor.l           #immed,(a4)
	eor             #immed,(a4)
	eor.b           #immed,CCR
	eor             #immed,CCR
	eor.w           #immed,SR
	eor             #immed,SR
	eori.b          #immed,(a4)
	eori.w          #immed,(a4)
	eori.l          #immed,(a4)
	eori            #immed,(a4)
	eori.b          #immed,CCR
	eori            #immed,CCR
	eori.w          #immed,SR
	eori            #immed,SR
	exg.l           d0,d0
	exg             d0,d0
	exg.l           a0,a0
	exg             a0,a0
	exg.l           d0,a0
	exg             d0,a0
	exg.l           a0,d0
	exg             a0,d0
*
	exg             d0,d1
	exg             d0,a1
	exg             a0,d1
	exg             a0,a1
*
	ext.w           d0
	ext             d0
	ext.l           d0
	extb.l          d0
	extb            d0
	illegal
	jmp             (a0)
	jsr             (a0)
	lea.l           (a0),a0
	lea             (a0),a0
	link.w          a0,#immed
	link            a0,#immed
	link.l          a0,#immed
	lpstop		#immed
	lsl.w           (a2)
	lsl             (a2)
	lsl.b           d0,d0
	lsl.w           d0,d0
	lsl.l           d0,d0
	lsl             d0,d0
	lsl.b           #immed,d0
	lsl.w           #immed,d0
	lsl.l           #immed,d0
	lsl             #immed,d0
	lsr.w           (a2)
	lsr             (a2)
	lsr.b           d0,d0
	lsr.w           d0,d0
	lsr.l           d0,d0
	lsr             d0,d0
	lsr.b           #immed,d0
	lsr.w           #immed,d0
	lsr.l           #immed,d0
	lsr             #immed,d0
	move.w          anyea,(a4)
	move            anyea,(a4)
	move.b          (a6),(a4)
	move.l          anyea,(a4)
	move.w          anyea,a0
	move            anyea,a0
	move.l          anyea,a0
	move.w          CCR,(a4)
	move            CCR,(a4)
	move.w          (a6),CCR
	move            (a6),CCR
	move.w          SR,(a4)
	move            SR,(a4)
	move.w          (a6),SR
	move            (a6),SR
	move.l          usp,a0
	move            usp,a0
	move.l          a0,usp
	move            a0,usp
	movea.w         anyea,a0
	movea           anyea,a0
	movea.l         anyea,a0
	movec.l         sfc,a0
	movec           dfc,a0
	movec.l         vbr,d0
	movec.l         a0,usp
	movem.w         a0-a3,-(a0)
	movem           a0-a3,-(a0)
	movem.l         a0-a3,-(a0)
	movem.w         (a0)+,a0-a3
	movem           (a0)+,a0-a3
	movem.l         (a0)+,a0-a3
	movem           a0-a7/d0-d7,-(sp)
	movem           a7-a0/d7-d0,-(sp)
	movem           (sp)+,a0-a7/d0-d7
	movem           (sp)+,a7-a0/d7-d0
	movem           a0/a2/a4/a6/d0/d2/d4/d6,(sp)
	movem           d3,-(sp)
	movem           (sp)+,d3
	movem           a0-a3/d0-d3,(sp)
	movem           a0-a3/d0-d3,-(sp)
	movep.w         d0,index(a0)
	movep           d0,index(a0)
	movep.l         d0,index(a0)
	movep.w         index(a0),d0
	movep           index(a0),d0
	movep.l         index(a0),d0
	moveq.l         #immed,d0
	moveq           #immed,d0
	moves.b         a0,(a2)
	moves.w         a0,(a2)
	moves.l         a0,(a2)
	moves           a0,(a2)
	moves.b         d0,(a2)
	moves.w         d0,(a2)
	moves.l         d0,(a2)
	moves           d0,(a2)
	moves.b         (a2),a0
	moves.w         (a2),a0
	moves.l         (a2),a0
	moves           (a2),a0
	moves.b         (a2),d0
	moves.w         (a2),d0
	moves.l         (a2),d0
	moves           (a2),d0
	muls.w          (a6),d0
	muls            (a6),d0
	muls.l          (a6),d0
	muls.l          (a6),d0:D1
	mulu.w          (a6),d0
	mulu            (a6),d0
	mulu.l          (a6),d0
	mulu.l          (a6),d0:D1
	nbcd.b          (a4)
	nbcd            (a4)
	neg.b           (a4)
	neg.w           (a4)
	neg.l           (a4)
	neg             (a4)
	negx.b          (a4)
	negx.w          (a4)
	negx.l          (a4)
	negx            (a4)
	nop
	not.b           (a4)
	not.w           (a4)
	not.l           (a4)
	not             (a4)
	or.b            (a6),d0
	or.w            (a6),d0
	or.l            (a6),d0
	or              (a6),d0
	or.b            d0,(a2)
	or.w            d0,(a2)
	or.l            d0,(a2)
	or              d0,(a2)
	or.b            #immed,(a4)
	or.w            #immed,(a4)
	or.l            #immed,(a4)
	or              #immed,(a4)
	or.b            #immed,CCR
	or              #immed,CCR
	or.w            #immed,SR
	or              #immed,SR
	ori.b           #immed,(a4)
	ori.w           #immed,(a4)
	ori.l           #immed,(a4)
	ori             #immed,(a4)
	ori.b           #immed,CCR
	ori             #immed,CCR
	ori.w           #immed,SR
	ori             #immed,SR
	pea.l           (a0)
	pea             (a0)
	reset
	rol.w           (a2)
	rol             (a2)
	rol.b           d0,d0
	rol.w           d0,d0
	rol.l           d0,d0
	rol             d0,d0
	rol.b           #immed,d0
	rol.w           #immed,d0
	rol.l           #immed,d0
	rol             #immed,d0
	ror.w           (a2)
	ror             (a2)
	ror.b           d0,d0
	ror.w           d0,d0
	ror.l           d0,d0
	ror             d0,d0
	ror.b           #immed,d0
	ror.w           #immed,d0
	ror.l           #immed,d0
	ror             #immed,d0
	roxl.w          (a2)
	roxl            (a2)
	roxl.b          d0,d0
	roxl.w          d0,d0
	roxl.l          d0,d0
	roxl            d0,d0
	roxl.b          #immed,d0
	roxl.w          #immed,d0
	roxl.l          #immed,d0
	roxl            #immed,d0
	roxr.w          (a2)
	roxr            (a2)
	roxr.b          d0,d0
	roxr.w          d0,d0
	roxr.l          d0,d0
	roxr            d0,d0
	roxr.b          #immed,d0
	roxr.w          #immed,d0
	roxr.l          #immed,d0
	roxr            #immed,d0
	rtd             #immed
	rte
	rtr
	rts
	sbcd.b          d0,d0
	sbcd            d0,d0
	sbcd.b          -(a0),-(a0)
	sbcd            -(a0),-(a0)
	scc.b           (a4)
	scc             (a4)
	scs             (a4)
	seq             (a4)
	sf              (a4)
	sge             (a4)
	sgt             (a4)
	shi             (a4)
	sle             (a4)
	sls             (a4)
	slt             (a4)
	smi             (a4)
	sne             (a4)
	spl             (a4)
	st              (a4)
	svc             (a4)
	svs             (a4)
	stop            #immed
	sub.b           #immed,(a4)
	sub.w           #immed,(a4)
	sub.l           #immed,(a4)
	sub             #immed,(a4)
	sub.w           anyea,d0
	sub.l           anyea,d0
	sub             anyea,d0
	sub.b           (a6),d0
	sub.b           d0,(a2)
	sub.w           d0,(a2)
	sub.l           d0,(a2)
	sub             d0,(a2)
	suba.w          anyea,a0
	suba.l          anyea,a0
	suba            anyea,a0
	subi.b          #immed,(a4)
	subi.w          #immed,(a4)
	subi.l          #immed,(a4)
	subi            #immed,(a4)
	subq.b          #immed,(a5)
	subq.w          #immed,(a5)
	subq.l          #immed,(a5)
	subq            #immed,(a5)
	subx.b          d0,d0
	subx.w          d0,d0
	subx.l          d0,d0
	subx            d0,d0
	subx.b          -(a0),-(a0)
	subx.w          -(a0),-(a0)
	subx.l          -(a0),-(a0)
	subx            -(a0),-(a0)
	swap.w          d0
	swap            d0
	tas.b           (a4)
	tas             (a4)
	trap            #immed
	trapcc
	trapcc.w        #immed
	trapcc.l        #immed
	trapcs
	trapcs.w        #immed
	trapcs.l        #immed
	trapeq
	trapeq.w        #immed
	trapeq.l        #immed
	trapf
	trapf.w         #immed
	trapf.l         #immed
	trapge
	trapge.w        #immed
	trapge.l        #immed
	trapgt
	trapgt.w        #immed
	trapgt.l        #immed
	traphi
	traphi.w        #immed
	traphi.l        #immed
	traple
	traple.w        #immed
	traple.l        #immed
	trapls
	trapls.w        #immed
	trapls.l        #immed
	traplt
	traplt.w        #immed
	traplt.l        #immed
	trapmi
	trapmi.w        #immed
	trapmi.l        #immed
	trapne
	trapne.w        #immed
	trapne.l        #immed
	trappl
	trappl.w        #immed
	trappl.l        #immed
	trapt
	trapt.w         #immed
	trapt.l         #immed
	trapvc
	trapvc.w        #immed
	trapvc.l        #immed
	trapvs
	trapvs.w        #immed
	trapvs.l        #immed
	trapv
	tst.w           anyea
	tst.l           anyea
	tst             anyea
	tst.b           (a6)
	unlk            a0
*
	dc.b	1,2,3,4,5,6,7,8,9,0,1,2,3
	dc.b	1,2,3,4,5,6,7,8,9,0,1,2
	dc.b	1,2,3,4,5,6,7,8,9,0,1
	dc.b	1,2,3,4,5,6,7,8,9,0
	dc.b	1,2,3,4,5,6,7,8,9
	dc.b	1,2,3,4,5,6,7,8
	dc.b	1,2,3,4,5,6,7
	dc.b	1,2,3,4,5,6
	dc.b	1,2,3,4,5
	dc.b	1,2,3,4
	dc.b	1,2,3
	dc.b	1,2
	dc.b	1
*
	dc.w    1,2,3,4,5,6,7,8,9,0,1,2,3
	dc.w    1,2,3,4,5,6,7,8,9,0,1,2
	dc.w    1,2,3,4,5,6,7,8,9,0,1
	dc.w    1,2,3,4,5,6,7,8,9,0
	dc.w    1,2,3,4,5,6,7,8,9
	dc.w    1,2,3,4,5,6,7,8
	dc.w    1,2,3,4,5,6,7
	dc.w    1,2,3,4,5,6
	dc.w    1,2,3,4,5
	dc.w    1,2,3,4
	dc.w    1,2,3
	dc.w    1,2
	dc.w    1

	dc.l    1,2,3,4,5,6,7,8,9,0,1,2,3
	dc.l    1,2,3,4,5,6,7,8,9,0,1,2
	dc.l    1,2,3,4,5,6,7,8,9,0,1
	dc.l    1,2,3,4,5,6,7,8,9,0
	dc.l    1,2,3,4,5,6,7,8,9
	dc.l    1,2,3,4,5,6,7,8
	dc.l    1,2,3,4,5,6,7
	dc.l    1,2,3,4,5,6
	dc.l    1,2,3,4,5
	dc.l    1,2,3,4
	dc.l    1,2,3
	dc.l    1,2
	dc.l    1

*	end
	nam
	name
	opt     nol             listing off
*
*       this shouldn't be listed
*
	opt     l               turn listing back on
	pag
	page
	spc
	ttl
	even
	even
	ds.b    5
	ds.w    5
	ds.l    5
	ds      5

	section 2
        org     $6000
	dc.b    1

	section 0
	section 1
	section 2
	section 2
	section immed-4
	dc.b    'string'
	dc.w    'string'
	dc.l    'string'
junk    equ     $12345678

        even

        tst.l   (za0.w*2)
	tst.l	(10,a1.l*1)
	tst.l	(a1.l*1)
	tst.l	(pc,d0)
	tst.l	(pc,zd0)
*	These should be the brief format.
	tst	-5(a2,d1.w)
	tst	*-18(pc,d1.w)
	move.b	-5(a2,d1.w),(a1)
	move.b	*-18(pc,d1.w),(a1)
        end
