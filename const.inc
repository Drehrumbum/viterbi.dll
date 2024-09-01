comment!#######################################################################
#                           This file is part of                              #
#           'viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)'           #
#                                                                             #
#  const.inc                                                                  #
#                                                                             #
#  include-file for all asm-parts.                                            #
#                                                                             #
#  (c) 2023-24 Heiko Vogel <hevog@gmx.de>                                     #
##############################################################################!

RENORMALIZE_THRESHOLD equ 150 ; stay below 180
DECISIONS_ARRAY_SIZE  equ 12030h  ; (384 * 24 + 6) * 8

;Some multi-byte NOP filler
NOP_2 textequ	<db 66h, 90h>
NOP_3 textequ	<db 0Fh, 1Fh, 00h>
NOP_4 textequ	<db 0Fh, 1Fh, 40h, 00h>
NOP_5 textequ	<db 0Fh, 1Fh, 44h, 00h, 00h>
NOP_6 textequ	<db 66h, 0Fh, 1Fh, 44h, 00h, 00h>
NOP_7 textequ	<db 0Fh, 1Fh, 80h, 00h, 00h, 00h, 00h>
NOP_8 textequ	<db 0Fh, 1Fh, 84h, 00h, 00h, 00h, 00h, 00h>
NOP_9 textequ	<db 66h, 0Fh, 1Fh, 84h, 00h, 00h, 00h, 00h, 00h>
NOP10 textequ	<db 66h, 2Eh, 0Fh, 1Fh, 84h, 00h, 00h, 00h, 00h, 00h>

externdef m128_63_0            : xmmword
externdef m256_63_0            : ymmword
externdef m128_63              : xmmword
externdef m256_XOR_0_3_4_7     : ymmword
externdef m256_XOR_1_5         : ymmword
externdef m128_XOR_1_5         : xmmword
externdef m256_XOR_2_6         : ymmword
externdef m128_1st_XOR_0_3_4_7 : xmmword
externdef m128_2nd_XOR_0_3_4_7 : xmmword
externdef m128_XOR_2_6         : xmmword
externdef m128_16X_0x1         : xmmword
