comment!#######################################################################
#                           This file is part of                              #
#           'viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)'           #
#                                                                             #
#  decon_sse2_lut32.asm                                                       #
#                                                                             #
#  Slightly modified assembly-language version based on the compiler's output #
#  This version uses a lookup-table with 256 pre-calculated 32-bit integers.  #
#                                                                             #
#  2024-07-24:                                                                #
#  Replacing the MOV instructions for reading the symbols with MOVZX to       #
#  prevent QIRX from crashing under very rare conditions. (Symbols > 0xFF)    #
#                                                                             #
#  (c) 2022-24 Heiko Vogel <hevog@gmx.de>                                     #
#                                                                             #
##############################################################################!

.nolist
include const.inc
include sehmac.inc
include chainback.inc
.list

extern symbols32LUT : ptr dword 


_TEXT$sse2 segment align(64)
ShadowSpace = 0
LocalSpace = DECISIONS_ARRAY_SIZE
UseVex = 0 
NumLongOps = 2

decon_sse2_lut32 proc frame
    SaveRegs xmm6, xmm7, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13,\
             xmm14, xmm15, rdi, rsi, rbx, rbp

    mov         rbp,   symbols32LUT
    mov         r10d,  ecx  ; save framebits
    xor         eax,   eax
    add         ecx,   6
    movdqa      xmm7,  m128_63_0
    movdqa      xmm12, m128_63
    shr         ecx,   1
    movdqa      xmm10, m128_1st_XOR_0_3_4_7
    movdqa      xmm11, m128_2nd_XOR_0_3_4_7
    movdqa      xmm8,  m128_XOR_1_5
    movdqa      xmm9,  m128_XOR_2_6
    movdqa      xmm13, xmm12
    movdqa      xmm2,  xmm12
    movdqa      xmm5,  xmm12

mainloop:
    sub         ecx,  1
    js          chainback

    movzx       ebx,   byte ptr  [rdx + 2 * rax]
    movzx       edi,   byte ptr  [rdx + 2 * rax + 4]
    movd        xmm3,  dword ptr [rbp + 4 * rbx]
    movd        xmm15, dword ptr [rbp + 4 * rdi]
    pshufd      xmm3,  xmm3,  0
    pshufd      xmm15, xmm15, 0 
    movzx       esi,   byte ptr [rdx + 2 * rax + 8]
    movdqa      xmm0,  xmm3
    movd        xmm6,  dword ptr [rbp + 4 * rsi]
    movzx       r11d,  byte ptr [rdx + 2 * rax + 12]
    pshufd      xmm6,  xmm6,  0
    movd        xmm4,  dword ptr [rbp + 4 * r11]
    pxor        xmm15, xmm8
    pshufd      xmm4,  xmm4,  0
    movdqa      xmm1,  xmm4
    pxor        xmm6,  xmm9
    pxor        xmm1,  xmm10
    pavgb       xmm1,  xmm6
    pxor        xmm0,  xmm10
    pavgb       xmm0,  xmm15
    pxor        xmm4,  xmm11
    movdqa      xmm14, xmm2
    pavgb       xmm1,  xmm0
    pxor        xmm3,  xmm11
    psrlw       xmm1,  2
    movdqa      xmm0,  xmm12
    pand        xmm1,  xmm12
    psubusb     xmm0,  xmm1
    paddusb     xmm14, xmm0
    pavgb       xmm3,  xmm15
    paddusb     xmm0,  xmm7
    paddusb     xmm7,  xmm1
    paddusb     xmm1,  xmm2
    pminub      xmm7,  xmm14

