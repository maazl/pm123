;  1 "synth_i586_dither.S"
;  1 "<built-in>"
;  1 "<command line>"
;  1 "synth_i586_dither.S"
;  14 "synth_i586_dither.S"
;  1 "mangle.h" 1
;  13 "mangle.h"
;  1 "config.h" 1
;  14 "mangle.h" 2
;  1 "intsym.h" 1
;  15 "mangle.h" 2
;  15 "synth_i586_dither.S" 2

section .data
extern	_INT123_dct64_i386


align 8
_LC0:
 dd 0x0,0x40dfffc0
align 8
_LC1:
 dd 0x0,0xc0e00000
align 8
section .text

global	_INT123_synth_1to1_i586_asm_dither
_INT123_synth_1to1_i586_asm_dither:
	sub	dword esp, 16
	push	dword ebp
	push	dword edi
	push	dword esi
	push	dword ebx
;  53 "synth_i586_dither.S"
	mov	dword eax, [esp+36]
	mov	dword esi, [esp+44]
	mov	dword ebx, [esp+52]
	mov	dword ebp, [ebx]
	mov	dword edi, [ebx+4];
	mov	dword [esp+28], edi
	xor	dword edi, edi
	cmp	dword [esp+40], edi
	jne	_L48
	dec	dword ebp
	and	dword ebp, 15
	mov	dword [ebx], ebp
	mov	dword ecx, [esp+48]
	jmp	dword _L49
_L48:


	sub	dword [esp+28], 128
	and	dword [esp+28], 0x0003fffc
	add	dword esi, 2
	mov	dword ecx, [esp+48]
	add	dword ecx, 2176
_L49:

	test	dword ebp, 1
	je	_L50
	mov	dword ebx, ecx
	mov	dword [esp+16], ebp
	push	dword eax
	mov	dword edx, [esp+20]
	lea	dword eax, [ebx+edx*4]
	push	dword eax
	mov	dword eax, [esp+24]
	inc	dword eax
	and	dword eax, 15
	lea	dword eax, [eax*4+1088]
	add	dword eax, ebx
	jmp	dword _L74
_L50:
	lea	dword ebx, [ecx+1088]
	lea	dword edx, [ebp+1]
	mov	dword [esp+16], edx
	push	dword eax
	lea	dword eax, [ecx+ebp*4+1092]
	push	dword eax
	lea	dword eax, [ecx+ebp*4]
_L74:
	push	dword eax
	call	dword _INT123_dct64_i386
	add	dword esp, 12


	mov	dword edx, [esp+16]
	lea	dword edx, [edx*4+0]

	mov	dword eax, [esp+56]
	add	dword eax, 64
	mov	dword ecx, eax
	sub	dword ecx, edx
	mov	dword ebp, 16
_L55:
	fld	dword [ecx]
	fmul	dword [ebx]
	fld	dword [ecx+4]
	fmul	dword [ebx+4]
	fxch	st1
	fld	dword [ecx+8]
	fmul	dword [ebx+8]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+12]
	fmul	dword [ebx+12]
	fxch	st2
	faddp	st1, st0
	fld	dword [ecx+16]
	fmul	dword [ebx+16]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+20]
	fmul	dword [ebx+20]
	fxch	st2
	faddp	st1, st0
	fld	dword [ecx+24]
	fmul	dword [ebx+24]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+28]
	fmul	dword [ebx+28]
	fxch	st2
	faddp	st1, st0
	fld	dword [ecx+32]
	fmul	dword [ebx+32]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+36]
	fmul	dword [ebx+36]
	fxch	st2
	faddp	st1, st0
	fld	dword [ecx+40]
	fmul	dword [ebx+40]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+44]
	fmul	dword [ebx+44]
	fxch	st2
	faddp	st1, st0
	fld	dword [ecx+48]
	fmul	dword [ebx+48]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+52]
	fmul	dword [ebx+52]
	fxch	st2
	faddp	st1, st0
	fld	dword [ecx+56]
	fmul	dword [ebx+56]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+60]
	fmul	dword [ebx+60]
	fxch	st2
	sub	dword esp, 4
	faddp	st1, st0
	fxch	st1
	fsubrp	st1, st0

	add	dword [esp+32], 4
	and	dword [esp+32], 0x0003fffc
	mov	dword edi, [esp+64]
	add	dword edi, [esp+32]

	fadd	dword [edi]


	fistp	dword [esp]
	pop	dword eax
	cmp	dword eax, 32767
	jg	.1
	cmp	dword eax, -32768
	jl	.2
	mov	word [esi], ax
	jmp	.4
