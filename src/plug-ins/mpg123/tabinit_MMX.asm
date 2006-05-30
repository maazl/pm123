# replaces make_decode_tables in tabinit.c and also produces a 16 bit
# integer version of decwin, that is decwins .

BITS 32

global decwin
global decwins
global conv16to8

segment .bss class=DATA use32 FLAT
        align 32
        decwin  resd  2176+32
        align 32
        decwins resd  2176+32

        align 32
        conv16to8_buf resb 4096
        align 32
        conv16to8 dd conv16to8_buf+2048

segment .data class=DATA use32 FLAT
        align 32

intwinbase:
        dw      0,    -1,    -1,    -1,    -1,    -1,    -1,    -2
        dw     -2,    -2,    -2,    -3,    -3,    -4,    -4,    -5
        dw     -5,    -6,    -7,    -7,    -8,    -9,   -10,   -11
        dw    -13,   -14,   -16,   -17,   -19,   -21,   -24,   -26
        dw    -29,   -31,   -35,   -38,   -41,   -45,   -49,   -53
        dw    -58,   -63,   -68,   -73,   -79,   -85,   -91,   -97
        dw   -104,  -111,  -117,  -125,  -132,  -139,  -147,  -154
        dw   -161,  -169,  -176,  -183,  -190,  -196,  -202,  -208
        dw   -213,  -218,  -222,  -225,  -227,  -228,  -228,  -227
        dw   -224,  -221,  -215,  -208,  -200,  -189,  -177,  -163
        dw   -146,  -127,  -106,   -83,   -57,   -29,     2,    36
        dw     72,   111,   153,   197,   244,   294,   347,   401
        dw    459,   519,   581,   645,   711,   779,   848,   919
        dw    991,  1064,  1137,  1210,  1283,  1356,  1428,  1498
        dw   1567,  1634,  1698,  1759,  1817,  1870,  1919,  1962
        dw   2001,  2032,  2057,  2075,  2085,  2087,  2080,  2063
        dw   2037,  2000,  1952,  1893,  1822,  1739,  1644,  1535
        dw   1414,  1280,  1131,   970,   794,   605,   402,   185
        dw    -45,  -288,  -545,  -814, -1095, -1388, -1692, -2006
        dw  -2330, -2663, -3004, -3351, -3705, -4063, -4425, -4788
        dw  -5153, -5517, -5879, -6237, -6589, -6935, -7271, -7597
        dw  -7910, -8209, -8491, -8755, -8998, -9219, -9416, -9585
        dw  -9727, -9838, -9916, -9959, -9966, -9935, -9863, -9750
        dw  -9592, -9389, -9139, -8840, -8492, -8092, -7640, -7134
        dw  -6574, -5959, -5288, -4561, -3776, -2935, -2037, -1082
        dw    -70,   998,  2122,  3300,  4533,  5818,  7154,  8540
        dw   9975, 11455, 12980, 14548, 16155, 17799, 19478, 21189
        dw  22929, 24694, 26482, 28289, 30112, 31947,-26209,-24360
        dw -22511,-20664,-18824,-16994,-15179,-13383,-11610, -9863
        dw  -8147, -6466, -4822, -3222, -1667,  -162,  1289,  2684
        dw   4019,  5290,  6494,  7629,  8692,  9679, 10590, 11420
        dw  12169, 12835, 13415, 13908, 14313, 14630, 14856, 14992
        dw  15038

intwindiv:
        dd 0x47800000                           ; 65536.0

segment .text class=CODE use32 FLAT

        align 32

global make_decode_tables
make_decode_tables:
        push    ebx
        push    edi
        push    esi

        xor ecx,ecx
        xor ebx,ebx
        mov esi, 32
        mov [esp+16],eax                        ; scaleval
        mov edi, intwinbase
        neg dword [esp+16]                      ; scaleval
        push dword 2                            ; intwinbase step
_L00:
        cmp     ecx,    528
        jnc     _L02
        movsx   eax,    WORD [edi]
        cmp     edi,    DWORD intwinbase+444
        jc      _L01
        add     eax,    60000
_L01:
        push    eax
        fild    DWORD [esp]
        fdiv    DWORD [intwindiv]
        fimul   DWORD [24+esp]
        pop     eax
        fst     DWORD [decwin+ecx*4]
        fstp    DWORD [decwin+64+ecx*4]
_L02:
        lea     edx,    [-1+esi]
        and     edx,    ebx
        cmp     edx,    31
        jnz     _L03
        add     ecx,    -1023
        test    ebx,    esi
        jz      _L03
        neg     DWORD [20+esp]
_L03:
        add     ecx,    esi
        add     edi,    DWORD [esp]
        inc     ebx
        cmp     edi,    DWORD intwinbase
        jz      _L04
        cmp     ebx,    256
        jnz     _L00
        neg     DWORD [esp]
        jmp     _L00
_L04:
        pop     eax

        xor     ecx,    ecx
        xor     ebx,    ebx
        push    DWORD 2
_L05:
        cmp     ecx,    528
        jnc     _L11
        movsx   eax,    WORD [edi]
        cmp     edi,    DWORD intwinbase+444
        jc      _L06
        add     eax,    60000
_L06:
        cdq
        imul    DWORD [20+esp]
        shrd    eax,    edx,    17
        cmp     eax,    32767
        mov     edx,    1055
        jle     _L07
        mov     eax,    32767
        jmp     _L08
_L07:
        cmp     eax,    -32767
        jge     _L08
        mov     eax,    -32767
_L08:
        cmp     ecx,    512
        jnc     _L09
        sub     edx,    ecx
        mov     WORD [decwins+edx*2],   ax
        mov     WORD [decwins-32+edx*2],        ax
_L09:
        test    ecx,    1
        jnz     _L10
        neg     eax
_L10:
        mov     WORD [decwins+ecx*2],   ax
        mov     WORD [decwins+32+ecx*2],        ax
_L11:
        lea     edx,    [-1+esi]
        and     edx,    ebx
        cmp     edx,    31
        jnz     _L12
        add     ecx,    -1023
        test    ebx,    esi
        jz      _L12
        neg     DWORD [20+esp]
_L12:
        add     ecx,    esi
        add     edi,    DWORD [esp]
        inc     ebx
        cmp     edi,    DWORD intwinbase
        jz      _L13
        cmp     ebx,    256
        jnz     near _L05
        neg     DWORD [esp]
        jmp     _L05
_L13:
        pop     eax

        pop     esi
        pop     edi
        pop     ebx
        ret
