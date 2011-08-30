;  1 "getcpuflags.S"
;  1 "<built-in>"
;  1 "<command line>"
;  1 "getcpuflags.S"
;  18 "getcpuflags.S"
;  1 "mangle.h" 1
;  13 "mangle.h"
;  1 "config.h" 1
;  14 "mangle.h" 2
;  1 "intsym.h" 1
;  15 "mangle.h" 2
;  19 "getcpuflags.S" 2

section .text
align 2

global	_INT123_getcpuflags

_INT123_getcpuflags
	push	dword ebp
	mov	dword ebp, esp
	push	dword edx
	push	dword ecx
	push	dword ebx
	push	dword esi

	mov	dword esi, [ebp+8]

	mov	dword eax, 0x80000000

	pushfd
	pushfd
	pop	dword eax
	mov	dword ebx, eax

	xor	dword eax, 0x00200000
	push	dword eax
	popfd

	pushfd
	pop	dword eax
	popfd
	cmp	dword eax, ebx
	je	_Lnocpuid


	mov	dword [esi+12], 0x0

	mov	dword eax, 0x80000000
        cpuid

	cmp	dword eax, 0x80000001
	jb	_Lnoextended

	mov	dword eax, 0x80000001
        cpuid
	mov	dword [esi+12], edx
_Lnoextended

	mov	dword eax, 0x00000001
        cpuid
	mov	dword [esi], eax
	mov	dword [esi+4], ecx
	mov	dword [esi+8], edx
	jmp	dword _Lend
align 2
_Lnocpuid

	mov	dword eax, 0
	mov	dword [esi], 0
	mov	dword [esi+4], 0
	mov	dword [esi+8], 0
	mov	dword [esi+12], 0
align 2
_Lend

	pop	dword esi
	pop	dword ebx
	pop	dword ecx
	pop	dword edx
	mov	dword esp, ebp
	pop	dword ebp
        ret


