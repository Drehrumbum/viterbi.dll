comment!#######################################################################
#                           This file is part of                              #
#           'viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)'           #
#                                                                             #
#  decon_ssse3.asm                                                            #
#                                                                             #
#  Slightly modified SSSE3 assembly version from the compiler's output.       #
#                                                                             #
#  (c) 2022 Heiko Vogel <hevog@gmx.de>                                        #
#                                                                             #
##############################################################################!

.nolist
include const.inc
include sehmac.inc
include chainback.inc
.list



_TEXT$ssse3 segment align(64)
ShadowSpace = 0
LocalSpace = DECISIONS_ARRAY_SIZE
UseVex = 0
NumLongOps = 5

decon_ssse3 proc frame

    SaveRegs xmm6, xmm7, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13,\
             xmm14, xmm15, rsi, rdi

    mov         r10d,  ecx
    xor         eax,   eax
    add         ecx,   6
    movdqa      xmm14, m128_63_0
    movdqa      xmm13, m128_63
    shr         ecx,   1
    movdqa      xmm11, m128_1st_XOR_0_3_4_7
    movdqa      xmm10, m128_2nd_XOR_0_3_4_7
    movdqa      xmm8,  m128_XOR_1_5
    movdqa      xmm9,  m128_XOR_2_6
    pxor        xmm12, xmm12
    movdqa      xmm15, xmm13
    movdqa      xmm6,  xmm13
    movdqa      xmm7,  xmm13


