BITS 32

%ifdef UNDERSCORE
  %define decwins _decwins
%endif

extern dct64_MMX
extern decwins

segment .text class=CODE use32 FLAT

        align 32

; thanks to AMD!!
global detect_MMX
detect_MMX:
	push ebx
        pushfd                  ; save EFLAGS
        pop  eax                ; store EFLAGS in EAX
        mov  edx, eax           ; save in EDX for later testing
        xor  eax, 00200000h     ; toggle bit 21
        push eax                ; put to stack
        popfd                   ; save changed EAX to EFLAGS
        pushfd                  ; push EFLAGS to TOS
        pop eax                 ; store EFLAGS in EAX
        cmp eax, edx            ; see if bit 21 has changed
        jz NO_CPUID             ; if no change, no CPUID

        mov eax,1               ; setup function 1
        CPUID                   ; call the function
        test edx, 800000        ; test 23rd bit
        jnz YES_MMX             ; multimedia technology supported

NO_MMX:
NO_CPUID:
        xor eax,eax
        jmp return

YES_MMX:
        mov eax, 1
return:
        pop ebx
        ret


        align 32

global synth_1to1_MMX
synth_1to1_MMX:
        push    ebp
        push    ebx
        push    edi
        push    esi

        ; for _Optlink calling convention
        mov     DWORD [20+esp], eax     ; synth_data
        mov     DWORD [24+esp], edx     ; bandPtr
        mov     DWORD [28+esp], ecx     ; channel

        mov     ecx, DWORD [28+esp]     ; channel
        mov     edi, DWORD [32+esp]     ; out
        mov     ebx, 15
        mov     edx, [20+esp]           ; synth_data.bo
        lea     edi, [edi+ecx*2]
        dec     ecx
        mov     esi, [20+esp]           ; synth_data
        add     esi, 4                  ; synth_data.buffs 
        mov     eax, DWORD [edx]
        jecxz   _L1
        dec     eax
        and     eax, ebx
        lea     esi, [1088+esi]
        mov     DWORD [edx], eax
_L1:
        lea     edx, [esi+eax*2]
        mov     ebp, eax
        inc     eax
        push    DWORD [24+esp]          ; bandPtr
        and     eax, ebx
        lea     ecx, [544+esi+eax*2]
        inc     ebx
        test    eax, 1
        jnz     _L2
        xchg    ecx, edx
        inc     ebp
        lea     esi, [544+esi]
_L2:
        push    edx
        push    ecx

        ; for _Optlink calling convention
        mov     eax, [esp]
        mov     edx, [esp+4]
        mov     ecx, [esp+8]

        call    dct64_MMX
        add     esp, 12
        lea     ecx, [1+ebx]
        sub     ebx, ebp

        lea     edx, [decwins+ebx+ebx*1 wrt FLAT]
_L3:
        movq    mm0, [edx]
        pmaddwd mm0, [esi]
        movq    mm1, [8+edx]
        pmaddwd mm1, [8+esi]
        movq    mm2, [16+edx]
        pmaddwd mm2, [16+esi]
        movq    mm3, [24+edx]
        pmaddwd mm3, [24+esi]
        paddd   mm0, mm1
        paddd   mm0, mm2
        paddd   mm0, mm3
        movq    mm1, mm0
        psrlq   mm1, 32
        paddd   mm0, mm1
        psrad   mm0, 13
        packssdw mm0, mm0
        movd    eax, mm0
        mov     WORD [edi], ax

        lea     esi, [32+esi]
        lea     edx, [64+edx]
        lea     edi, [4+edi]
        loop    _L3

        sub     esi, 64
        mov     ecx, 15
_L4:
        movq    mm0, [edx]
        pmaddwd mm0, [esi]
        movq    mm1, [8+edx]
        pmaddwd mm1, [8+esi]
        movq    mm2, [16+edx]
        pmaddwd mm2, [16+esi]
        movq    mm3, [24+edx]
        pmaddwd mm3, [24+esi]
        paddd   mm0, mm1
        paddd   mm0, mm2
        paddd   mm0, mm3
        movq    mm1, mm0
        psrlq   mm1, 32
        paddd   mm1, mm0
        psrad   mm1, 13
        packssdw mm1, mm1
        psubd   mm0, mm0
        psubsw  mm0, mm1
        movd    eax, mm0
        mov     WORD [edi], ax

        sub     esi, 32
        add     edx, 64
        lea     edi, [4+edi]
        loop    _L4
        emms

        pop     esi
        pop     edi
        pop     ebx
        pop     ebp
        xor     eax,eax
        ret