; give a little bit time between the reads (if possible)
    movzx       ebx,   byte ptr [rdx + 2 * rax + 24] ;6
    pavgb       xmm4,  xmm6
    pminub      xmm0,  xmm1
    pcmpeqb     xmm14, xmm7
    pcmpeqb     xmm1,  xmm0
    movdqa      xmm2,  xmm14
    punpckhbw   xmm14, xmm1
    pmovmskb    edi,   xmm14
    punpcklbw   xmm2,  xmm1
    movd        xmm14, dword ptr [rbp + 4 * rbx] ;6 
    pmovmskb    r8d,   xmm2
    pavgb       xmm4,  xmm3
    movdqa      xmm2,  xmm12
    psrlw       xmm4,  2
    movdqa      xmm3,  xmm5
    pand        xmm4,  xmm12
    movzx       ebx,   byte ptr [rdx + 2 * rax + 20] ;5
    movdqa      xmm6,  xmm13
    psubusb     xmm2,  xmm4
    paddusb     xmm3,  xmm2
    paddusb     xmm2,  xmm13
    paddusb     xmm6,  xmm4
    paddusb     xmm4,  xmm5
    movd        xmm13, dword ptr [rbp + 4 * rbx] ;5
    pminub      xmm6,  xmm3
    pshufd      xmm14, xmm14, 0
    pcmpeqb     xmm3,  xmm6
    pminub      xmm2,  xmm4
    movdqa      xmm5,  xmm3
    movzx       ebx,   byte ptr [rdx + 2 * rax + 16] ;4
    pcmpeqb     xmm4,  xmm2
    movdqa      xmm15, xmm7
    punpcklbw   xmm5,  xmm4
    pmovmskb    esi,   xmm5
    pshufd      xmm13, xmm13, 0
    movd        xmm5,  dword ptr [rbp + 4 * rbx] ;4
    punpckhbw   xmm3,  xmm4
    pmovmskb    r11d,  xmm3
    movzx       ebx,   byte ptr [rdx + 2 * rax + 28] ;7
    punpckhbw   xmm15, xmm0
    movdqa      xmm4,  xmm6
    punpckhbw   xmm6,  xmm2 
    punpcklbw   xmm4,  xmm2
    movdqa      xmm3,  xmm7
    movd        xmm2,  dword ptr [rbp + 4 * rbx] ;7
    punpcklbw   xmm3,  xmm0
    pshufd      xmm5,  xmm5, 0
    pxor        xmm14, xmm9 
    movdqa      xmm0,  xmm5
    pshufd      xmm2,  xmm2, 0
    pxor        xmm0,  xmm10 
    movdqa      xmm7,  xmm2
    pxor        xmm13, xmm8
    pxor        xmm7,  xmm10
    pavgb       xmm0,  xmm13
    pavgb       xmm7,  xmm14
    pxor        xmm5,  xmm11
    pavgb       xmm7,  xmm0
    pxor        xmm2,  xmm11
    psrlw       xmm7,  2
    pavgb       xmm2,  xmm14
    mov         [rsp + rax    ], r8d
    mov         [rsp + rax + 2], edi
    mov         [rsp + rax + 4], esi
    mov         [rsp + rax + 6], r11d
    pand        xmm7,  xmm12
    movdqa      xmm0,  xmm12 
    add         eax,   16
    movdqa      xmm1,  xmm4
    psubusb     xmm0,  xmm7
    pavgb       xmm5,  xmm13
    paddusb     xmm1,  xmm0
    paddusb     xmm0,  xmm3
    paddusb     xmm3,  xmm7
    pavgb       xmm2,  xmm5
    paddusb     xmm7,  xmm4
    pminub      xmm3,  xmm1
    psrlw       xmm2,  2
    movd        edi,   xmm3
    pcmpeqb     xmm1,  xmm3
    pminub      xmm0,  xmm7
    cmp         dil,   RENORMALIZE_THRESHOLD
    movdqa      xmm4,  xmm1
    pcmpeqb     xmm7,  xmm0
    pand        xmm2,  xmm12
    punpcklbw   xmm4,  xmm7
    pmovmskb    ebx,   xmm4 
    punpckhbw   xmm1,  xmm7
    pmovmskb    edi,   xmm1
    movdqa      xmm4,  xmm12
    psubusb     xmm4,  xmm2
    movdqa      xmm1,  xmm6
    movdqa      xmm5,  xmm15
    paddusb     xmm1,  xmm4
    paddusb     xmm5,  xmm2
    movdqa      xmm7,  xmm3
    paddusb     xmm4,  xmm15
    paddusb     xmm2,  xmm6
    pminub      xmm5,  xmm1
    pminub      xmm4,  xmm2
    pcmpeqb     xmm1,  xmm5
    pcmpeqb     xmm2,  xmm4
    movdqa      xmm6,  xmm1
    punpcklbw   xmm7,  xmm0
    movdqa      xmm13, xmm3
    punpcklbw   xmm6,  xmm2
    pmovmskb    esi,   xmm6
    punpckhbw   xmm1,  xmm2
    pmovmskb    r11d,  xmm1
    movdqa      xmm2,  xmm5
    punpckhbw   xmm13, xmm0
    mov         [rsp + rax - 8], ebx
    mov         [rsp + rax - 6], edi
    mov         [rsp + rax - 4], esi
    mov         [rsp + rax - 2], r11d
    punpcklbw   xmm2,  xmm4
    punpckhbw   xmm5,  xmm4
    jb          mainloop

    psubusb     xmm7,  xmm12
    psubusb     xmm13, xmm12
    psubusb     xmm2,  xmm12
    psubusb     xmm5,  xmm12 
    jmp         mainloop

    Chainback_mac
    xor eax, eax
    RestoreRegs
    ret

decon_sse2_lut32 endp
_TEXT$sse2 ends
end