mainloop:
    sub         ecx,  1
    js          chainback

    movd        xmm0,  dword ptr [rdx + 2*rax]
    movdqa      xmm5,  xmm11
    pshufb      xmm0,  xmm12
    movd        xmm2,  dword ptr [rdx + 2*rax + 4]
    movdqa      xmm1,  xmm11
    pshufb      xmm2,  xmm12
    pxor        xmm5,  xmm0
    movd        xmm3,  dword ptr [rdx + 2*rax + 8]
    pxor        xmm0,  xmm10
    pshufb      xmm3,  xmm12
    pxor        xmm2,  xmm8
    movd        xmm4,  dword ptr [rdx + 2*rax + 12]
    pxor        xmm3,  xmm9
    pshufb      xmm4,  xmm12
    pavgb       xmm5,  xmm2
    pxor        xmm1,  xmm4
    pavgb       xmm0,  xmm2
    pavgb       xmm1,  xmm3
    pxor        xmm4,  xmm10
    pavgb       xmm1,  xmm5
    movdqa      xmm2,  xmm13
    psrlw       xmm1,  2
    pavgb       xmm4,  xmm3
    pand        xmm1,  xmm13
    pavgb       xmm4,  xmm0
    psubusb     xmm2,  xmm1
    movdqa      xmm3,  xmm14
    movdqa      xmm0,  xmm6
    paddusb     xmm3,  xmm1
    paddusb     xmm0,  xmm2
    paddusb     xmm2,  xmm14
    paddusb     xmm1,  xmm6
    pminub      xmm3,  xmm0
    pminub      xmm2,  xmm1
    pcmpeqb     xmm0,  xmm3
    pcmpeqb     xmm1,  xmm2
    movdqa      xmm6,  xmm0
    psrlw       xmm4,  2
    punpcklbw   xmm6,  xmm1
    pand        xmm4,  xmm13
    movdqa      xmm5,  xmm13
    punpckhbw   xmm0,  xmm1
    pmovmskb    esi,   xmm6
    psubusb     xmm5,  xmm4
    pmovmskb    edi,   xmm0
    movdqa      xmm14, xmm3
    movdqa      xmm0,  xmm7
    movdqa      xmm6,  xmm15
    paddusb     xmm0,  xmm5
    paddusb     xmm6,  xmm4
    paddusb     xmm5,  xmm15
    paddusb     xmm4,  xmm7
    pminub      xmm6,  xmm0
    pminub      xmm5,  xmm4
    pcmpeqb     xmm0,  xmm6
    pcmpeqb     xmm4,  xmm5
    movdqa      xmm1,  xmm0
    movdqa      xmm7,  xmm3
    punpcklbw   xmm1,  xmm4
    punpckhbw   xmm0,  xmm4
    punpckhbw   xmm14, xmm2
    pmovmskb    r8d,   xmm1
    movdqa      xmm15, xmm6
    pmovmskb    r11d,  xmm0
    punpcklbw   xmm7,  xmm2
    punpcklbw   xmm15, xmm5
    punpckhbw   xmm6,  xmm5
    movd        xmm2,  dword ptr [rdx + 2*rax + 16]
    movdqa      xmm0,  xmm11
    pshufb      xmm2,  xmm12
    movd        xmm3,  dword ptr [rdx + 2*rax + 20]
    movdqa      xmm1,  xmm11
    pshufb      xmm3,  xmm12
    pxor        xmm0,  xmm2
    movd        xmm5,  dword ptr [rdx + 2*rax + 24]
    pxor        xmm3,  xmm8
    pshufb      xmm5,  xmm12
    movd        xmm4,  dword ptr [rdx + 2*rax + 28]
    pavgb       xmm0,  xmm3
    pshufb      xmm4,  xmm12
    pxor        xmm5,  xmm9
    pxor        xmm1,  xmm4
    pxor        xmm2,  xmm10
    pavgb       xmm1,  xmm5
    pxor        xmm4,  xmm10
    pavgb       xmm1,  xmm0
    pavgb       xmm2,  xmm3
    psrlw       xmm1,  2
    pavgb       xmm4,  xmm5
    pand        xmm1,  xmm13
    pavgb       xmm4,  xmm2
    movdqa      xmm5,  xmm15
    movdqa      xmm2,  xmm13
    movdqa      xmm3,  xmm7
    mov         dword ptr [rsp + rax], esi
    mov         dword ptr [rsp + rax + 2], edi
    mov         dword ptr [rsp + rax + 4], r8d
    mov         dword ptr [rsp + rax + 6], r11d
    psubusb     xmm2,  xmm1
    paddusb     xmm3,  xmm1
    add         eax,   16
    paddusb     xmm5,  xmm2
    paddusb     xmm2,  xmm7
    paddusb     xmm1,  xmm15
    pminub      xmm3,  xmm5
    pminub      xmm2,  xmm1
    pcmpeqb     xmm5,  xmm3
    pcmpeqb     xmm1,  xmm2
    movd        edi,   xmm3
    movdqa      xmm7,  xmm5
    psrlw       xmm4,  2
    cmp         dil,   RENORMALIZE_THRESHOLD
    punpcklbw   xmm7,  xmm1
    punpckhbw   xmm5,  xmm1
    pand        xmm4,  xmm13
    movdqa      xmm0,  xmm13
    pmovmskb    esi,   xmm7
    movdqa      xmm15, xmm3
    psubusb     xmm0,  xmm4
    pmovmskb    edi,   xmm5
    movdqa      xmm7,  xmm14
    movdqa      xmm1,  xmm6
    paddusb     xmm7,  xmm4
    paddusb     xmm1,  xmm0
    paddusb     xmm4,  xmm6
    paddusb     xmm0,  xmm14
    pminub      xmm7,  xmm1
    pminub      xmm0,  xmm4
    pcmpeqb     xmm1,  xmm7
    pcmpeqb     xmm4,  xmm0
    movdqa      xmm5,  xmm1
    punpcklbw   xmm5,  xmm4
    pmovmskb    r8d,   xmm5
    punpckhbw   xmm1,  xmm4
    movdqa      xmm6,  xmm7
    movdqa      xmm14, xmm3
    pmovmskb    r11d,  xmm1
    mov         dword ptr [rsp + rax - 8], esi
    mov         dword ptr [rsp + rax - 6], edi
    mov         dword ptr [rsp + rax - 4], r8d
    mov         dword ptr [rsp + rax - 2], r11d
    punpcklbw   xmm14, xmm2
    punpcklbw   xmm6,  xmm0
    punpckhbw   xmm7,  xmm0
    punpckhbw   xmm15, xmm2
    jb          mainloop

    psubusb     xmm14, xmm13
    psubusb     xmm15, xmm13
    psubusb     xmm6,  xmm13
    psubusb     xmm7,  xmm13
    jmp         mainloop

    Chainback_mac
    xor eax, eax
    RestoreRegs
    ret

decon_ssse3 endp
_TEXT$ssse3 ends
end