.1:	mov	word [esi], 32767
	jmp	.3
.2:	mov	word [esi], -32768
.3:

.4:
_L54:
	add	dword ebx, 64
	sub	dword ecx, -128
	add	dword esi, 4
	dec	dword ebp
	jnz	_L55
	fld	dword [ecx]
	fmul	dword [ebx]
	fld	dword [ecx+8]
	fmul	dword [ebx+8]
	fld	dword [ecx+16]
	fmul	dword [ebx+16]
	fxch	st2
	faddp	st1, st0
	fld	dword [ecx+24]
	fmul	dword [ebx+24]
	fxch	st2
	faddp	st1, st0
	fld	dword [ecx+32]
	fmul	dword [ebx+32]
	fxch	st2
	faddp	st1, st0
	fld	dword [ecx+40]
	fmul	dword [ebx+40]
	fxch	st2
	faddp	st1, st0
	fld	dword [ecx+48]
	fmul	dword [ebx+48]
	fxch	st2
	faddp	st1, st0
	fld	dword [ecx+56]
	fmul	dword [ebx+56]
	fxch	st2
	sub	dword esp, 4
	faddp	st1, st0
	fxch	st1
	faddp	st1, st0

	add	dword [esp+32], 4
	and	dword [esp+32], 0x0003fffc
	mov	dword edi, [esp+64]
	add	dword edi, [esp+32]

	fadd	dword [edi]

	fistp	dword [esp]
	pop	dword eax
	cmp	dword eax, 32767
	jg	.1
	cmp	dword eax, -32768
	jl	.2
	mov	word [esi], ax
	jmp	.4
.1:	mov	word [esi], 32767
	jmp	.3
.2:	mov	word [esi], -32768
.3:

.4:
_L62:
	add	dword ebx, -64
	add	dword esi, 4
	mov	dword edx, [esp+16]
	lea	dword ecx, [ecx+edx*8+-128]
	mov	dword ebp, 15
_L68:
	fld	dword [ecx+-4]
        fchs
	fmul	dword [ebx]
	fld	dword [ecx+-8]
	fmul	dword [ebx+4]
	fxch	st1
	fld	dword [ecx+-12]
	fmul	dword [ebx+8]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+-16]
	fmul	dword [ebx+12]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+-20]
	fmul	dword [ebx+16]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+-24]
	fmul	dword [ebx+20]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+-28]
	fmul	dword [ebx+24]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+-32]
	fmul	dword [ebx+28]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+-36]
	fmul	dword [ebx+32]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+-40]
	fmul	dword [ebx+36]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+-44]
	fmul	dword [ebx+40]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+-48]
	fmul	dword [ebx+44]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+-52]
	fmul	dword [ebx+48]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+-56]
	fmul	dword [ebx+52]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx+-60]
	fmul	dword [ebx+56]
	fxch	st2
	fsubrp	st1, st0
	fld	dword [ecx]
	fmul	dword [ebx+60]
	fxch	st2
	sub	dword esp, 4
	fsubrp	st1, st0
	fxch	st1
	fsubrp	st1, st0

	add	dword [esp+32], 4
	and	dword [esp+32], 0x0003fffc
	mov	dword edi, [esp+64]
	add	dword edi, [esp+32]

	fadd	dword [edi]

	fistp	dword [esp]
	pop	dword eax
	cmp	dword eax, 32767
	jg	.1
	cmp	dword eax, -32768
	jl	.2
	mov	word [esi], ax
	jmp	.4
.1:	mov	word [esi], 32767
	jmp	.3
.2:	mov	word [esi], -32768
.3:

.4:
_L67:
	add	dword ebx, -64
	add	dword ecx, -128
	add	dword esi, 4
	dec	dword ebp
	jnz	_L68

	mov	dword eax, 0

	mov	dword ebx, [esp+52]
	mov	dword esi, [esp+28]
	mov	dword [ebx+4], esi;

	pop	dword ebx
	pop	dword esi
	pop	dword edi
	pop	dword ebp
	add	dword esp, 16

        ret


