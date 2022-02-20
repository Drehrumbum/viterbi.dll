/*
* This file is part of
*
* viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)
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
* (c) 2021-2022 by Heiko Vogel <hevog@gmx.de>
*
* 
* Pre-calculated "BranchTable" for viterbi-decoding with the
* DAB polynominals 109, 79, 83 and 109. 
* 
* The compiler loves this table... 
*/
__declspec(align(64))
static const unsigned char BranchTable[]{
    0, 0, 255, 255, 255, 255, 0, 0,  0, 0, 255, 255, 255, 255, 0, 0,
    255, 255, 0, 0, 0, 0, 255, 255,  255, 255, 0, 0, 0, 0, 255, 255,
    0, 255, 255, 0, 255, 0, 0, 255,  0, 255, 255, 0, 255, 0, 0, 255,
    0, 255, 255, 0, 255, 0, 0, 255,  0, 255, 255, 0, 255, 0, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255,  255, 0, 255, 0, 255, 0, 255, 0,
    0, 255, 0, 255, 0, 255, 0, 255,  255, 0, 255, 0, 255, 0, 255, 0,
    0, 0, 255, 255, 255, 255, 0, 0,  0, 0, 255, 255, 255, 255, 0, 0,
    255, 255, 0, 0, 0, 0, 255, 255,  255, 255, 0, 0, 0, 0, 255, 255
};
