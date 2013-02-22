;  1 "synth_3dnow.S"
;  1 "<built-in>"
;  1 "<command line>"
;  1 "synth_3dnow.S"
;  34 "synth_3dnow.S"
;  1 "mangle.h" 1
;  13 "mangle.h"
;  1 "config.h" 1
;  14 "mangle.h" 2
;  1 "intsym.h" 1
;  15 "mangle.h" 2
;  35 "synth_3dnow.S" 2
;  53 "synth_3dnow.S"
extern	_INT123_dct64_3dnow
section .text
align 16
global	_INT123_synth_1to1_3dnow_asm

_INT123_synth_1to1_3dnow_asm
	sub	dword esp, 24
	push	dword ebp
	push	dword edi
	xor	dword ebp, ebp
	push	dword esi
	push	dword ebx
;  75 "synth_3dnow.S"
	mov	dword esi, [esp+52]
	mov	dword [esp+16], esi
	mov	dword ebx, [esp+48]
	mov	dword esi, [esp+60]
	mov	dword edx, [esi]

        femms
	test	dword ebx, ebx
	jne	_L26

	dec	dword edx
	and	dword edx, 15
	mov	dword [esi], edx
	mov	dword ecx, [esp+56]
	jmp	dword _L27
_L26
	add	dword [esp+16], 2
	mov	dword ecx, [esp+56]
	add	dword ecx, 2176
_L27

	test	byte dl, 1
	je	_L28
	mov	dword [esp+36], edx
	mov	dword ebx, ecx
	mov	dword esi, [esp+44]
	mov	dword edi, edx
	push	dword esi
	sal	dword edi, 2
	mov	dword eax, ebx
	mov	dword [esp+24], edi
	add	dword eax, edi
	push	dword eax
	mov	dword eax, edx
	inc	dword eax
	and	dword eax, 15
	lea	dword eax, [eax*4+1088]
	add	dword eax, ebx
	push	dword eax
	call	dword _INT123_dct64_3dnow
	add	dword esp, 12
	jmp	dword _L29
_L28
	lea	dword esi, [edx+1]
	mov	dword edi, [esp+44]
	mov	dword [esp+36], esi
	lea	dword eax, [ecx+edx*4+1092]
	push	dword edi
	lea	dword ebx, [ecx+1088]
	push	dword eax
	sal	dword esi, 2
	lea	dword eax, [ecx+edx*4]
	push	dword eax
	call	dword _INT123_dct64_3dnow
	add	dword esp, 12
	mov	dword [esp+20], esi
_L29
	mov	dword edx, [esp+64]
	add	dword edx, 64
	mov	dword ecx, 16
	sub	dword edx, [esp+20]
	mov	dword edi, [esp+16]

	pcmpeqb	mm7, mm7
	pslld	mm7, 31
	movq	mm0, [edx]
	movq	mm1, [ebx]
