/*
* This file is part of
*
* viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)
*
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation; either version 2 of the License, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along with
* this program; if not, write to the Free Software Foundation, Inc., 59 Temple
* Place - Suite 330, Boston, MA 02111-1307, USA
* and it is distributed under the GNU General Public License (GPL).
*
* 
* (c) 2023 Heiko Vogel <hevog@gmx.de>
* 
*/


const char szIniFileText[] =
"x:x\r\n"
"^ ^\r\n"
"^ ^------- On-screen information ______________ : 0 = off, other chars: on\r\n"
"^--------- Instruction-set override ___________ : 0 to 4,  other chars: auto\r\n\r\n"
"This is the viterbi.dll config-file. Normally you don't need to change\r\n"
"anything here because the DLL selects the highest possible instruction-set\r\n"
"your CPU supports. Do not 'downgrade' your CPU just for fun. Try the little\r\n"
"test-program instead. It will select the fastest instruction-set for you.\r\n\r\n"
"Instruction-set override:\r\n"
"\t0 => SSE2\r\n"
"\t1 => SSSE3\r\n"
"\t2 => AVX\r\n"
"\t3 => AVX2\r\n"
"\t4 => AVX512F\r\n\r\n"
"A little rectangle appears in the upper left corner of the screen on startup, where\r\n"
"the selected instruction-set is shown. Set on-screen information to zero if you\r\n"
"don't want to see it anymore.\r\n\r\n"
"Make sure to save changes of this file in 8-bit ASCII format. Then restart the\r\n"
"DAB-receiver in QIRX.";