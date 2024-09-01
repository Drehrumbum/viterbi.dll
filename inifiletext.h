/*
* This file is part of
*
* viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)
*
* 
* (c) 2023/24 Heiko Vogel <hevog@gmx.de>
* 
*/


const char szIniFileText[] =
"x:x\r\n"
"^ ^\r\n"
"^ ^------- On-screen information ________________ : 0 = off, other chars: on\r\n"
"^--------- Instruction-set override _____________ : 0 to 3,  other chars: auto\r\n\r\n"
"This is the viterbi.dll config-file. Normally you don't need to change\r\n"
"anything here because the DLL selects the highest possible instruction-set\r\n"
"your CPU supports. Do not 'downgrade' your CPU just for fun. Try the little\r\n"
"test-program instead. It will select the fastest instruction-set for you.\r\n\r\n"
"Instruction-set override:\r\n"
"\t0 => SSE2\r\n"
"\t1 => SSSE3\r\n"
"\t2 => AVX\r\n"
"\t3 => AVX2\r\n"
"\t4 => AVX512\r\n\r\n"
"A little rectangle appears in the upper left corner of the screen on startup,\r\n"
"where the selected instruction-set is shown. Set 'On-screen information' to zero\r\n"
"if you don't want to see it anymore.\r\n\r\n"
"Make sure to save changes of this file in 8-bit ASCII format. Then restart the\r\n"
"DAB-receiver in QIRX.";