comment!#######################################################################
#                           This file is part of                              #
#           'viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)'           #
#                                                                             #
#  const.asm                                                                  #
#                                                                             #
#  Constants used by the viterbi-decoder. The 256-bit values are permuted to  #
#  avoid four vpermq-instructions inside of the viterbi-mainloop.             #
#                                                                             #
#  (c) 2022-23 Heiko Vogel <hevog@gmx.de>                                     #
##############################################################################!


include const.inc


_rdata segment align(64) alias(".rdata") readonly
;--- cache-line ---
m256_63_0 label ymmword
m128_63_0 label xmmword
db      0
db     15 dup(63)

m128_63 label xmmword
db     16 dup(63)

m256_XOR_0_3_4_7 label ymmword
db     000H, 000H, 0FFH, 0FFH, 0FFH, 0FFH, 000H, 000H,
       0FFH, 0FFH, 000H, 000H, 000H, 000H, 0FFH, 0FFH,
       000H, 000H, 0FFH, 0FFH, 0FFH, 0FFH, 000H, 000H,
       0FFH, 0FFH, 000H, 000H, 000H, 000H, 0FFH, 0FFH

; --- cache line ---

m128_1st_XOR_0_3_4_7 label xmmword
db     000H, 000H, 0FFH, 0FFH, 0FFH, 0FFH, 000H, 000H,
       000H, 000H, 0FFH, 0FFH, 0FFH, 0FFH, 000H, 000H

m128_2nd_XOR_0_3_4_7 label xmmword
db     0FFH, 0FFH, 000H, 000H, 000H, 000H, 0FFH, 0FFH,
       0FFH, 0FFH, 000H, 000H, 000H, 000H, 0FFH, 0FFH

m128_XOR_1_5 label xmmword
db     000H, 0FFH, 0FFH, 000H, 0FFH, 000H, 000H, 0FFH,
       000H, 0FFH, 0FFH, 000H, 0FFH, 000H, 000H, 0FFH

m128_XOR_2_6 label xmmword
db     000H, 0FFH, 000H, 0FFH, 000H, 0FFH, 000H, 0FFH,
       0FFH, 000H, 0FFH, 000H, 0FFH, 000H, 0FFH, 000H

; --- cache-line ---

m256_XOR_1_5 label ymmword
db     000H, 0FFH, 0FFH, 000H, 0FFH, 000H, 000H, 0FFH,
       000H, 0FFH, 0FFH, 000H, 0FFH, 000H, 000H, 0FFH,
       000H, 0FFH, 0FFH, 000H, 0FFH, 000H, 000H, 0FFH,
       000H, 0FFH, 0FFH, 000H, 0FFH, 000H, 000H, 0FFH

m256_XOR_2_6 label ymmword
db     000H, 0FFH, 000H, 0FFH, 000H, 0FFH, 000H, 0FFH,
       000H, 0FFH, 000H, 0FFH, 000H, 0FFH, 000H, 0FFH,
       0FFH, 000H, 0FFH, 000H, 0FFH, 000H, 0FFH, 000H,
       0FFH, 000H, 0FFH, 000H, 0FFH, 000H, 0FFH, 000H

; --- cache line ---
; factor for SSE4.1 (pmulld)
;m128_16X_0x1 label xmmword
;db     16 dup(1)

_rdata ends
end