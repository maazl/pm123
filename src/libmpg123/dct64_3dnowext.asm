;  1 "dct64_3dnowext.S"
;  1 "<built-in>"
;  1 "<command line>"
;  1 "dct64_3dnowext.S"
;  23 "dct64_3dnowext.S"
;  1 "mangle.h" 1
;  13 "mangle.h"
;  1 "config.h" 1
;  14 "mangle.h" 2
;  1 "intsym.h" 1
;  15 "mangle.h" 2
;  24 "dct64_3dnowext.S" 2

extern	_INT123_costab_mmxsse
section .data
align 4


plus_1f
 dd 1065353216
align 8


x_plus_minus_3dnow
 dd 0
 dd -2147483648

section .text
align 32
global	_INT123_dct64_3dnowext

_INT123_dct64_3dnowext
	push	dword ebp
	mov	dword ebp, esp
	push	dword edi
	push	dword esi
	push	dword ebx
	sub	dword esp, 256

	mov	dword eax, [ebp+16]
	lea	dword edx, [ebp+128+-268]
	mov	dword esi, [ebp+8]
	mov	dword edi, [ebp+12]
	mov	dword ebx, _INT123_costab_mmxsse
	lea	dword ecx, [ebp+-268]
	movq	mm0, [eax]
	movq	mm4, [eax+8]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [eax+120]
	pswapd	mm5, [eax+112]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[edx], mm0
	movq	[edx+8], mm4
	pfsub	mm3, mm1
	pfsub	mm7, mm5
	pfmul	mm3, [ebx]
	pfmul	mm7, [ebx+8]
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[edx+120], mm3
	movq	[edx+112], mm7
	movq	mm0, [eax+16]
	movq	mm4, [eax+24]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [eax+104]
	pswapd	mm5, [eax+96]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[edx+16], mm0
	movq	[edx+24], mm4
	pfsub	mm3, mm1
	pfsub	mm7, mm5
	pfmul	mm3, [ebx+16]
	pfmul	mm7, [ebx+24]
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[edx+104], mm3
	movq	[edx+96], mm7
	movq	mm0, [eax+32]
	movq	mm4, [eax+40]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [eax+88]
	pswapd	mm5, [eax+80]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[edx+32], mm0
	movq	[edx+40], mm4
	pfsub	mm3, mm1
	pfsub	mm7, mm5
	pfmul	mm3, [ebx+32]
	pfmul	mm7, [ebx+40]
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[edx+88], mm3
	movq	[edx+80], mm7
	movq	mm0, [eax+48]
	movq	mm4, [eax+56]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [eax+72]
	pswapd	mm5, [eax+64]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[edx+48], mm0
	movq	[edx+56], mm4
	pfsub	mm3, mm1
	pfsub	mm7, mm5
	pfmul	mm3, [ebx+48]
	pfmul	mm7, [ebx+56]
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[edx+72], mm3
	movq	[edx+64], mm7
	movq	mm0, [edx]
	movq	mm4, [edx+8]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [edx+56]
	pswapd	mm5, [edx+48]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[ecx], mm0
	movq	[ecx+8], mm4
	pfsub	mm3, mm1
	pfsub	mm7, mm5
	pfmul	mm3, [ebx+64]
	pfmul	mm7, [ebx+72]
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[ecx+56], mm3
	movq	[ecx+48], mm7
	movq	mm0, [edx+16]
	movq	mm4, [edx+24]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [edx+40]
	pswapd	mm5, [edx+32]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[ecx+16], mm0
	movq	[ecx+24], mm4
	pfsub	mm3, mm1
	pfsub	mm7, mm5
	pfmul	mm3, [ebx+80]
	pfmul	mm7, [ebx+88]
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[ecx+40], mm3
	movq	[ecx+32], mm7
	movq	mm0, [edx+64]
	movq	mm4, [edx+72]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [edx+120]
	pswapd	mm5, [edx+112]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[ecx+64], mm0
	movq	[ecx+72], mm4
	pfsubr	mm3, mm1
	pfsubr	mm7, mm5
	pfmul	mm3, [ebx+64]
	pfmul	mm7, [ebx+72]
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[ecx+120], mm3
	movq	[ecx+112], mm7
	movq	mm0, [edx+80]
	movq	mm4, [edx+88]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [edx+104]
	pswapd	mm5, [edx+96]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[ecx+80], mm0
	movq	[ecx+88], mm4
	pfsubr	mm3, mm1
	pfsubr	mm7, mm5
	pfmul	mm3, [ebx+80]
	pfmul	mm7, [ebx+88]
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[ecx+104], mm3
	movq	[ecx+96], mm7
	movq	mm2, [ebx+96]
	movq	mm6, [ebx+104]
	movq	mm0, [ecx]
	movq	mm4, [ecx+8]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [ecx+24]
	pswapd	mm5, [ecx+16]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[edx], mm0
	movq	[edx+8], mm4
	pfsub	mm3, mm1
	pfsub	mm7, mm5
	pfmul	mm3, mm2
	pfmul	mm7, mm6
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[edx+24], mm3
	movq	[edx+16], mm7
	movq	mm0, [ecx+32]
	movq	mm4, [ecx+40]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [ecx+56]
	pswapd	mm5, [ecx+48]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[edx+32], mm0
	movq	[edx+40], mm4
	pfsubr	mm3, mm1
	pfsubr	mm7, mm5
	pfmul	mm3, mm2
	pfmul	mm7, mm6
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[edx+56], mm3
	movq	[edx+48], mm7
	movq	mm0, [ecx+64]
	movq	mm4, [ecx+72]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [ecx+88]
	pswapd	mm5, [ecx+80]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[edx+64], mm0
	movq	[edx+72], mm4
	pfsub	mm3, mm1
	pfsub	mm7, mm5
	pfmul	mm3, mm2
	pfmul	mm7, mm6
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[edx+88], mm3
	movq	[edx+80], mm7
	movq	mm0, [ecx+96]
	movq	mm4, [ecx+104]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [ecx+120]
	pswapd	mm5, [ecx+112]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[edx+96], mm0
	movq	[edx+104], mm4
	pfsubr	mm3, mm1
	pfsubr	mm7, mm5
	pfmul	mm3, mm2
	pfmul	mm7, mm6
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[edx+120], mm3
	movq	[edx+112], mm7
	movq	mm2, [ebx+112]
	movq	mm0, [edx]
	movq	mm4, [edx+16]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [edx+8]
	pswapd	mm5, [edx+24]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[ecx], mm0
	movq	[ecx+16], mm4
	pfsub	mm3, mm1
	pfsubr	mm7, mm5
	pfmul	mm3, mm2
	pfmul	mm7, mm2
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[ecx+8], mm3
	movq	[ecx+24], mm7
	movq	mm0, [edx+32]
	movq	mm4, [edx+48]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [edx+40]
	pswapd	mm5, [edx+56]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[ecx+32], mm0
	movq	[ecx+48], mm4
	pfsub	mm3, mm1
	pfsubr	mm7, mm5
	pfmul	mm3, mm2
	pfmul	mm7, mm2
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[ecx+40], mm3
	movq	[ecx+56], mm7
	movq	mm0, [edx+64]
	movq	mm4, [edx+80]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [edx+72]
	pswapd	mm5, [edx+88]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[ecx+64], mm0
	movq	[ecx+80], mm4
	pfsub	mm3, mm1
	pfsubr	mm7, mm5
	pfmul	mm3, mm2
	pfmul	mm7, mm2
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[ecx+72], mm3
	movq	[ecx+88], mm7
	movq	mm0, [edx+96]
	movq	mm4, [edx+112]
	movq	mm3, mm0
	movq	mm7, mm4
	pswapd	mm1, [edx+104]
	pswapd	mm5, [edx+120]
	pfadd	mm0, mm1
	pfadd	mm4, mm5
	movq	[ecx+96], mm0
	movq	[ecx+112], mm4
	pfsub	mm3, mm1
	pfsubr	mm7, mm5
	pfmul	mm3, mm2
	pfmul	mm7, mm2
	pswapd	mm3, mm3
	pswapd	mm7, mm7
	movq	[ecx+104], mm3
	movq	[ecx+120], mm7
	movd	mm6, [plus_1f]
	punpckldq	mm6, [ebx+120]
	movq	mm7, [x_plus_minus_3dnow]
	movq	mm0, [ecx+32]
	movq	mm2, [ecx+64]
	movq	mm1, mm0
	movq	mm3, mm2
	pxor	mm1, mm7
	pxor	mm3, mm7
	pfacc	mm0, mm1
	pfacc	mm2, mm3
	pfmul	mm0, mm6
	pfmul	mm2, mm6
	movq	[edx+32], mm0
	movq	[edx+64], mm2
	movd	mm0, [ecx+44]
	movd	mm2, [ecx+40]
	movd	mm3, [ebx+120]
	punpckldq	mm0, [ecx+76]
	punpckldq	mm2, [ecx+72]
	punpckldq	mm3, mm3
	movq	mm4, mm0
	movq	mm5, mm2
	pfsub	mm0, mm2
	pfmul	mm0, mm3
	movq	mm1, mm0
	pfadd	mm0, mm5
	pfadd	mm0, mm4
	movq	mm2, mm0
	punpckldq	mm0, mm1
	punpckhdq	mm2, mm1
	movq	[edx+40], mm0
	movq	[edx+72], mm2
	movd	mm3, [ecx+48]
	movd	mm2, [ecx+60]
	pfsub	mm3, [ecx+52]
	pfsub	mm2, [ecx+56]
	pfmul	mm3, [ebx+120]
	pfmul	mm2, [ebx+120]
	movq	mm1, mm2
	pfadd	mm1, [ecx+56]
	pfadd	mm1, [ecx+60]
	movq	mm0, mm1
	pfadd	mm0, [ecx+48]
	pfadd	mm0, [ecx+52]
	pfadd	mm1, mm3
	punpckldq	mm1, mm2
	pfadd	mm2, mm3
	punpckldq	mm0, mm2
	movq	[edx+56], mm1
	movq	[edx+48], mm0
	movd	mm1, [ecx+92]
	pfsub	mm1, [ecx+88]
	pfmul	mm1, [ebx+120]
	movd	[edx+92], mm1
	pfadd	mm1, [ecx+92]
	pfadd	mm1, [ecx+88]
	movq	mm0, mm1
	pfadd	mm0, [ecx+80]
	pfadd	mm0, [ecx+84]
	movd	[edx+80], mm0
	movd	mm0, [ecx+80]
	pfsub	mm0, [ecx+84]
	pfmul	mm0, [ebx+120]
	pfadd	mm1, mm0
	pfadd	mm0, [edx+92]
	punpckldq	mm0, mm1
	movq	[edx+84], mm0
	movq	mm0, [ecx+96]
	movq	mm1, mm0
	pxor	mm1, mm7
	pfacc	mm0, mm1
	pfmul	mm0, mm6
	movq	[edx+96], mm0
	movd	mm0, [ecx+108]
	pfsub	mm0, [ecx+104]
	pfmul	mm0, [ebx+120]
	movd	[edx+108], mm0
	pfadd	mm0, [ecx+104]
	pfadd	mm0, [ecx+108]
	movd	[edx+104], mm0
	movd	mm1, [ecx+124]
	pfsub	mm1, [ecx+120]
	pfmul	mm1, [ebx+120]
	movd	[edx+124], mm1
	pfadd	mm1, [ecx+120]
	pfadd	mm1, [ecx+124]
	movq	mm0, mm1
	pfadd	mm0, [ecx+112]
	pfadd	mm0, [ecx+116]
	movd	[edx+112], mm0
	movd	mm0, [ecx+112]
	pfsub	mm0, [ecx+116]
	pfmul	mm0, [ebx+120]
	pfadd	mm1, mm0
	pfadd	mm0, [edx+124]
	punpckldq	mm0, mm1
	movq	[edx+116], mm0
	jnz	near _L01
	movd	mm0, [ecx]
	pfadd	mm0, [ecx+4]
	movd	[esi+1024], mm0
	movd	mm0, [ecx]
	pfsub	mm0, [ecx+4]
	pfmul	mm0, [ebx+120]
	movd	[esi], mm0
	movd	[edi], mm0
	movd	mm0, [ecx+12]
	pfsub	mm0, [ecx+8]
	pfmul	mm0, [ebx+120]
	movd	[edi+512], mm0
	pfadd	mm0, [ecx+12]
	pfadd	mm0, [ecx+8]
	movd	[esi+512], mm0
	movd	mm0, [ecx+16]
	pfsub	mm0, [ecx+20]
	pfmul	mm0, [ebx+120]
	movq	mm3, mm0
	movd	mm0, [ecx+28]
	pfsub	mm0, [ecx+24]
	pfmul	mm0, [ebx+120]
	movd	[edi+768], mm0
	movq	mm2, mm0
	pfadd	mm0, [ecx+24]
	pfadd	mm0, [ecx+28]
	movq	mm1, mm0
	pfadd	mm0, [ecx+16]
	pfadd	mm0, [ecx+20]
	movd	[esi+768], mm0
	pfadd	mm1, mm3
	movd	[esi+256], mm1
	pfadd	mm2, mm3
	movd	[edi+256], mm2
	movq	mm0, [edx+32]
	movq	mm1, [edx+48]
	pfadd	mm0, [edx+48]
	pfadd	mm1, [edx+40]
	movd	[esi+896], mm0
	movd	[esi+640], mm1
	psrlq	mm0, 32
	psrlq	mm1, 32
	movd	[edi+128], mm0
	movd	[edi+384], mm1
	movd	mm0, [edx+40]
	pfadd	mm0, [edx+56]
	movd	[esi+384], mm0
	movd	mm0, [edx+56]
	pfadd	mm0, [edx+36]
	movd	[esi+128], mm0
	movd	mm0, [edx+60]
	movd	[edi+896], mm0
	pfadd	mm0, [edx+44]
	movd	[edi+640], mm0
	movq	mm0, [edx+96]
	movq	mm2, [edx+112]
	movq	mm4, [edx+104]
	pfadd	mm0, [edx+112]
	pfadd	mm2, [edx+104]
	pfadd	mm4, [edx+120]
	movq	mm1, mm0
	movq	mm3, mm2
	movq	mm5, mm4
	pfadd	mm0, [edx+64]
	pfadd	mm2, [edx+80]
	pfadd	mm4, [edx+72]
	movd	[esi+960], mm0
	movd	[esi+704], mm2
	movd	[esi+448], mm4
	psrlq	mm0, 32
	psrlq	mm2, 32
	psrlq	mm4, 32
	movd	[edi+64], mm0
	movd	[edi+320], mm2
	movd	[edi+576], mm4
	pfadd	mm1, [edx+80]
	pfadd	mm3, [edx+72]
	pfadd	mm5, [edx+88]
	movd	[esi+832], mm1
	movd	[esi+576], mm3
	movd	[esi+320], mm5
	psrlq	mm1, 32
	psrlq	mm3, 32
	psrlq	mm5, 32
	movd	[edi+192], mm1
	movd	[edi+448], mm3
	movd	[edi+704], mm5
	movd	mm0, [edx+120]
	pfadd	mm0, [edx+100]
	movq	mm1, mm0
	pfadd	mm0, [edx+88]
	movd	[esi+192], mm0
	pfadd	mm1, [edx+68]
	movd	[esi+64], mm1
	movd	mm0, [edx+124]
	movd	[edi+960], mm0
	pfadd	mm0, [edx+92]
	movd	[edi+832], mm0
	jmp	dword _L_bye
