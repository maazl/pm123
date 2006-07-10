 ;
 ; Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 ;                     Taneli Lepp„ <rosmo@sektori.com>
 ;
 ; Redistribution and use in source and binary forms, with or without
 ; modification, are permitted provided that the following conditions are met:
 ;
 ;    1. Redistributions of source code must retain the above copyright notice,
 ;       this list of conditions and the following disclaimer.
 ;
 ;    2. Redistributions in binary form must reproduce the above copyright
 ;       notice, this list of conditions and the following disclaimer in the
 ;       documentation and/or other materials provided with the distribution.
 ;
 ;    3. The name of the author may not be used to endorse or promote products
 ;       derived from this software without specific prior written permission.
 ;
 ; THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 ; WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 ; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 ; EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 ; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 ; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 ; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 ; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 ; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 ; ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ;

%ifdef UNDERSCORE
  %define FIRorder    _FIRorder
  %define finalFIRmmx _finalFIRmmx
  %define finalAMPi   _finalAMPi
%endif

BITS 32

extern finalFIRmmx
extern finalAMPi
extern FIRorder

extern DosBeep

%imacro beep 0
  push eax
  push ecx
  push edx
  push dword 10
  push dword 600
  mov al, 2
  call DosBeep
  add esp,8
  pop edx
  pop ecx
  pop eax
%endmacro

%define MAX_ORDER 4100

segment .data align=16 class=DATA use32 FLAT

;        temp times 32768*2 dw 0
;        format db "packed %d %d %d %d %d %d %d %d ",10,0
;        format2 db "unpacked %d %d %d %d %d %d %d %d ",10,0
;        one db "one %d ",10,0

segment .text align=16 class=CODE use32 FLAT


        align 16

; thanks to AMD!!
global detect_mmx
detect_mmx:
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


        align 16

;void filter_samples_mmx_stereo(short *newsamples, short *temp, char *buf, int len);
;eax == newsamples
;edx == temp
;ecx == buf
;esp+16 == len
global filter_samples_mmx_stereo
filter_samples_mmx_stereo:
        push esi
        push edi
        push ebx
        push ebp

        ;-- unpacking new samples from buffers
        ; we sorta assume len > 0
        ; We use MMX unpack to at the very least put two samples
        ; of the same channel one after another.  MMX unpack was not made
        ; for this so it unpacks it a bit weirdly for our purposes but
        ; it's much faster than doing it manually.  That is why finalFIRmmx
        ; look so screwed.
        ;
        ; From a few sample such as A1 A2 B1 B2 C1 C2 D1 D2 in memory,
        ; this code will shoot out  A1 C1 A2 C2 B1 D1 B2 D2 in memory also

        xor esi,esi
        mov edi,[esp+32]                        ; len
                                                ; ecx == buf
        mov ebx,[FIRorder wrt FLAT]
        lea ebx,[edx+ebx*4]                     ; temp + FIRorder*4

        align 010h
.nextcopy:
        movq mm0,[ecx+esi]                      ; two packed chunks
        movq mm1,[ecx+esi+8]
        movq mm2, mm0

        punpcklwd mm0, mm1
        punpckhwd mm2, mm1

        movq [ebx+esi],mm0                      ; two unpacked chunks
        movq [ebx+esi+8],mm2

        add     esi, 16
        cmp     edi,esi                         ; i < len
        jg      .nextcopy


; 316       while(i < len)
        mov ecx, edi
        xor edi, edi                            ; i

        align 010h
.@BLBL56:

        ; -- applying filter
        ; 1/4 of the filter is done with 20 bit shifting
        ; the next 1/2 is done with 15 bit shifting
        ; and the rest is done with 20 bit shifting also.
        ; Since this is stereo, there is two samples per chunk of 8 bytes
        ; so that's why with MMX you only need to do FIRorder/2 loops
        ; per sample
        ; Also, four samples are generated on each loop because
        ; that way you can use aligned filter data padded with zeroes for MMX

        xor esi,esi                   ; j
        mov ebx,[FIRorder wrt FLAT]
        shr ebx,3                     ; FIRorder/8  first 1/4
        lea ebp,[edx+edi]             ; temp+i

        movq mm4,[ebp]
        movq mm5,mm4
        movq mm6,mm5
        movq mm7,mm6

        pmaddwd mm4,[finalFIRmmx                 wrt FLAT]
        pmaddwd mm5,[finalFIRmmx + MAX_ORDER*4   wrt FLAT]
        pmaddwd mm6,[finalFIRmmx + MAX_ORDER*4*2 wrt FLAT]
        pmaddwd mm7,[finalFIRmmx + MAX_ORDER*4*3 wrt FLAT]

        psrad mm4,1
        psrad mm5,1
        psrad mm6,1
        psrad mm7,1

        inc esi

        align 010h
