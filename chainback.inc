comment!#######################################################################
#                           This file is part of                              #
#           'viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)'           #
#                                                                             #
#  The macro for 'chainback' used by all assembly-language procs.             #
#                                                                             #
#  Registers @ entry:                                                         #
#  rsp: decisions                                                             #
#  r9 : output-buffer                                                         #
#  r10: framebits                                                             #
#                                                                             #
#  Registers used: rax, rcx, rdx, rdi, rsi, r9, r10                           #
#                                                                             #
#  (c) 2022 Heiko Vogel <hevog@gmx.de>                                        #
#                                                                             #
##############################################################################!

Chainback_mac macro
align 16
chainback:    sub     r10d, 1
    mov     rcx, 0 
    lea     rsi, [rsp + 8*r10 + 48]

; loop starts at chainback + 16 bytes
@@: mov     eax, ecx
    mov     edx, r10d
    shr     eax, 7
    mov     edi, ecx
    shr     rdx, 3
    shr     cl, 2
    mov     eax, dword ptr [rsi + 4*rax]
    add     rsi, -8
    shr     edi, 1
    shr     eax, cl
    shl     eax, 7
    movzx   ecx, al
    or      ecx, edi
    add     r10d, -1
    mov     byte ptr [r9 + rdx], cl
    jb      @b
endm