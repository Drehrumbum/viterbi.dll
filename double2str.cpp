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
*
* We don't have a C-Lib and wsprintf() from user32.dll does not support
* floating point numbers. We need this stuff only if VIT_WRITE_LOGFILE
* is defined and formated output should go to the logfile.
*
* double2str converts a double to a zero-terminated string with the
* wanted min-width and precision and returns a pointer to the beginning
* of the string. The given buffer must have a size of at least 32 bytes.
* The function is good enough for the things we want to do with it and
* was inspired from
* https://stackoverflow.com/questions/23191203/convert-float-to-string-without-sprintf
*
* Example:
*   char buffer[32];
*   char pszNum* = double2str(-3.1415926, MAKELONG(12,6), buffer);
* pszNum points to "   -3.141593".
*
* (c) 2023 Heiko Vogel <hevog@gmx.de>
*
*/


#include "viterbi.h"

//extern "C" int _fltused = 0; // fool the linker

char* double2str(double x, int widthPrec, char* buffer) {

    unsigned long long units, decimals, modulo = 1;
    int numDecimals = HIWORD(widthPrec);
    int minWidth    = LOWORD(widthPrec);
    int i, len = 0;
// get the sign and reset it
    BOOLEAN isNegative = _bittestandreset64((LONG64*)&x, 63);

    for (i = 0; i < 31; i++)
        buffer[i] = ' ';

    buffer += 32;
    *--buffer = '\0';

    for (i = 0; i < numDecimals; i++) modulo *= 10;

    decimals = (unsigned long long)(x * modulo + 0.5) % modulo;
    units    = (unsigned long long)x;

    for (i = 1; i <= numDecimals; i++, len++) {
        *--buffer = (decimals % 10) + '0';
        decimals /= 10;
    }

    if (numDecimals) {
        *--buffer = '.';
        len++;
    }

    if (x < 1.0) {
        *--buffer = '0';
        len++;
    }

    while (units > 0) {
        *--buffer = (units % 10) + '0';
        units /= 10;
        len++;
    }

    if (isNegative) {
        *--buffer = '-';
        len++;
    }

    while (len++ < minWidth)
        --buffer;

    return buffer;
}
