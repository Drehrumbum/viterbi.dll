comment!#######################################################################
#                           This file is part of                              #
#           'viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)'           #
#                                                                             #
#  decon_avx.asm                                                              #
#                                                                             #
#  Slightly modified assembly-language version based on the compiler's output #
#  There is a lot of pressure on the registers, but the XOR-constants and the #
#  metrics are held in the registers during the mainloop.                     #
#                                                                             #
#                                                                             #
#  (c) 2022-23 Heiko Vogel <hevog@gmx.de>                                     #
#                                                                             #
##############################################################################!

.nolist
include const.inc
include sehmac.inc
include chainback.inc
.list


hevo segment align(64) 'CODE'
ShadowSpace = 0
LocalSpace = DECISIONS_ARRAY_SIZE
UseVex = 1
NumLongOps = 4
align 64

decon_avx proc frame

    SaveRegs xmm6, xmm7, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14,\
             xmm15, rdi, rsi

    mov            r10d,  ecx
    xor            eax,   eax
    add            ecx,   6
    vmovdqa        xmm2,  m128_63_0
    vmovdqa        xmm13, m128_63
    shr            ecx,   1
    vmovdqa        xmm10, m128_1st_XOR_0_3_4_7
    vmovdqa        xmm11, m128_2nd_XOR_0_3_4_7
    vmovdqa        xmm12, m128_XOR_1_5
    vmovdqa        xmm9,  m128_XOR_2_6
    vmovdqa        xmm15, xmm13
    vmovdqa        xmm0,  xmm13
    vmovdqa        xmm14, xmm15


