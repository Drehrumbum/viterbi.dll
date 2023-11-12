comment!#######################################################################
#                           This file is part of                              #
#           'viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)'           #
#                                                                             #
#  const.inc                                                                  #
#                                                                             #
#  include-file for all asm-parts.                                            #
#                                                                             #
#  (c) 2023 Heiko Vogel <hevog@gmx.de>                                        #
##############################################################################!

RENORMALIZE_THRESHOLD equ 150 ; stay below 180
DECISIONS_ARRAY_SIZE  equ (384 * 24 + 6) * 8


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
