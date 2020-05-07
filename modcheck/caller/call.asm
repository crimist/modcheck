.code

; https://stackoverflow.com/a/50501503/6389542
; arg order: rcx, rdx, r8, r9
SpoofCall proc
	push r8		; push real ret onto stack
	push rdx	; push gadget onto stack
	jmp rcx		; jmp to adr
	ret
SpoofCall endp

end
