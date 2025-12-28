; bdm-io.asm - assembly language routines to access MC683XX MCU
; through Background Debug port connected to LPT1 or LPT2

		.MODEL small,c
		.data
		public  frozen

bdm_port        dw      ?               ; base address of LPT port
frozen          db      ?               ; non-zero means we need to un-freeze
clockrate		 dw	    ?			; clock rate for serial access to BDM

bdm_ctl         equ     2               ; offset of control port from base
step_out        equ     8               ; set low to gate IFETCH onto BKPT
dsi             equ     4               ; data shift input - PC->MCU
rst_out         equ     2               ; set high to force reset on MCU
reset           equ     2               ; low when RESET asserted
dsclk           equ     1               ; data shift clock/breakpoint pin

lpt1            equ     8               ; offset of printer table in bios RAM
bdm_stat        equ     1               ; offset of status port from base
nc              equ     80h             ; not connected - low when unplugged
pwr_dwn         equ     40h             ; power down - low when Vcc failed
dso             equ     20h             ; data shift output - MCU->PC
freeze          equ     8               ; FREEZE asserted when MCU stopped

waitcnt         equ     0ffffh          ; no of loops to wait for response
go_cmd          equ     0c00h           ; GO command
nop_cmd         equ     0               ; null (NOP) command

rst_stat        equ     2               ; return status - reset asserted
frz_stat        equ     1               ; return status - freeze asserted

bios_seg        equ     40h             ; bios data segment

		.CODE

		include callf.inc

		extrn   c bdm_error:proc

; bdm_error codes

fault_unknown   equ     0               ; unknown bdm fault
fault_power     equ     1               ; target power failed
fault_cable     equ     2               ; target cable disconnected
fault_response  equ     3               ; no response to breakpoint
fault_reset     equ     4               ; can't clock MCU when in reset
fault_port      equ     5               ; port not installed

; bdm_init sets up selected parallel port to control bdm interface
; port should be 0 (LPT1) or 1 (LPT2)
; returns fault code if something not right, or 0 if OK

bdm_init	PROC    uses dx es si,port:word,speed:word
			 public  bdm_init
		mov	   ax,speed	    ; set up clock rate variable
		mov	   clockrate,ax
		mov     si,port
		and     si,1            ; clear junk just in case
		shl     si,1
		add     si,lpt1
		mov     dx,bios_seg
		mov     es,dx
		mov     dx,es:[si]      ; set base address of port
		or      dx,dx           ; is port installed?
		jne     init_1          ; it's non-zero - go ahead
		mov     ax,fault_port   ; port not installed
		ret

init_1:	mov     bdm_port,dx
		add     dx,bdm_ctl      ; point to control port
		mov     al,step_out     ; clear all outputs to 0 except step_out
		out     dx,al
		callf   bdm_getstat
		mov	   ax,0		    ; everything OK - return 0
		ret                     ; done
bdm_init        ENDP

; bdm-stop stops MCU in preparation for sending command

bdm_stop	PROC    USES ax cx dx
			 public  bdm_stop
		mov     frozen,0        ; set flag
		mov     dx,bdm_port     ; get base address of port
		add     dx,bdm_stat     ; point to status port
		in      al,dx
		test    al,freeze       ; FREEZE asserted ?
		jnz     stop_2          ; yes - nothing to do
		mov     frozen,1        ; else assert BKPT and wait
		mov     al,dsclk+step_out
		inc     dx
		out     dx,al
		dec     dx
		mov     cx,waitcnt
stop_1:         in      al,dx
		test    al,freeze       ;
		jnz     stop_2          ; loop till FREEZE asserted
		mov     ax,1            ; else delay
		call    delay
		loop    stop_1
		mov     al,step_out     ; deassert clock
		inc     dx
		out     dx,al
		mov     ax,fault_response
		push    ax
		call    bdm_error       ; bail out - no response
stop_2:         mov     ax,nop_cmd      ; now clock null command
		push    ax
		callf   bdm_clock
		add     sp,2
		ret
bdm_stop        ENDP