.nextcoef:
        movq mm0,[ebp+esi*8]
        movq mm1,mm0
        movq mm2,mm1
        movq mm3,mm2

        pmaddwd mm0,[finalFIRmmx                 + esi*8 wrt FLAT]
        pmaddwd mm1,[finalFIRmmx + MAX_ORDER*4   + esi*8 wrt FLAT]
        pmaddwd mm2,[finalFIRmmx + MAX_ORDER*4*2 + esi*8 wrt FLAT]
        pmaddwd mm3,[finalFIRmmx + MAX_ORDER*4*3 + esi*8 wrt FLAT]

        psrad mm0,1
        psrad mm1,1
        psrad mm2,1
        psrad mm3,1

        paddd mm4,mm0
        paddd mm5,mm1
        paddd mm6,mm2
        paddd mm7,mm3

        inc esi
        cmp     ebx,esi;        ; i < FIRorder/8
        jg      .nextcoef

        ; -- going to the last coeficients
        add esi, ebx
        add esi, ebx
        mov ebx,[FIRorder wrt FLAT]
        shr ebx,1                     ; FIRorder/2  last 1/4

        align 010h
.nextcoef2:
        movq mm0,[ebp+esi*8]
        movq mm1,mm0
        movq mm2,mm1
        movq mm3,mm2

        pmaddwd mm0,[finalFIRmmx                 + esi*8 wrt FLAT]
        pmaddwd mm1,[finalFIRmmx + MAX_ORDER*4   + esi*8 wrt FLAT]
        pmaddwd mm2,[finalFIRmmx + MAX_ORDER*4*2 + esi*8 wrt FLAT]
        pmaddwd mm3,[finalFIRmmx + MAX_ORDER*4*3 + esi*8 wrt FLAT]

        psrad mm0,1
        psrad mm1,1
        psrad mm2,1
        psrad mm3,1

        paddd mm4,mm0
        paddd mm5,mm1
        paddd mm6,mm2
        paddd mm7,mm3

        inc esi
        cmp     ebx,esi;        ; i <= FIRorder/2
        jge     .nextcoef2

        ; -- preparing to sum with values shifted by 15 only
        psrad mm4,5
        psrad mm5,5
        psrad mm6,5
        psrad mm7,5


        ; -- going to the middle coeficients
        mov ebx,[FIRorder wrt FLAT]
        shr ebx,3
        mov esi, ebx
        imul ebx,3                    ; 3*FIRorder/8  middle 1/2

        align 010h
.nextcoef3:
        movq mm0,[ebp+esi*8]
        movq mm1,mm0
        movq mm2,mm1
        movq mm3,mm2

        pmaddwd mm0,[finalFIRmmx                 + esi*8 wrt FLAT]
        pmaddwd mm1,[finalFIRmmx + MAX_ORDER*4   + esi*8 wrt FLAT]
        pmaddwd mm2,[finalFIRmmx + MAX_ORDER*4*2 + esi*8 wrt FLAT]
        pmaddwd mm3,[finalFIRmmx + MAX_ORDER*4*3 + esi*8 wrt FLAT]

        psrad mm0,1
        psrad mm1,1
        psrad mm2,1
        psrad mm3,1

        paddd mm4,mm0
        paddd mm5,mm1
        paddd mm6,mm2
        paddd mm7,mm3

        inc esi
        cmp     ebx,esi;        ; i < 3*FIRorder/8
        jg      .nextcoef3


        psrad mm4,14
        psrad mm5,14
        psrad mm6,14
        psrad mm7,14

        ; -- final pack
        packssdw mm4,mm5
        packssdw mm6,mm7


        ; -- multiply by the amplification variable
        movq mm0,[finalAMPi wrt FLAT]

        movq mm5, mm4           ; keeping copy
        pmulhw mm4, mm0         ; to multiply high
        pmullw mm5, mm0         ; and low values
        movq mm1, mm5           ; copy the high values
        punpckhwd mm5,mm4       ; to unpack them with low values
        punpcklwd mm1,mm4       ; and get them all
        psrad mm5,12            ; shifting by 12 (finalAMPi is 12 shifted)
        psrad mm1,12
        packssdw mm1,mm5        ; and pack saturate again

        movq mm7, mm6
        pmulhw mm6, mm0
        pmullw mm7, mm0
        movq mm2, mm7
        punpckhwd mm7,mm6
        punpcklwd mm2,mm6
        psrad mm7,12
        psrad mm2,12
        packssdw mm2,mm7

        ; -- storing final result in memory
        movq [eax + edi    ],mm1    ; newsamples + i
        movq [eax + edi+08h],mm2