_L01
	movq	mm0, [ecx]
	movq	mm1, mm0
	pxor	mm1, mm7
	pfacc	mm0, mm1
	pfmul	mm0, mm6
	pf2iw	mm0, mm0
	movd	eax, mm0
	mov	word [esi+512], ax
	psrlq	mm0, 32
	movd	eax, mm0
	mov	word [esi], ax
	movd	mm0, [ecx+12]
	pfsub	mm0, [ecx+8]
	pfmul	mm0, [ebx+120]
	pf2iw	mm7, mm0
	movd	eax, mm7
	mov	word [edi+256], ax
	pfadd	mm0, [ecx+12]
	pfadd	mm0, [ecx+8]
	pf2iw	mm0, mm0
	movd	eax, mm0
	mov	word [esi+256], ax
	movd	mm3, [ecx+16]
	pfsub	mm3, [ecx+20]
	pfmul	mm3, [ebx+120]
	movq	mm2, mm3
	movd	mm2, [ecx+28]
	pfsub	mm2, [ecx+24]
	pfmul	mm2, [ebx+120]
	movq	mm1, mm2
	pf2iw	mm7, mm2
	movd	eax, mm7
	mov	word [edi+384], ax
	pfadd	mm1, [ecx+24]
	pfadd	mm1, [ecx+28]
	movq	mm0, mm1
	pfadd	mm0, [ecx+16]
	pfadd	mm0, [ecx+20]
	pf2iw	mm0, mm0
	movd	eax, mm0
	mov	word [esi+384], ax
	pfadd	mm1, mm3
	pf2iw	mm1, mm1
	movd	eax, mm1
	mov	word [esi+128], ax
	pfadd	mm2, mm3
	pf2iw	mm2, mm2
	movd	eax, mm2
	mov	word [edi+128], ax
	movq	mm0, [edx+32]
	movq	mm1, [edx+48]
	pfadd	mm0, [edx+48]
	pfadd	mm1, [edx+40]
	pf2iw	mm0, mm0
	pf2iw	mm1, mm1
	movd	eax, mm0
	movd	ecx, mm1
	mov	word [esi+448], ax
	mov	word [esi+320], cx
	psrlq	mm0, 32
	psrlq	mm1, 32
	movd	eax, mm0
	movd	ecx, mm1
	mov	word [edi+64], ax
	mov	word [edi+192], cx
	movd	mm3, [edx+40]
	movd	mm4, [edx+56]
	movd	mm0, [edx+60]
	movd	mm2, [edx+44]
	movd	mm5, [edx+120]
	punpckldq	mm3, mm4
	punpckldq	mm0, [edx+124]
	pfadd	mm5, [edx+100]
	punpckldq	mm4, [edx+36]
	punpckldq	mm2, [edx+92]
	movq	mm6, mm5
	pfadd	mm3, mm4
	pf2iw	mm1, mm0
	pf2iw	mm3, mm3
	pfadd	mm5, [edx+88]
	movd	eax, mm1
	movd	ecx, mm3
	mov	word [edi+448], ax
	mov	word [esi+192], cx
	pf2iw	mm5, mm5
	psrlq	mm1, 32
	psrlq	mm3, 32
	movd	ebx, mm5
	movd	eax, mm1
	movd	ecx, mm3
	mov	word [esi+96], bx
	mov	word [edi+480], ax
	mov	word [esi+64], cx
	pfadd	mm0, mm2
	pf2iw	mm0, mm0
	movd	eax, mm0
	pfadd	mm6, [edx+68]
	mov	word [edi+320], ax
	psrlq	mm0, 32
	pf2iw	mm6, mm6
	movd	eax, mm0
	movd	ebx, mm6
	mov	word [edi+416], ax
	mov	word [esi+32], bx
	movq	mm0, [edx+96]
	movq	mm2, [edx+112]
	movq	mm4, [edx+104]
	pfadd	mm0, mm2
	pfadd	mm2, mm4
	pfadd	mm4, [edx+120]
	movq	mm1, mm0
	movq	mm3, mm2
	movq	mm5, mm4
	pfadd	mm0, [edx+64]
	pfadd	mm2, [edx+80]
	pfadd	mm4, [edx+72]
	pf2iw	mm0, mm0
	pf2iw	mm2, mm2
	pf2iw	mm4, mm4
	movd	eax, mm0
	movd	ecx, mm2
	movd	ebx, mm4
	mov	word [esi+480], ax
	mov	word [esi+352], cx
	mov	word [esi+224], bx
	psrlq	mm0, 32
	psrlq	mm2, 32
	psrlq	mm4, 32
	movd	eax, mm0
	movd	ecx, mm2
	movd	ebx, mm4
	mov	word [edi+32], ax
	mov	word [edi+160], cx
	mov	word [edi+288], bx
	pfadd	mm1, [edx+80]
	pfadd	mm3, [edx+72]
	pfadd	mm5, [edx+88]
	pf2iw	mm1, mm1
	pf2iw	mm3, mm3
	pf2iw	mm5, mm5
	movd	eax, mm1
	movd	ecx, mm3
	movd	ebx, mm5
	mov	word [esi+416], ax
	mov	word [esi+288], cx
	mov	word [esi+160], bx
	psrlq	mm1, 32
	psrlq	mm3, 32
	psrlq	mm5, 32
	movd	eax, mm1
	movd	ecx, mm3
	movd	ebx, mm5
	mov	word [edi+96], ax
	mov	word [edi+224], cx
	mov	word [edi+352], bx
        movsw
_L_bye
        femms


	add	dword esp, 256
	pop	dword ebx
	pop	dword esi
	pop	dword edi
        leave
        ret



