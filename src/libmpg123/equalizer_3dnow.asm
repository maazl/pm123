;  1 "equalizer_3dnow.S"
;  1 "<built-in>"
;  1 "<command line>"
;  1 "equalizer_3dnow.S"
;  9 "equalizer_3dnow.S"
;  1 "mangle.h" 1
;  13 "mangle.h"
;  1 "config.h" 1
;  14 "mangle.h" 2
;  1 "intsym.h" 1
;  15 "mangle.h" 2
;  10 "equalizer_3dnow.S" 2

section .text
align 2
global	_INT123_do_equalizer_3dnow


_INT123_do_equalizer_3dnow
	push	dword esi
	push	dword ebx

	mov	dword ebx, [esp+12]

	mov	dword ecx, [esp+16]
	xor	dword edx, edx

	mov	dword esi, [esp+20]
	sal	dword ecx, 7
align 2
_L9
	movq	mm0, [ebx+edx]
	pfmul	mm0, [esi+ecx]

	movq	mm1, [ebx+edx+8]
	pfmul	mm1, [esi+ecx+8]
	movq	[ebx+edx], mm0

	movq	mm0, [ebx+edx+16]
	pfmul	mm0, [esi+ecx+16]
	movq	[ebx+edx+8], mm1

	movq	mm1, [ebx+edx+24]
	pfmul	mm1, [esi+ecx+24]
	movq	[ebx+edx+16], mm0

	movq	mm0, [ebx+edx+32]
	pfmul	mm0, [esi+ecx+32]
	movq	[ebx+edx+24], mm1

	movq	mm1, [ebx+edx+40]
	pfmul	mm1, [esi+ecx+40]
	movq	[ebx+edx+32], mm0

	movq	mm0, [ebx+edx+48]
	pfmul	mm0, [esi+ecx+48]
	movq	[ebx+edx+40], mm1

	movq	mm1, [ebx+edx+56]
	pfmul	mm1, [esi+ecx+56]
	movq	[ebx+edx+48], mm0
	movq	[ebx+edx+56], mm1

	add	dword edx, 64
	add	dword ecx, 32
	cmp	dword edx, 124
	jle	_L9
align 2
	pop	dword ebx
	pop	dword esi
        ret