; 348          i++;
        add     edi,16

; 316       while(i < len)
        cmp     ecx,edi
        jg      near .@BLBL56

        emms
        pop     ebp
        pop     ebx
        pop     edi
        pop     esi
        ret

;void filter_samples_mmx_mono(short *newsamples, short *temp, char *buf, int len);
;eax == newsamples
;edx == temp
;ecx == buf
;esp+16 == len
global filter_samples_mmx_mono
filter_samples_mmx_mono:
        push esi
        push edi
        push ebx
        push ebp

        ;-- copying new samples from buffers
        ; we sorta assume len > 0

        xor esi,esi
        mov edi,[esp+32]                        ; len
                                                ; ecx == buf
        mov ebx,[FIRorder wrt FLAT]
        lea ebx,[edx+ebx*2]                     ; temp + FIRorder*2

        align 010h
.nextcopy:
        movq mm0,[ecx+esi]                      ; a couple of chunks
        movq mm1,[ecx+esi+8]
        movq [ebx+esi],mm0                      ; back in temp
        movq [ebx+esi+8],mm1

        add     esi, 16
        cmp     edi,esi                         ; i < len
        jg      .nextcopy


; 316       while(i < len)
        mov ecx, edi
        xor edi, edi                            ; i

        align 010h
.@BLBL56:

        ; -- applying filter
        ; 1/4 of the filter is done with 20 bit shifting
        ; the next 1/2 is done with 15 bit shifting
        ; and the rest is done with 20 bit shifting also.
        ; Since this is mono, there is four samples per chunk of 8 bytes
        ; so that's why with MMX you only need to do FIRorder/4 loops
        ; per sample
        ; Also, four samples are generated on each loop because
        ; that way you can use aligned filter data padded with zeroes for MMX

        xor esi,esi                   ; j
        mov ebx,[FIRorder wrt FLAT]
        shr ebx,4                     ; FIRorder/16  first 1/4
        lea ebp,[edx+edi]             ; temp+i

        movq mm4,[ebp]
        movq mm5,mm4
        movq mm6,mm5
        movq mm7,mm6

        pmaddwd mm4,[finalFIRmmx                 wrt FLAT]
        pmaddwd mm5,[finalFIRmmx + MAX_ORDER*4   wrt FLAT]
        pmaddwd mm6,[finalFIRmmx + MAX_ORDER*4*2 wrt FLAT]
        pmaddwd mm7,[finalFIRmmx + MAX_ORDER*4*3 wrt FLAT]

        psrad mm4,1
        psrad mm5,1
        psrad mm6,1
        psrad mm7,1

        inc esi

        align 010h
