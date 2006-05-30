BITS 32

extern dct64
extern decwin

segment .data align=16 class=DATA use32 FLAT

        bo   dd 01h

segment .bss align=16 class=DATA use32 FLAT

        buffs times 01100h db 0

segment .text align=16 class=CODE use32 FLAT

        align 16

; 89 {

global synth_1to1_pent
synth_1to1_pent:

        push    ebp
        push    ebx
        push    edi
        push    esi
        sub     esp,32

; 99 {
        mov dword [esp+12],0; clear clip
        mov esi,ecx; samples
        xor edi,edi

        mov     ebp,dword [bo]

; 112   if(!channel) {
        cmp edx,edi;    channel
        jne BLBL17

; 113     bo--;
        dec     ebp;

; 114     bo &= 0xf;
        and     ebp,0fh
        mov     dword [bo],ebp

; 115     buf = buffs[0];
        mov     ecx,buffs
        jmp     BLBL18
        align   04h
BLBL17:

; 118     samples++;
        add     esi,02h;       samples

; 119     buf = buffs[1];
        mov     ecx,buffs+0880h
BLBL18:

; 122   if(bo & 0x1) {
        test    ebp,1
        je      BLBL19
        mov     edi,eax; bandPtr
        mov     ebx,ecx; b0
        mov     [esp+24],ebp; bo1
        lea     edx,[ecx+ebp*04h]
        inc     ebp
        and     ebp,0fh
        lea     eax,[ecx+ebp*04h+1088]
        mov     ecx,edi
        jmp     BLBL21
        align   04h
BLBL19:
        mov     edi,eax; bandPtr
        lea     ebx,[ecx+1088]; b0
        lea     eax,[ecx+ebp*04h]
        inc     ebp
        mov     [esp+24],ebp; bo1
        lea     edx,[ecx+ebp*04h+1088]
        mov     ecx,edi
BLBL21:
; 130     dct64(buf[0]+bo,buf[1]+bo+1,bandPtr);
        call    dct64

BLBL20:

; 135     real *window = decwin + 16 - bo1;
        mov     edx,[esp+24]; bo1
        lea     edx,[edx*4]
        mov     edi,decwin+040h wrt FLAT
        sub     edi,edx

; 137     for (j=16;j;j--,b0+=0x10,window+=0x20,samples+=step)
        mov     ebp,010h;      j
        align 010h
BLBL22:

        fld     dword [edi]
        fmul    dword [ebx]
        fld     dword [edi+04h]
        fmul    dword [ebx+04h]
        fxch    st1
        fld     dword [edi+08h]
        fmul    dword [ebx+08h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi+0ch]
        fmul    dword [ebx+0ch]
        fxch    st2
        faddp   st1,st0
        fld     dword [edi+010h]
        fmul    dword [ebx+010h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi+014h]
        fmul    dword [ebx+014h]
        fxch    st2
        faddp   st1,st0
        fld     dword [edi+018h]
        fmul    dword [ebx+018h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi+01ch]
        fmul    dword [ebx+01ch]
        fxch    st2
        faddp   st1,st0
        fld     dword [edi+020h]
        fmul    dword [ebx+020h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi+024h]
        fmul    dword [ebx+024h]
        fxch    st2
        faddp   st1,st0
        fld     dword [edi+028h]
        fmul    dword [ebx+028h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi+02ch]
        fmul    dword [ebx+02ch]
        fxch    st2
        faddp   st1,st0
        fld     dword [edi+030h]
        fmul    dword [ebx+030h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi+034h]
        fmul    dword [ebx+034h]
        fxch    st2
        faddp   st1,st0
        fld     dword [edi+038h]
        fmul    dword [ebx+038h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi+03ch]
        fmul    dword [ebx+03ch]
        fxch    st2
        faddp   st1,st0
        fxch    st1
        fsubp   st1,st0

; 159       WRITE_SAMPLE(samples,sum,clip);
        fistp   dword [esp+28];    CBE7
        mov     eax,[esp+28];  CBE7
        cmp     eax,07fffh
        jg      BLBL23
        cmp     eax,0ffff8000h
        jl      BLBL24
        mov     [esi],ax
        jmp     BLBL26
BLBL23:
        mov     word [esi],07fffh
        jmp     BLBL25
BLBL24:
        mov     word [esi],08000h
        jmp     BLBL25
BLBL25:
        inc     dword [esp+12]; clip
BLBL26:

; 137     for (j=16;j;j--,b0+=0x10,window+=0x20,samples+=step)
        add     ebx,040h
        add     edi,080h
        add     esi,04h
        dec     ebp
        jne     near BLBL22

; 164       sum  = window[0x0] * b0[0x0];
        fld     dword [edi]
        fmul    dword [ebx]

; 165       sum += window[0x2] * b0[0x2];
        fld     dword [edi+08h]
        fmul    dword [ebx+08h]

; 166       sum += window[0x4] * b0[0x4];
        fld     dword [edi+010h]
        fmul    dword [ebx+010h]
        fxch    st2

; 165       sum += window[0x2] * b0[0x2];
        faddp   st1,st0

; 167       sum += window[0x6] * b0[0x6];
        fld     dword [edi+018h]
        fmul    dword [ebx+018h]
        fxch    st2

; 166       sum += window[0x4] * b0[0x4];
        faddp   st1,st0

; 168       sum += window[0x8] * b0[0x8];
        fld     dword [edi+020h]
        fmul    dword [ebx+020h]
        fxch    st2

; 167       sum += window[0x6] * b0[0x6];
        faddp   st1,st0

; 169       sum += window[0xA] * b0[0xA];
        fld     dword [edi+028h]
        fmul    dword [ebx+028h]
        fxch    st2

; 168       sum += window[0x8] * b0[0x8];
        faddp   st1,st0

; 170       sum += window[0xC] * b0[0xC];
        fld     dword [edi+030h]
        fmul    dword [ebx+030h]
        fxch    st2

; 169       sum += window[0xA] * b0[0xA];
        faddp   st1,st0

; 171       sum += window[0xE] * b0[0xE];
        fld     dword [edi+038h]
        fmul    dword [ebx+038h]
        fxch    st2

; 170       sum += window[0xC] * b0[0xC];
        faddp   st1,st0
        fxch    st1

; 171       sum += window[0xE] * b0[0xE];
        faddp   st1,st0

; 159       WRITE_SAMPLE(samples,sum,clip);
        fistp   dword [esp+28];    CBE7
        mov     eax,[esp+28];  CBE7
        cmp     eax,07fffh
        jg      BLBL28
        cmp     eax,0ffff8000h
        jl      BLBL29
        mov     [esi],ax
        jmp     BLBL31
BLBL28:
        mov     word [esi],07fffh
        jmp     BLBL30
BLBL29:
        mov     word [esi],08000h
        jmp     BLBL30
BLBL30:
        inc     dword [esp+12];  clip
BLBL31:

        sub     ebx,040h; b0
        sub     edi,080h; window
        add     esi,04h; samples
        mov     edx,[esp+24]; bo1
        lea     edi,[edi+edx*08h]; window

; 180     for (j=15;j;j--,b0-=0x10,window-=0x20,samples+=step)
        mov     ebp, 0fh;       j
        align 010h
BLBL33:

        fld     dword [edi-04h];    CBE7
        fchs
        fmul    dword [ebx]
        fld     dword [edi-08h]
        fmul    dword [ebx+04h]
        fxch    st1
        fld     dword [edi-0ch]
        fmul    dword [ebx+08h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi-010h]
        fmul    dword [ebx+0ch]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi-014h]
        fmul    dword [ebx+010h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi-018h]
        fmul    dword [ebx+014h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi-01ch]
        fmul    dword [ebx+018h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi-020h]
        fmul    dword [ebx+01ch]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi-024h]
        fmul    dword [ebx+020h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi-028h]
        fmul    dword [ebx+024h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi-02ch]
        fmul    dword [ebx+028h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi-030h]
        fmul    dword [ebx+02ch]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi-034h]
        fmul    dword [ebx+030h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi-038h]
        fmul    dword [ebx+034h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi-03ch]
        fmul    dword [ebx+038h]
        fxch    st2
        fsubp   st1,st0
        fld     dword [edi]
        fmul    dword [ebx+03ch]
        fxch    st2
        fsubp   st1,st0
        fxch    st1
        fsubp   st1,st0

; 159       WRITE_SAMPLE(samples,sum,clip);
        fistp   dword [esp+28];    CBE7
        mov     eax,[esp+28];  CBE7
        cmp     eax,07fffh
        jg      BLBL35
        cmp     eax,0ffff8000h
        jl      BLBL36
        mov     [esi],ax
        jmp     BLBL38
BLBL35:
        mov     word [esi],07fffh
        jmp     BLBL37
BLBL36:
        mov     word [esi],08000h
        jmp     BLBL37
BLBL37:
        inc     dword [esp+12]; clip
BLBL38:

; 180     for (j=15;j;j--,b0-=0x10,window-=0x20,samples+=step)
        sub     ebx,040h; b0
        sub     edi,080h; window
        add     esi,04h;  samples
        dec     ebp;      j
        jne     near BLBL33

; 206   return clip;
        mov     eax,dword [esp+12]; clip
        add     esp,32
        pop     esi
        pop     edi
        pop     ebx
        pop     ebp
        ret     

