;  1 "tabinit_mmx.S"
;  1 "<built-in>"
;  1 "<command line>"
;  1 "tabinit_mmx.S"
;  9 "tabinit_mmx.S"
;  1 "mangle.h" 1
;  13 "mangle.h"
;  1 "config.h" 1
;  14 "mangle.h" 2
;  1 "intsym.h" 1
;  15 "mangle.h" 2
;  10 "tabinit_mmx.S" 2

section .data
align 5
global	_INT123_costab_mmxsse
_INT123_costab_mmxsse
 dd 1056974725
 dd 1057056395
 dd 1057223771
 dd 1057485416
 dd 1057855544
 dd 1058356026
 dd 1059019886
 dd 1059897405
 dd 1061067246
 dd 1062657950
 dd 1064892987
 dd 1066774581
 dd 1069414683
 dd 1073984175
 dd 1079645762
 dd 1092815430
 dd 1057005197
 dd 1057342072
 dd 1058087743
 dd 1059427869
 dd 1061799040
 dd 1065862217
 dd 1071413542
 dd 1084439708
 dd 1057128951
 dd 1058664893
 dd 1063675095
 dd 1076102863
 dd 1057655764
 dd 1067924853
 dd 1060439283
align 5
intwinbase
 dd 0, -1, -1, -1, -1, -1, -1, -2
 dd -2, -2, -2, -3, -3, -4, -4, -5
 dd -5, -6, -7, -7, -8, -9, -10, -11
 dd -13, -14, -16, -17, -19, -21, -24, -26
 dd -29, -31, -35, -38, -41, -45, -49, -53
 dd -58, -63, -68, -73, -79, -85, -91, -97
 dd -104, -111, -117, -125, -132, -139, -147, -154
 dd -161, -169, -176, -183, -190, -196, -202, -208
 dd -213, -218, -222, -225, -227, -228, -228, -227
 dd -224, -221, -215, -208, -200, -189, -177, -163
 dd -146, -127, -106, -83, -57, -29, 2, 36
 dd 72, 111, 153, 197, 244, 294, 347, 401
 dd 459, 519, 581, 645, 711, 779, 848, 919
 dd 991, 1064, 1137, 1210, 1283, 1356, 1428, 1498
 dd 1567, 1634, 1698, 1759, 1817, 1870, 1919, 1962
 dd 2001, 2032, 2057, 2075, 2085, 2087, 2080, 2063
 dd 2037, 2000, 1952, 1893, 1822, 1739, 1644, 1535
 dd 1414, 1280, 1131, 970, 794, 605, 402, 185
 dd -45, -288, -545, -814, -1095, -1388, -1692, -2006
 dd -2330, -2663, -3004, -3351, -3705, -4063, -4425, -4788
 dd -5153, -5517, -5879, -6237, -6589, -6935, -7271, -7597
 dd -7910, -8209, -8491, -8755, -8998, -9219, -9416, -9585
 dd -9727, -9838, -9916, -9959, -9966, -9935, -9863, -9750
 dd -9592, -9389, -9139, -8840, -8492, -8092, -7640, -7134
 dd -6574, -5959, -5288, -4561, -3776, -2935, -2037, -1082
 dd -70, 998, 2122, 3300, 4533, 5818, 7154, 8540
 dd 9975, 11455, 12980, 14548, 16155, 17799, 19478, 21189
 dd 22929, 24694, 26482, 28289, 30112, 31947,-26209,-24360
 dd -22511,-20664,-18824,-16994,-15179,-13383,-11610, -9863
 dd -8147, -6466, -4822, -3222, -1667, -162, 1289, 2684
 dd 4019, 5290, 6494, 7629, 8692, 9679, 10590, 11420
 dd 12169, 12835, 13415, 13908, 14313, 14630, 14856, 14992
 dd 15038

intwindiv
 dd 0x47800000  ;  65536.0
section .text
align 5

global	_INT123_make_decode_tables_mmx_asm
_INT123_make_decode_tables_mmx_asm
	push	dword edi
	push	dword esi
	push	dword ebx


	xor	dword ecx, ecx
	xor	dword ebx, ebx
	mov	dword esi, 32
	mov	dword edi, intwinbase
	neg	dword [esp+16]
	push	dword 2

_L00
	cmp	dword ecx, 528
	jnc	_L02
	movsx	eax, word [edi]

	cmp	dword edi, intwinbase+444
	jc	_L01
	add	dword eax, 60000
_L01
	push	dword eax

	fild	qword [esp]
	fdiv	dword [intwindiv]
	fimul	dword [esp+24]

	mov	dword eax, [esp+28]
	fst	dword [eax+ecx*4]
	fstp	dword [eax+ecx*4+64]
	pop	dword eax

_L02
	lea	dword edx, [esi+-1]
	and	dword edx, ebx
	cmp	dword edx, 31
	jnz	_L03
	add	dword ecx, -1023
	test	dword ebx, esi
	jz	_L03
	neg	dword [esp+20]
_L03
	add	dword ecx, esi
	add	dword edi, [esp]
	inc	dword ebx
	cmp	dword edi, intwinbase
	jz	_L04
	cmp	dword ebx, 256
	jnz	_L00
	neg	dword [esp]
	jmp	dword _L00
_L04
	pop	dword eax

	xor	dword ecx, ecx
	xor	dword ebx, ebx
	push	dword 2
_L05
	cmp	dword ecx, 528
	jnc	_L11
	movsx	eax, word [edi]

	cmp	dword edi, intwinbase+444
	jc	_L06
	add	dword eax, 60000
_L06
	cdq
	imul	dword [esp+20]
	shrd	dword eax, edx, 17
	cmp	dword eax, 32767
	mov	dword edx, 1055
	jle	_L07
	mov	dword eax, 32767
	jmp	dword _L08
_L07
	cmp	dword eax, -32767
	jge	_L08
	mov	dword eax, -32767
_L08

	push	dword ebx

	mov	dword ebx, [esp+32]
	cmp	dword ecx, 512
	jnc	_L09
	sub	dword edx, ecx
	mov	word [ebx+edx*2], ax
	mov	word [ebx+edx*2+-32], ax
_L09
	test	dword ecx, 1
	jnz	_L10
	neg	dword eax
_L10
	mov	word [ebx+ecx*2], ax
	mov	word [ebx+ecx*2+32], ax
	pop	dword ebx
_L11
	lea	dword edx, [esi+-1]
	and	dword edx, ebx
	cmp	dword edx, 31
	jnz	_L12
	add	dword ecx, -1023
	test	dword ebx, esi
	jz	_L12
	neg	dword [esp+20]
_L12
	add	dword ecx, esi
	add	dword edi, [esp]
	inc	dword ebx
	cmp	dword edi, intwinbase
	jz	_L13
	cmp	dword ebx, 256
	jnz	_L05
	neg	dword [esp]
	jmp	dword _L05
_L13
	pop	dword eax

	pop	dword ebx
	pop	dword esi
	pop	dword edi
        ret


