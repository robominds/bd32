; callf.inc
; macro to define near/far call, as determined by memory model in use

callf   macro   where
	if      @CodeSize
	call    far ptr where
	else
	call    near ptr where
	endif
	endm


