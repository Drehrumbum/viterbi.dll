comment!#######################################################################
#                           This file is part of                              #
#           'viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)'           #
#                                                                             #
#  decon_avx5.asm                                                             #
#                                                                             #
#  Slightly modified assembly-language version based on the compiler's output #
#  This code is currently disabled, because I can't test it.                  #
#                                                                             #
#  (c) 2023 Heiko Vogel <hevog@gmx.de>                                        #
#                                                                             #
##############################################################################!

.nolist
include const.inc
include sehmac.inc
include chainback.inc
.list


_TEXT$avx5 segment align(64)
ShadowSpace = 0
LocalSpace = DECISIONS_ARRAY_SIZE
UseVex = 1
NumLongOps = 0
org $ +30h ; mainloop stays at cache-line after merging
decon_avx5 proc frame

    SaveRegs rdi, rsi

    mov            r10d,  ecx
    add            rcx,   6
    vpbroadcastb   ymm0,  byte ptr m128_63
    vmovdqa        ymm4,  m256_63_0
    xor            rax,   rax
    shr            rcx,   1
    vmovdqa        ymm1,  m256_XOR_0_3_4_7
    vmovdqa        ymm2,  m256_XOR_1_5
    vmovdqa        ymm3,  m256_XOR_2_6
    vmovdqa        ymm5,  ymm0
    NOP_7


mainloop:
    sub            ecx,  1
    js             chainback

    vpbroadcastb   ymm16, byte ptr [rdx + 2*rax]
    vpbroadcastb   ymm17, byte ptr [rdx + 2*rax + 4]
    vpbroadcastb   ymm18, byte ptr [rdx + 2*rax + 8]
    vpbroadcastb   ymm19, byte ptr [rdx + 2*rax + 12]
    vpxorq         ymm16, ymm16, ymm1
    vpxorq         ymm17, ymm17, ymm2
    vpbroadcastb   ymm20, byte ptr [rdx + 2*rax + 16]
    vpbroadcastb   ymm21, byte ptr [rdx + 2*rax + 20]
    vpavgb         ymm16, ymm16, ymm17
    vpxorq         ymm17, ymm18, ymm3
    vpxorq         ymm18, ymm19, ymm1
    vpavgb         ymm17, ymm17, ymm18
    vpbroadcastb   ymm18, byte ptr [rdx + 2*rax + 24]
    vpbroadcastb   ymm19, byte ptr [rdx + 2*rax + 28]
    vpavgb         ymm16, ymm16, ymm17
    vpxorq         ymm17, ymm20, ymm1
    vpxorq         ymm20, ymm21, ymm2
    add            eax,   16
    vpavgb         ymm17, ymm17, ymm20
    vpsrlw         ymm16, ymm16, 2
    vpxorq         ymm18, ymm18, ymm3
    vpxorq         ymm19, ymm19, ymm1
    vpavgb         ymm18, ymm18, ymm19
    vpavgb         ymm17, ymm17, ymm18
    vpandq         ymm16, ymm16, ymm0
    vpsubusb       ymm18, ymm0,  ymm16
    vpaddusb       ymm19, ymm4,  ymm16
    vpaddusb       ymm20, ymm5,  ymm18
    vpaddusb       ymm4,  ymm4,  ymm18
    vpaddusb       ymm5,  ymm5,  ymm16
    vpminub        ymm16, ymm20, ymm19
    vpminub        ymm18, ymm5,  ymm4
; ml64 doesn't understand 'vpcmpleub', so we use the original mnemonic
; 'vpcmpub' from the docs instead
;    vpcmpleub       k0,    ymm20, ymm19
    vpcmpub        k0,    ymm20, ymm19, 2
    vpmovm2b       ymm19, k0
;    vpcmpleub       k0,    ymm5, ymm4
    vpcmpub        k0,    ymm5,  ymm4, 2
    vpmovm2b       ymm4,  k0
    vpsrlw         ymm5,  ymm17, 2
    vpand          ymm5,  ymm5,  ymm0
    vpsubusb       ymm17, ymm0,  ymm5
    vpunpcklbw     ymm20, ymm16, ymm18
    vpermq         ymm20, ymm20, 216
    vpaddusb       ymm21, ymm20, ymm5
    vpunpckhbw     ymm16, ymm16, ymm18
    vpermq         ymm16, ymm16, 216
    vpaddusb       ymm18, ymm16, ymm17
    vpaddusb       ymm17, ymm20, ymm17
    vpaddusb       ymm5,  ymm16, ymm5
    vpminub        ymm16, ymm18, ymm21
;    vpcmpleub       k0,    ymm18, ymm21
    vpcmpub        k0,    ymm18, ymm21, 2
    vpmovm2b       ymm18, k0
    vmovd          edi,   xmm16
    vpminub        ymm20, ymm5,  ymm17
;    vpcmpleub       k0,    ymm5, ymm17
    cmp            dil,   RENORMALIZE_THRESHOLD 
    vpcmpub        k0,    ymm5,  ymm17, 2
    vpmovm2b       ymm5,  k0
    vpunpcklbw     ymm17, ymm19, ymm4
    vpmovb2m       k0,    ymm17
    kmovd          dword ptr [rsp + rax - 16], k0
    vpunpckhbw     ymm4,  ymm19, ymm4
    vpmovb2m       k0,    ymm4
    kmovd          dword ptr [rsp + rax - 12], k0
    vpunpcklbw     ymm4,  ymm18, ymm5
    vpmovb2m       k0,    ymm4
    kmovd          dword ptr [rsp + rax - 8], k0
    vpunpckhbw     ymm4,  ymm18, ymm5 
    vpmovb2m       k0,    ymm4
    kmovd          dword ptr [rsp + rax - 4], k0
    vpunpcklbw     ymm4,  ymm16, ymm20
    vpermq         ymm4,  ymm4,  216
    vpunpckhbw     ymm5,  ymm16, ymm20
    vpermq         ymm5,  ymm5,  216
    jb             mainloop

    vpsubusb       ymm4,  ymm4,  ymm0
    vpsubusb       ymm5,  ymm5,  ymm0
    jmp            mainloop

    Chainback_mac
    xor eax, eax
    RestoreRegs
    vzeroupper
    ret
decon_avx5 endp

_TEXT$avx5 ends
end