; bdm_clk sends 0's to MCU for <parameter> bits, returns MCU response
; the return value is a long in dx:ax (17 lsb's significant)
; this routine is used mainly to re-sync after error detected

bdm_clk	proc    uses bx cx,count:word
			 public  bdm_clk
		callf   bdm_getstat     ; look at reset line
		test    al,rst_stat     ; reset asserted?
		jz      ck__0           ; no - continue
		mov     ax,fault_reset  ; can't clock when reset asserted
		push    ax
		call    bdm_error
ck__0:          mov     dx,bdm_port     ; get i/o base address
		inc     dx              ; point to status port
		mov     bx,0            ; outgoing command word is all zeroes
		mov     ah,0            ; ah is msb
		mov     cx,count        ; number of bits to send
ck__1:          in      al,dx           ; get data from MCU
		test    al,dso          ; isolate it
		jnz     ck__2           ; branch if bit is 0
		stc                     ; otherwise shift a 1 into dl
ck__2:          rcl     bx,1            ; get bit into dl:0
		rcl     ah,1            ; get next output bit into carry
		mov     al,dsclk+step_out       ; strobe out next bit
		jnb     ck__3
		or      al,dsi          ; output a one
ck__3:          inc     dx              ; point at output port
		out     dx,al           ; output it
		push    ax              ; delay
		mov     ax,clockrate
		inc     ax
		call    delay
		pop     ax
		and     al,not dsclk    ; toggle clock line
		out     dx,al
		push    ax              ; delay
		mov     ax,clockrate
		inc     ax
		call    delay
		pop     ax
		dec     dx              ; point back at status port
		loop    ck__1           ; repeat until all bits clocked
		mov     dl,ah           ; put data 23:16 in al
		mov     dh,0            ; clear msb
		mov     ax,bx           ; put return value in ax
		ret                     ; done
bdm_clk         endp

; bdm_clock sends command word, returns MCU response
; the return value is a long in dx:ax (17 lsb's significant)

bdm_clock proc    uses bx cx,cmd:word
			 public  bdm_clock
		callf   bdm_getstat     ; look at reset line
		test    al,rst_stat     ; reset asserted?
		jz      clk_0           ; no - continue
		mov     ax,fault_reset  ; can't clock when reset asserted
		push    ax
		call    bdm_error
clk_0:          mov     dx,bdm_port     ; get i/o base address
		inc     dx              ; point to status port
		mov     bx,cmd          ; get outgoing command word
		mov     cx,17           ; 17 bits to send
		mov     ah,bh           ; ah is msb
		mov     bh,bl           ; bx is nsb:lsb
		mov     bl,0
		clc                     ; make sure outgoing bit 16 is 0
		rcr     ah,1
		rcr     bx,1
clk_1:          in      al,dx           ; get data from MCU
		test    al,dso          ; isolate it
		jnz     clk_2           ; branch if bit is 0
		stc                     ; otherwise shift a 1 into dl
clk_2:          rcl     bx,1            ; get bit into dl:0
		rcl     ah,1            ; get next output bit into carry
		mov     al,dsclk+step_out       ; strobe out next bit
		jnb     clk_3
		or      al,dsi          ; output a one
clk_3:          inc     dx              ; point at output port
		out     dx,al           ; output it
		push    ax              ; delay
		mov     ax,clockrate
		inc     ax
		call    delay
		pop     ax
		and     al,not dsclk    ; toggle clock line
		out     dx,al
		push    ax              ; delay
		mov     ax,clockrate
		inc     ax
		call    delay
		pop     ax
		out     dx,al
		dec     dx              ; point back at status port
		loop    clk_1           ; repeat until all bits clocked
		mov     dl,ah           ; put data 23:16 in al
		mov     dh,0            ; clear msb
		mov     ax,bx           ; put return value in ax
		ret                     ; done
bdm_clock       endp

; bdm_step sends GO command word, then triggers breakpoint on first fetch

bdm_step	proc    uses bx cx
                public  bdm_step
		callf   bdm_getstat     ; look at reset line
		test    al,rst_stat     ; reset asserted?
		jz      stp_0           ; no - continue
		mov     ax,fault_reset  ; can't clock when reset asserted
		push    ax
		call    bdm_error
stp_0:          mov     dx,bdm_port     ; get i/o base address
		inc     dx              ; point to status port
		mov     bx,go_cmd       ; get outgoing command word
		mov     cx,16           ; 17 bits to send - last one separate
		mov     ah,bh           ; ah is msb
		mov     bh,bl           ; bx is nsb:lsb
		mov     bl,0
		clc                     ; make sure outgoing bit 16 is 0
		rcr     ah,1
		rcr     bx,1
stp_1:          in      al,dx           ; get data from MCU
		test    al,dso          ; isolate it
		jnz     stp_2           ; branch if bit is 0
		stc                     ; otherwise shift a 1 into dl
stp_2:          rcl     bx,1            ; get bit into dl:0
		rcl     ah,1            ; get next output bit into carry
		mov     al,dsclk+step_out       ; strobe out next bit
		jnb     stp_3
		or      al,dsi          ; output a one
stp_3:          inc     dx              ; point at output port
		out     dx,al           ; output it
		push    ax              ; delay
		mov     ax,clockrate
		inc     ax
		call    delay
		pop     ax
		and     al,not dsclk    ; toggle clock line
		out     dx,al
		push    ax              ; delay
		mov     ax,clockrate
		inc     ax
		call    delay
		pop     ax
		out     dx,al
		dec     dx              ; point back at status port
		loop    stp_1           ; repeat until all bits clocked

		in      al,dx           ; get data from MCU
		test    al,dso          ; isolate it
		jnz     stp_4           ; branch if bit is 0
		stc                     ; otherwise shift a 1 into dl
stp_4:          rcl     bx,1            ; get bit into dl:0
		rcl     ah,1            ; get next output bit into carry
		mov     al,dsclk+step_out       ; strobe out next bit
		jnb     stp_5
		or      al,dsi          ; output a one
stp_5:          inc     dx              ; point at output port
		out     dx,al           ; output it
		push    ax              ; delay
		mov     ax,clockrate
		inc     ax
		call    delay
		pop     ax
		and     al,not (step_out or dsclk)      ; enable break on fetch
		out     dx,al
		mov     dl,ah           ; put data 23:16 in al
		mov     dh,0            ; clear msb
		mov     ax,bx           ; put return value in ax
		ret                     ; done
bdm_step        endp

; bdm_getstat returns status of MCU

bdm_getstat proc    uses bx dx
                public  bdm_getstat
		mov     dx,bdm_port
		inc     dx              ; point to status port
		in      al,dx           ; get status
		test    al,nc           ; is cable plugged in?
		jnz     gs_0
		mov     ax,fault_cable
		push    ax
		call    bdm_error       ; fault - cable not plugged in
gs_0:           test    al,pwr_dwn      ; Vdd on target OK?
		jnz     gs_1            ; carry on if so
		mov     ax,fault_power
		push    ax
		call    bdm_error       ; no power on target - bail out
gs_1:           mov     ah,0
		test    al,freeze       ; freeze asserted?
		jz      gs_2
		or      ah,frz_stat     ; yes - set freeze flag
gs_2:           inc     dx              ; look at reset line
		in      al,dx
		test    al,reset        ; reset asserted ?
		jz      gs_3
		or      ah,rst_stat     ; yes - set flag
gs_3:           mov     al,ah           ; put status in lsb
		mov     ah,0            ; clear msb of result
		ret                     ; done
bdm_getstat     endp

; bdm_reset resets MCU by pulsing reset signal low, then high

bdm_reset	proc    uses ax dx
                public  bdm_reset
		mov     dx,bdm_port
		add     dx,bdm_ctl      ; point to output port
		mov     al,rst_out+step_out
		out     dx,al
		mov     ax,waitcnt      ; delay <waitcnt> loops
		call    delay
		mov     al,step_out
		out     dx,al
		ret                     ; done
bdm_reset       endp

; Delay waits before returning to the calling routine
; delay count parameter passed in ax

delay           proc    near
;                                                                       8088 80286 80386
dly_0:          nop                     ; delay                            3     3     3
		nop                     ; delay                            3     3     3
		dec     ax              ;                                  3     2     2
		jnz     dly_0           ; repeat till count goes to 0     16    18    18 clocks
;                                                                         ==    ==    ==
;                                                                         27    26    26
		ret                     ;                                 5.7  2.16  1.04 us/loop at
delay           endp                    ;                                4.77   12    25 MHz clock

		end