mainloop:
    sub            ecx,  1
    js             chainback

    vpxor          xmm7, xmm7, xmm7
    imul           rdi,  qword ptr [rdx + 2*rax + 16], 01010101h
    vmovd          xmm1, dword ptr [rdx + 2*rax]
    vmovd          xmm3, dword ptr [rdx + 2*rax + 4]
    vpshufb        xmm1, xmm1, xmm7
    vmovd          xmm4, dword ptr [rdx + 2*rax + 8]
    vpshufb        xmm3, xmm3, xmm7
    vmovd          xmm5, dword ptr [rdx + 2*rax + 12]
    vpshufb        xmm4, xmm4, xmm7
    vpshufb        xmm5, xmm5, xmm7
    imul           rsi,  qword ptr [rdx + 2*rax + 24], 01010101h
    vpxor          xmm6, xmm10, xmm1
    vpxor          xmm3, xmm12, xmm3
    vpavgb         xmm6, xmm6, xmm3
    vpxor          xmm8, xmm9, xmm4
    vpxor          xmm7, xmm10, xmm5
    vpavgb         xmm4, xmm8, xmm7
    vpavgb         xmm4, xmm6, xmm4
    vpsrlw         xmm4, xmm4, 2
    vpand          xmm4, xmm13, xmm4
    vpsubusb       xmm6, xmm13, xmm4
    vpaddusb       xmm7, xmm2, xmm4
    vpaddusb       xmm2, xmm2, xmm6
    vpaddusb       xmm6, xmm0, xmm6
    vpaddusb       xmm0, xmm0, xmm4
    vpminub        xmm4, xmm6, xmm7
    vpminub        xmm2, xmm0, xmm2
    vpcmpeqb       xmm6, xmm6, xmm4
    vpcmpeqb       xmm0, xmm0, xmm2
    vpunpcklbw     xmm7, xmm6, xmm0
    vpmovmskb      r11d, xmm7
    vpunpckhbw     xmm0, xmm6, xmm0
    vpmovmskb      r8d,  xmm0
    vpxor          xmm0, xmm11, xmm1
    vpxor          xmm1, xmm11, xmm5
    vpavgb         xmm0, xmm0, xmm3
    vpavgb         xmm1, xmm8, xmm1
    vpavgb         xmm0, xmm0, xmm1
    vpsrlw         xmm0, xmm0, 2
    vpand          xmm0, xmm13, xmm0
    vpsubusb       xmm1, xmm13, xmm0
    vpaddusb       xmm5, xmm15, xmm0
    vpaddusb       xmm6, xmm15, xmm1
    vpaddusb       xmm1, xmm14, xmm1
    vpaddusb       xmm0, xmm14, xmm0
    vpminub        xmm5, xmm1, xmm5
    vpminub        xmm6, xmm0, xmm6
    vpcmpeqb       xmm7, xmm1, xmm5
    vmovq          xmm15, rsi          ; syms 6 and 7
    vpcmpeqb       xmm0, xmm0, xmm6
    vmovq          xmm14, rdi          ; syms 4 and 5
    vpunpcklbw     xmm1, xmm5, xmm6
    vpunpckhbw     xmm8, xmm5, xmm6
    vpshufd        xmm3, xmm14, 055h   ; sym 5
    vpunpcklbw     xmm5, xmm7, xmm0
    vpunpckhbw     xmm0, xmm7, xmm0
    vpshufd        xmm14, xmm14, 0     ; sym 4
    vpmovmskb      esi,  xmm5
    vpmovmskb      edi,  xmm0
    vpunpcklbw     xmm5, xmm4, xmm2
    vpunpckhbw     xmm2, xmm4, xmm2
    vpxor          xmm3, xmm12, xmm3
    vpxor          xmm0, xmm10, xmm14
    vpshufd        xmm4, xmm15, 0      ; sym 6
    vpshufd        xmm15, xmm15, 055h  ; sym 7
    vpavgb         xmm0, xmm0, xmm3
    vpxor          xmm6, xmm9, xmm4
    vpxor          xmm7, xmm10, xmm15
    vpavgb         xmm4, xmm6, xmm7
    vpavgb         xmm0, xmm0, xmm4
    vpsrlw         xmm0, xmm0, 2
    vpand          xmm0, xmm13, xmm0
    mov            dword ptr [rsp + rax    ], r11d
    mov            dword ptr [rsp + rax + 2], r8d
    mov            dword ptr [rsp + rax + 4], esi
    mov            dword ptr [rsp + rax + 6], edi

    vpsubusb       xmm4, xmm13, xmm0
    vpaddusb       xmm7, xmm5, xmm0
    vpaddusb       xmm5, xmm5, xmm4
    add            eax, 16
    vpaddusb       xmm4, xmm1, xmm4
    vpaddusb       xmm0, xmm1, xmm0
    vpminub        xmm1, xmm4, xmm7
    vpcmpeqb       xmm7, xmm4, xmm1
    vmovd          esi,  xmm1
    vpminub        xmm4, xmm0, xmm5
    vpcmpeqb       xmm0, xmm0, xmm4
    cmp            sil,  RENORMALIZE_THRESHOLD
    vpunpcklbw     xmm5, xmm7, xmm0
    vpmovmskb      r8d,  xmm5
    vpunpckhbw     xmm0, xmm7, xmm0
    vpmovmskb      edi,  xmm0
    vpxor          xmm0, xmm11, xmm14
    vpavgb         xmm0, xmm0, xmm3
    vpxor          xmm3, xmm11, xmm15
    vpavgb         xmm3, xmm6, xmm3
    vpavgb         xmm0, xmm0, xmm3
    vpsrlw         xmm0, xmm0, 2
    vpand          xmm0, xmm13, xmm0
    vpsubusb       xmm3, xmm13, xmm0
    vpaddusb       xmm5, xmm2, xmm0
    vpaddusb       xmm2, xmm2, xmm3
    vpaddusb       xmm3, xmm8, xmm3
    vpaddusb       xmm0, xmm8, xmm0
    vpminub        xmm5, xmm3, xmm5
    vpminub        xmm2, xmm0, xmm2
    vpcmpeqb       xmm3, xmm3, xmm5
    vpunpckhbw     xmm14, xmm5, xmm2
    vpcmpeqb       xmm6, xmm0, xmm2
    vpunpcklbw     xmm0, xmm5, xmm2
    vpunpcklbw     xmm5, xmm3, xmm6
    vpunpckhbw     xmm2, xmm3, xmm6
    vpmovmskb      esi,  xmm5
    vpmovmskb      r11d, xmm2
    mov            dword ptr [rsp + rax - 8], r8d
    mov            dword ptr [rsp + rax - 6], edi
    mov            dword ptr [rsp + rax - 4], esi
    mov            dword ptr [rsp + rax - 2], r11d
    vpunpcklbw     xmm2,  xmm1, xmm4
    vpunpckhbw     xmm15, xmm1, xmm4
    jb             mainloop

    vpsubusb       xmm0,  xmm0,  xmm13
    vpsubusb       xmm14, xmm14, xmm13
    vpsubusb       xmm2,  xmm2,  xmm13
    vpsubusb       xmm15, xmm15, xmm13
    jmp            mainloop

    Chainback_mac
    RestoreRegs
    xor            eax, eax
    ret
decon_avx endp
hevo ends
end