align 32
_L33
	movq	mm3, [edx+8]
	pfmul	mm0, mm1
	movq	mm4, [ebx+8]
	movq	mm5, [edx+16]
	pfmul	mm3, mm4
	movq	mm6, [ebx+16]
	pfadd	mm0, mm3
	movq	mm1, [edx+24]
	pfmul	mm5, mm6
	movq	mm2, [ebx+24]
	pfadd	mm0, mm5
	movq	mm3, [edx+32]
	pfmul	mm1, mm2
	movq	mm4, [ebx+32]
	pfadd	mm0, mm1
	movq	mm5, [edx+40]
	pfmul	mm3, mm4
	movq	mm6, [ebx+40]
	pfadd	mm0, mm3
	movq	mm1, [edx+48]
	pfmul	mm5, mm6
	movq	mm2, [ebx+48]
	pfadd	mm5, mm0
	movq	mm3, [edx+56]
	pfmul	mm2, mm1
	movq	mm4, [ebx+56]
	pfadd	mm2, mm5
	add	dword ebx, 64
	sub	dword edx, -128
	movq	mm0, [edx]
	pfmul	mm3, mm4
	movq	mm1, [ebx]
	pfadd	mm2, mm3
	movq	mm3, mm2
	psrlq	mm3, 32
	pfsub	mm2, mm3
	inc	dword ebp





	pf2id	mm2, mm2
	packssdw	mm2, mm2

	movd	eax, mm2
	mov	word [edi+0], ax
	add	dword edi, 4
	dec	dword ecx
	jnz	_L33

	movd	mm0, [ebx]
	movd	mm1, [edx]
	punpckldq	mm0, [ebx+8]
	punpckldq	mm1, [edx+8]
	movd	mm3, [ebx+16]
	movd	mm4, [edx+16]
	pfmul	mm0, mm1
	punpckldq	mm3, [ebx+24]
	punpckldq	mm4, [edx+24]
	movd	mm5, [ebx+32]
	movd	mm6, [edx+32]
	pfmul	mm3, mm4
	punpckldq	mm5, [ebx+40]
	punpckldq	mm6, [edx+40]
	pfadd	mm0, mm3
	movd	mm1, [ebx+48]
	movd	mm2, [edx+48]
	pfmul	mm5, mm6
	punpckldq	mm1, [ebx+56]
	punpckldq	mm2, [edx+56]
	pfadd	mm0, mm5
	pfmul	mm1, mm2
	pfadd	mm0, mm1
	pfacc	mm0, mm1





	pf2id	mm0, mm0
	packssdw	mm0, mm0

	movd	eax, mm0
	mov	word [edi+0], ax
	inc	dword ebp
	mov	dword esi, [esp+36]
	add	dword ebx, -64
	mov	dword ebp, 15
	add	dword edi, 4
	lea	dword edx, [edx+esi*8+-128]

	mov	dword ecx, 15
	movd	mm0, [ebx]
	movd	mm1, [edx+-4]
	punpckldq	mm0, [ebx+4]
	punpckldq	mm1, [edx+-8]
align 32
_L46
	movd	mm3, [ebx+8]
	movd	mm4, [edx+-12]
	pfmul	mm0, mm1
	punpckldq	mm3, [ebx+12]
	punpckldq	mm4, [edx+-16]
	movd	mm5, [ebx+16]
	movd	mm6, [edx+-20]
	pfmul	mm3, mm4
	punpckldq	mm5, [ebx+20]
	punpckldq	mm6, [edx+-24]
	pfadd	mm0, mm3
	movd	mm1, [ebx+24]
	movd	mm2, [edx+-28]
	pfmul	mm5, mm6
	punpckldq	mm1, [ebx+28]
	punpckldq	mm2, [edx+-32]
	pfadd	mm0, mm5
	movd	mm3, [ebx+32]
	movd	mm4, [edx+-36]
	pfmul	mm1, mm2
	punpckldq	mm3, [ebx+36]
	punpckldq	mm4, [edx+-40]
	pfadd	mm0, mm1
	movd	mm5, [ebx+40]
	movd	mm6, [edx+-44]
	pfmul	mm3, mm4
	punpckldq	mm5, [ebx+44]
	punpckldq	mm6, [edx+-48]
	pfadd	mm0, mm3
	movd	mm1, [ebx+48]
	movd	mm2, [edx+-52]
	pfmul	mm5, mm6
	punpckldq	mm1, [ebx+52]
	punpckldq	mm2, [edx+-56]
	pfadd	mm5, mm0
	movd	mm3, [ebx+56]
	movd	mm4, [edx+-60]
	pfmul	mm1, mm2
	punpckldq	mm3, [ebx+60]
	punpckldq	mm4, [edx]
	pfadd	mm5, mm1
	add	dword edx, -128
	add	dword ebx, -64
	movd	mm0, [ebx]
	movd	mm1, [edx+-4]
	pfmul	mm3, mm4
	punpckldq	mm0, [ebx+4]
	punpckldq	mm1, [edx+-8]
	pfadd	mm3, mm5
	pfacc	mm3, mm3
	inc	dword ebp
	pxor	mm3, mm7





	pf2id	mm3, mm3
	packssdw	mm3, mm3

	movd	eax, mm3
	mov	word [edi], ax
	add	dword edi, 4
	dec	dword ecx
	jnz	_L46

        femms
	mov	dword eax, ebp
	pop	dword ebx
	pop	dword esi
	pop	dword edi
	pop	dword ebp
	add	dword esp, 24
        ret