.nextcoef:
        movq mm0,[ebp+esi*8]
        movq mm1,mm0
        movq mm2,mm1
        movq mm3,mm2

        pmaddwd mm0,[finalFIRmmx                 + esi*8 wrt FLAT]
        pmaddwd mm1,[finalFIRmmx + MAX_ORDER*4   + esi*8 wrt FLAT]
        pmaddwd mm2,[finalFIRmmx + MAX_ORDER*4*2 + esi*8 wrt FLAT]
        pmaddwd mm3,[finalFIRmmx + MAX_ORDER*4*3 + esi*8 wrt FLAT]

        psrad mm0,1
        psrad mm1,1
        psrad mm2,1
        psrad mm3,1

        paddd mm4,mm0
        paddd mm5,mm1
        paddd mm6,mm2
        paddd mm7,mm3

        inc esi
        cmp     ebx,esi;        ; i < FIRorder/16
        jg      .nextcoef

        ; -- going to the last coeficients
        add esi, ebx
        add esi, ebx
        mov ebx,[FIRorder wrt FLAT]
        shr ebx,2                     ; FIRorder/4  last 1/4

        align 010h
.nextcoef2:
        movq mm0,[ebp+esi*8]
        movq mm1,mm0
        movq mm2,mm1
        movq mm3,mm2

        pmaddwd mm0,[finalFIRmmx                 + esi*8 wrt FLAT]
        pmaddwd mm1,[finalFIRmmx + MAX_ORDER*4   + esi*8 wrt FLAT]
        pmaddwd mm2,[finalFIRmmx + MAX_ORDER*4*2 + esi*8 wrt FLAT]
        pmaddwd mm3,[finalFIRmmx + MAX_ORDER*4*3 + esi*8 wrt FLAT]

        psrad mm0,1
        psrad mm1,1
        psrad mm2,1
        psrad mm3,1

        paddd mm4,mm0
        paddd mm5,mm1
        paddd mm6,mm2
        paddd mm7,mm3

        inc esi
        cmp     ebx,esi;        ; i <= FIRorder/4
        jge     .nextcoef2

        ; -- preparing to sum with values shifted by 15 only
        psrad mm4,5
        psrad mm5,5
        psrad mm6,5
        psrad mm7,5


        ; -- going to the middle coeficients
        mov ebx,[FIRorder wrt FLAT]
        shr ebx,4
        mov esi, ebx
        imul ebx,3                    ; 3*FIRorder/16  middle 1/2

        align 010h
.nextcoef3:
        movq mm0,[ebp+esi*8]
        movq mm1,mm0
        movq mm2,mm1
        movq mm3,mm2

        pmaddwd mm0,[finalFIRmmx                 + esi*8 wrt FLAT]
        pmaddwd mm1,[finalFIRmmx + MAX_ORDER*4   + esi*8 wrt FLAT]
        pmaddwd mm2,[finalFIRmmx + MAX_ORDER*4*2 + esi*8 wrt FLAT]
        pmaddwd mm3,[finalFIRmmx + MAX_ORDER*4*3 + esi*8 wrt FLAT]

        psrad mm0,1
        psrad mm1,1
        psrad mm2,1
        psrad mm3,1

        paddd mm4,mm0
        paddd mm5,mm1
        paddd mm6,mm2
        paddd mm7,mm3

        inc esi
        cmp     ebx,esi;        ; i < 3*FIRorder/16
        jg      .nextcoef3

        movq mm2, mm4
        punpckldq mm4, mm5
        punpckhdq mm2, mm5
        paddd mm2, mm4

        movq mm3, mm6
        punpckldq mm6, mm7
        punpckhdq mm3, mm7
        paddd mm3, mm6

        psrad mm2,14
        psrad mm3,14

        ; -- final pack
        packssdw mm2,mm3

        ; -- multiply by the amplification variable
        movq     mm4,mm2
        movq mm0,[finalAMPi wrt FLAT]

        movq mm5, mm4           ; keeping copy
        pmulhw mm4, mm0         ; to multiply high
        pmullw mm5, mm0         ; and low values
        movq mm1, mm5           ; copy the high values
        punpckhwd mm5,mm4       ; to unpack them with low values
        punpcklwd mm1,mm4       ; and get them all
        psrad mm5,12            ; shifting by 12 (finalAMPi is 12 shifted)
        psrad mm1,12
        packssdw mm1,mm5        ; and pack saturate again

        ; -- storing final result in memory
        movq [eax + edi    ],mm1    ; newsamples + i

; 348          i++;
        add     edi,8

; 316       while(i < len)
        cmp     ecx,edi
        jg      near .@BLBL56

        emms
        pop     ebp
        pop     ebx
        pop     edi
        pop     esi
        ret

