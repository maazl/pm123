;  1 "dct64_sse_float.S"
;  1 "<built-in>"
;  1 "<command line>"
;  1 "dct64_sse_float.S"
;  9 "dct64_sse_float.S"
;  1 "mangle.h" 1
;  13 "mangle.h"
;  1 "config.h" 1
;  14 "mangle.h" 2
;  1 "intsym.h" 1
;  15 "mangle.h" 2
;  10 "dct64_sse_float.S" 2
;  22 "dct64_sse_float.S"
section .data
extern	_INT123_costab_mmxsse

align 4
pnpn
 dd 0
 dd -2147483648
 dd 0
 dd -2147483648
align 4
mask
 dd -1
 dd -1
 dd -1
 dd 0

section .text
align 4
global	_INT123_dct64_real_sse
_INT123_dct64_real_sse
	push	dword ebp
	mov	dword ebp, esp

	and	dword esp, -16
	sub	dword esp, 128
	push	dword ebx

	mov	dword ecx, [ebp+(8+04)]
	mov	dword ebx, [ebp+(8+14)]
	mov	dword eax, [ebp+(8+24)]

	movups	xmm7, [eax]
	movups	xmm6, [eax+16]
	movups	xmm0, [eax+112]
	movups	xmm1, [eax+96]
	shufps	xmm0, xmm0, 0x1b
	shufps	xmm1, xmm1, 0x1b
	movaps	xmm4, xmm7
	movaps	xmm5, xmm6
	addps	xmm4, xmm0
	addps	xmm5, xmm1
	subps	xmm7, xmm0
	subps	xmm6, xmm1
	movaps	[esp+(4+016)], xmm4
	movaps	[esp+(4+116)], xmm5

	movups	xmm2, [eax+32]
	movups	xmm3, [eax+48]
	movups	xmm0, [eax+80]
	movups	xmm1, [eax+64]
	shufps	xmm0, xmm0, 0x1b
	shufps	xmm1, xmm1, 0x1b
	movaps	xmm5, xmm2
	movaps	xmm4, xmm3
	addps	xmm2, xmm0
	addps	xmm3, xmm1
	subps	xmm5, xmm0
	subps	xmm4, xmm1

	mulps	xmm7, [_INT123_costab_mmxsse]
	mulps	xmm6, [_INT123_costab_mmxsse+16]
	mulps	xmm5, [_INT123_costab_mmxsse+32]
	mulps	xmm4, [_INT123_costab_mmxsse+48]

	shufps	xmm2, xmm2, 0x1b
	shufps	xmm3, xmm3, 0x1b
	shufps	xmm4, xmm4, 0x1b
	shufps	xmm5, xmm5, 0x1b
	movaps	xmm0, [esp+(4+016)]
	movaps	xmm1, [esp+(4+116)]
	subps	xmm0, xmm3
	subps	xmm1, xmm2
	addps	xmm3, [esp+(4+016)]
	addps	xmm2, [esp+(4+116)]
	movaps	[esp+(4+016)], xmm3
	movaps	[esp+(4+116)], xmm2
	movaps	xmm2, xmm6
	movaps	xmm3, xmm7
	subps	xmm6, xmm5
	subps	xmm7, xmm4
	addps	xmm4, xmm3
	addps	xmm5, xmm2
	mulps	xmm0, [_INT123_costab_mmxsse+64]
	mulps	xmm1, [_INT123_costab_mmxsse+80]
	mulps	xmm6, [_INT123_costab_mmxsse+80]
	mulps	xmm7, [_INT123_costab_mmxsse+64]

	movaps	xmm2, [esp+(4+016)]
	movaps	xmm3, [esp+(4+116)]
	shufps	xmm3, xmm3, 0x1b
	shufps	xmm5, xmm5, 0x1b
	shufps	xmm1, xmm1, 0x1b
	shufps	xmm6, xmm6, 0x1b
	movaps	[esp+(4+116)], xmm0
	subps	xmm2, xmm3
	subps	xmm0, xmm1
	addps	xmm3, [esp+(4+016)]
	addps	xmm1, [esp+(4+116)]
	movaps	[esp+(4+016)], xmm3
	movaps	[esp+(4+216)], xmm1
	movaps	xmm1, xmm5
	movaps	xmm5, xmm4
	movaps	xmm3, xmm7
	subps	xmm5, xmm1
	subps	xmm7, xmm6
	addps	xmm4, xmm1
	addps	xmm6, xmm3
	mulps	xmm2, [_INT123_costab_mmxsse+96]
	mulps	xmm0, [_INT123_costab_mmxsse+96]
	mulps	xmm5, [_INT123_costab_mmxsse+96]
	mulps	xmm7, [_INT123_costab_mmxsse+96]
	movaps	[esp+(4+116)], xmm2
	movaps	[esp+(4+316)], xmm0

	movaps	xmm2, xmm4
	movaps	xmm3, xmm5
	shufps	xmm2, xmm6, 0x44
	shufps	xmm5, xmm7, 0xbb
	shufps	xmm4, xmm6, 0xbb
	shufps	xmm3, xmm7, 0x44
	movaps	xmm6, xmm2
	movaps	xmm7, xmm3
	subps	xmm2, xmm4
	subps	xmm3, xmm5
	addps	xmm4, xmm6
	addps	xmm5, xmm7
	movaps	xmm0, [_INT123_costab_mmxsse+112]
	movlhps	xmm0, xmm0
	mulps	xmm2, xmm0
	mulps	xmm3, xmm0
	movaps	[esp+(4+416)], xmm0
	movaps	xmm6, xmm4
	movaps	xmm7, xmm5
	shufps	xmm4, xmm2, 0x14
	shufps	xmm6, xmm2, 0xbe
	shufps	xmm5, xmm3, 0x14
	shufps	xmm7, xmm3, 0xbe
	movaps	[esp+(4+516)], xmm5
	movaps	[esp+(4+716)], xmm7

	movaps	xmm0, [esp+(4+016)]
	movaps	xmm1, [esp+(4+116)]
	movaps	xmm2, xmm0
	movaps	xmm3, xmm1
	shufps	xmm2, [esp+(4+216)], 0x44
	shufps	xmm1, [esp+(4+316)], 0xbb
	shufps	xmm0, [esp+(4+216)], 0xbb
	shufps	xmm3, [esp+(4+316)], 0x44
	movaps	xmm5, xmm2
	movaps	xmm7, xmm3
	subps	xmm2, xmm0
	subps	xmm3, xmm1
	addps	xmm0, xmm5
	addps	xmm1, xmm7
	mulps	xmm2, [esp+(4+416)]
	mulps	xmm3, [esp+(4+416)]
	movaps	xmm5, xmm0
	movaps	xmm7, xmm1
	shufps	xmm0, xmm2, 0x14
	shufps	xmm5, xmm2, 0xbe
	shufps	xmm1, xmm3, 0x14
	shufps	xmm7, xmm3, 0xbe

	movaps	[esp+(4+016)], xmm0
	movaps	[esp+(4+116)], xmm1
	movaps	[esp+(4+216)], xmm5
	movaps	[esp+(4+316)], xmm7

	movss	xmm5, [_INT123_costab_mmxsse+120]
	shufps	xmm5, xmm5, 0x00
	xorps	xmm5, [pnpn]

	movaps	xmm0, xmm4
	movaps	xmm1, xmm6
	unpcklps	xmm4, [esp+(4+516)]
	unpckhps	xmm0, [esp+(4+516)]
	unpcklps	xmm6, [esp+(4+716)]
	unpckhps	xmm1, [esp+(4+716)]
	movaps	xmm2, xmm4
	movaps	xmm3, xmm6
	unpcklps	xmm4, xmm0
	unpckhps	xmm2, xmm0
	unpcklps	xmm6, xmm1
	unpckhps	xmm3, xmm1
	movaps	xmm0, xmm4
	movaps	xmm1, xmm6
	subps	xmm0, xmm2
	subps	xmm1, xmm3
	addps	xmm4, xmm2
	addps	xmm6, xmm3
	mulps	xmm0, xmm5
	mulps	xmm1, xmm5
	movaps	[esp+(4+516)], xmm5
	movaps	xmm5, xmm4
	movaps	xmm7, xmm6
	unpcklps	xmm4, xmm0
	unpckhps	xmm5, xmm0
	unpcklps	xmm6, xmm1
	unpckhps	xmm7, xmm1

	movaps	xmm0, [esp+(4+016)]
	movaps	xmm2, [esp+(4+216)]
	movaps	[esp+(4+416)], xmm4
	movaps	[esp+(4+616)], xmm6

	movaps	xmm4, xmm0
	movaps	xmm6, xmm2
	unpcklps	xmm0, [esp+(4+116)]
	unpckhps	xmm4, [esp+(4+116)]
	unpcklps	xmm2, [esp+(4+316)]
	unpckhps	xmm6, [esp+(4+316)]
	movaps	xmm1, xmm0
	movaps	xmm3, xmm2
	unpcklps	xmm0, xmm4
	unpckhps	xmm1, xmm4
	unpcklps	xmm2, xmm6
	unpckhps	xmm3, xmm6
	movaps	xmm4, xmm0
	movaps	xmm6, xmm2
	subps	xmm4, xmm1
	subps	xmm6, xmm3
	addps	xmm0, xmm1
	addps	xmm2, xmm3
	mulps	xmm4, [esp+(4+516)]
	mulps	xmm6, [esp+(4+516)]
	movaps	xmm1, xmm0
	movaps	xmm3, xmm2
	unpcklps	xmm0, xmm4
	unpckhps	xmm1, xmm4
	unpcklps	xmm2, xmm6
	unpckhps	xmm3, xmm6

	movaps	[esp+(4+016)], xmm0
	movaps	[esp+(4+116)], xmm1
	movaps	[esp+(4+216)], xmm2
	movaps	[esp+(4+316)], xmm3
	movaps	[esp+(4+516)], xmm5
	movaps	[esp+(4+716)], xmm7

	movss	xmm0, [esp+(4+12)]
	movss	xmm1, [esp+(4+28)]
	movss	xmm2, [esp+(4+44)]
	movss	xmm3, [esp+(4+60)]
	addss	xmm0, [esp+(4+8)]
	addss	xmm1, [esp+(4+24)]
	addss	xmm2, [esp+(4+40)]
	addss	xmm3, [esp+(4+56)]
	movss	[esp+(4+8)], xmm0
	movss	[esp+(4+24)], xmm1
	movss	[esp+(4+40)], xmm2
	movss	[esp+(4+56)], xmm3
	movss	xmm0, [esp+(4+76)]
	movss	xmm1, [esp+(4+92)]
	movss	xmm2, [esp+(4+108)]
	movss	xmm3, [esp+(4+124)]
	addss	xmm0, [esp+(4+72)]
	addss	xmm1, [esp+(4+88)]
	addss	xmm2, [esp+(4+104)]
	addss	xmm3, [esp+(4+120)]
	movss	[esp+(4+72)], xmm0
	movss	[esp+(4+88)], xmm1
	movss	[esp+(4+104)], xmm2
	movss	[esp+(4+120)], xmm3

	movaps	xmm1, [esp+(4+16)]
	movaps	xmm3, [esp+(4+48)]
	movaps	xmm5, [esp+(4+80)]
	movaps	xmm7, [esp+(4+112)]
	movaps	xmm0, xmm1
	movaps	xmm2, xmm3
	movaps	xmm4, xmm5
	movaps	xmm6, xmm7
	shufps	xmm0, xmm0, 0x1e
	shufps	xmm2, xmm2, 0x1e
	shufps	xmm4, xmm4, 0x1e
	shufps	xmm6, xmm6, 0x1e
	andps	xmm0, [mask]
	andps	xmm2, [mask]
	andps	xmm4, [mask]
	andps	xmm6, [mask]
	addps	xmm1, xmm0
	addps	xmm3, xmm2
	addps	xmm5, xmm4
	addps	xmm7, xmm6

	movaps	xmm2, [esp+(4+32)]
	movaps	xmm6, [esp+(4+96)]
	movaps	xmm0, xmm2
	movaps	xmm4, xmm6
	shufps	xmm0, xmm0, 0x1e
	shufps	xmm4, xmm4, 0x1e
	andps	xmm0, [mask]
	andps	xmm4, [mask]
	addps	xmm2, xmm3
	addps	xmm3, xmm0
	addps	xmm6, xmm7
	addps	xmm7, xmm4

	movaps	xmm0, [esp+(4+0)]
	movaps	xmm4, [esp+(4+64)]

	movss	[ecx+1024], xmm0
	movss	[ecx+896], xmm2
	movss	[ecx+768], xmm1
	movss	[ecx+640], xmm3

	shufps	xmm0, xmm0, 0xe1
	shufps	xmm2, xmm2, 0xe1
	shufps	xmm1, xmm1, 0xe1
	shufps	xmm3, xmm3, 0xe1
	movss	[ecx], xmm0
	movss	[ebx], xmm0
	movss	[ebx+128], xmm2
	movss	[ebx+256], xmm1
	movss	[ebx+384], xmm3

	movhlps	xmm0, xmm0
	movhlps	xmm2, xmm2
	movhlps	xmm1, xmm1
	movhlps	xmm3, xmm3
	movss	[ecx+512], xmm0
	movss	[ecx+384], xmm2
	movss	[ecx+256], xmm1
	movss	[ecx+128], xmm3

	shufps	xmm0, xmm0, 0xe1
	shufps	xmm2, xmm2, 0xe1
	shufps	xmm1, xmm1, 0xe1
	shufps	xmm3, xmm3, 0xe1
	movss	[ebx+512], xmm0
	movss	[ebx+640], xmm2
	movss	[ebx+768], xmm1
	movss	[ebx+896], xmm3

	movaps	xmm0, xmm4
	shufps	xmm0, xmm0, 0x1e
	movaps	xmm1, xmm5
	andps	xmm0, [mask]

	addps	xmm4, xmm6
	addps	xmm5, xmm7
	addps	xmm6, xmm1
	addps	xmm7, xmm0

	movss	[ecx+960], xmm4
	movss	[ecx+832], xmm6
	movss	[ecx+704], xmm5
	movss	[ecx+576], xmm7
	movhlps	xmm0, xmm4
	movhlps	xmm1, xmm6
	movhlps	xmm2, xmm5
	movhlps	xmm3, xmm7
	movss	[ecx+448], xmm0
	movss	[ecx+320], xmm1
	movss	[ecx+192], xmm2
	movss	[ecx+64], xmm3

	shufps	xmm4, xmm4, 0xe1
	shufps	xmm6, xmm6, 0xe1
	shufps	xmm5, xmm5, 0xe1
	shufps	xmm7, xmm7, 0xe1
	movss	[ebx+64], xmm4
	movss	[ebx+192], xmm6
	movss	[ebx+320], xmm5
	movss	[ebx+448], xmm7

	shufps	xmm0, xmm0, 0xe1
	shufps	xmm1, xmm1, 0xe1
	shufps	xmm2, xmm2, 0xe1
	shufps	xmm3, xmm3, 0xe1
	movss	[ebx+576], xmm0
	movss	[ebx+704], xmm1
	movss	[ebx+832], xmm2
	movss	[ebx+960], xmm3

	pop	dword ebx
	mov	dword esp, ebp
	pop	dword ebp
        ret


