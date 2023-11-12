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
* (c) 2022 Heiko Vogel <hevog@gmx.de>
* 
*/

extern "C" int GetCPUCaps();
// Bitmasks returned by GetCPUCaps
#define CpuHasSSE2  0b00000000
#define CpuHasSSE3  0b00000001
#define CpuHasSSSE3 0b00000010
#define CpuHasSSE41 0b00000100
#define CpuHasSSE42 0b00001000
#define CpuHasAVX1  0b00010000
#define CpuHasAVX2  0b00100000
#define CpuHasFMA3  0b01000000
// The next bit is only set if the CPU supports AVX512F+BW+VL!
#define CpuHasAVX5  0b10000000
