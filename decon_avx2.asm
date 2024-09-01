comment!#######################################################################
#                           This file is part of                              #
#           'viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)'           #
#                                                                             #
#  decon_avx2.asm                                                             #
#                                                                             #
#  Slightly modified assembly-language version based on the compiler's output #
#                                                                             #
#                                                                             #
#  (c) 2022-24 Heiko Vogel <hevog@gmx.de>                                     #
#                                                                             #
##############################################################################!

.nolist
include const.inc
include sehmac.inc
include chainback.inc
.list



_TEXT$avx2 segment align(64)
ShadowSpace = 0
LocalSpace = DECISIONS_ARRAY_SIZE
UseVex = 1
NumLongOps = 4

decon_avx2 proc frame

    SaveRegs xmm6, xmm7, xmm8, xmm9, xmm10, xmm11, rdi, rsi

    mov            r10d,  ecx
    xor            eax,   eax
    add            ecx,   6
    vpbroadcastb   ymm0,  byte ptr [m128_63]
    vmovdqa        ymm4,  m256_63_0
    shr            ecx,   1
    vmovdqa        ymm1,  m256_XOR_0_3_4_7
    vmovdqa        ymm2,  m256_XOR_1_5
    vmovdqa        ymm3,  m256_XOR_2_6
    vmovdqa        ymm5,  ymm0

mainloop:
    sub            ecx,   1
    js             chainback

    vpbroadcastb   ymm6,  byte ptr [rdx + 2*rax]
    vpbroadcastb   ymm7,  byte ptr [rdx + 2*rax +  4]
    vpbroadcastb   ymm8,  byte ptr [rdx + 2*rax +  8]
    vpbroadcastb   ymm9,  byte ptr [rdx + 2*rax + 12]
    vpbroadcastb   ymm10, byte ptr [rdx + 2*rax + 16]
    vpbroadcastb   ymm11, byte ptr [rdx + 2*rax + 20]
    vpxor          ymm6,  ymm6, ymm1
    vpxor          ymm7,  ymm7, ymm2
    vpavgb         ymm6,  ymm6, ymm7
    vpxor          ymm7,  ymm8, ymm3
    vpxor          ymm8,  ymm9, ymm1
    vpavgb         ymm7,  ymm8, ymm7
    vpbroadcastb   ymm8,  byte ptr [rdx + 2*rax + 24]
    vpbroadcastb   ymm9,  byte ptr [rdx + 2*rax + 28]
    vpavgb         ymm6,  ymm6,  ymm7
    vpxor          ymm7,  ymm10, ymm1
    vpxor          ymm10, ymm11, ymm2
    add            eax,   16
    vpavgb         ymm7,  ymm10, ymm7
    vpxor          ymm8,  ymm8,  ymm3
    vpxor          ymm9,  ymm9,  ymm1
    vpavgb         ymm8,  ymm8,  ymm9
    vpavgb         ymm7,  ymm8,  ymm7
    vpsrlw         ymm6,  ymm6,  2
    vpsrlw         ymm7,  ymm7,  2
    vpand          ymm6,  ymm6,  ymm0
    vpsubusb       ymm8,  ymm0,  ymm6
    vpand          ymm7,  ymm7,  ymm0
    vpsubusb       ymm9,  ymm0,  ymm7
    vpaddusb       ymm10, ymm4,  ymm6
    vpaddusb       ymm11, ymm8,  ymm5
    vpaddusb       ymm4,  ymm8,  ymm4
    vpaddusb       ymm5,  ymm5,  ymm6
    vpminub        ymm6,  ymm11, ymm10
    vpminub        ymm4,  ymm5,  ymm4
    vpcmpeqb       ymm8,  ymm11, ymm6
    vpcmpeqb       ymm5,  ymm5,  ymm4
    vpunpcklbw     ymm10, ymm6,  ymm4
    vpermq         ymm10, ymm10, 216
    vpaddusb       ymm11, ymm10, ymm7
    vpunpckhbw     ymm4,  ymm6,  ymm4
    vpermq         ymm4,  ymm4,  216
    vpaddusb       ymm6,  ymm9,  ymm4
    vpaddusb       ymm9,  ymm10, ymm9
    vpaddusb       ymm4,  ymm4,  ymm7
    vpminub        ymm7,  ymm11, ymm6
    vpcmpeqb       ymm6,  ymm6,  ymm7
    vmovd          esi,   xmm7
    vpminub        ymm9,  ymm9,  ymm4
    vpcmpeqb       ymm4,  ymm9,  ymm4
    cmp            sil,   RENORMALIZE_THRESHOLD
    vpunpcklbw     ymm10, ymm8,  ymm5
    vpmovmskb      edi,   ymm10
    mov            dword ptr [rsp + rax - 16], edi
    vpunpckhbw     ymm5,  ymm8,  ymm5
    vpmovmskb      esi,   ymm5
    mov            dword ptr [rsp + rax - 12], esi
    vpunpcklbw     ymm5,  ymm6,  ymm4
    vpmovmskb      edi,   ymm5
    mov            dword ptr [rsp + rax -  8], edi
    vpunpckhbw     ymm4,  ymm6,  ymm4
    vpmovmskb      esi,   ymm4
    mov            dword ptr [rsp + rax -  4], esi
    vpunpcklbw     ymm4,  ymm7,  ymm9
    vpermq         ymm4,  ymm4,  216
    vpunpckhbw     ymm5,  ymm7,  ymm9
    vpermq         ymm5,  ymm5,  216
    jb             mainloop

    vpsubusb       ymm4,  ymm4,  ymm0
    vpsubusb       ymm5,  ymm5,  ymm0
    jmp            mainloop

    Chainback_mac
    xor eax, eax
    RestoreRegs
   ; vzeroupper
    ret
decon_avx2 endp

_TEXT$avx2 ends
end