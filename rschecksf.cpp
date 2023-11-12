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
* Original DLL-version for QIRX (c) 2018 by Clem Schmidt
* (https://qirx.softsyst.com). The sources of this DLL can be found at Clem's
* website within the download-section, more likely inside of a Linux-package.
* 
* Original sceen-play (c) 2001++ by Phil Karn, KA9Q. Mr. Karn's software for
* Viterbi- and Reed-Solomon decoding can be found at
* http://www.ka9q.net/code/fec/ or at Github.
*
*
* (c) 2021-23 Heiko Vogel <hevog@gmx.de>
*
*/


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include "viterbi.h"

#define PAD 135
extern RS_LookUp* rsLUT;



// Replacement for x % 255 calculations.
// The remainder is valid for 0 <= x < 66299
__forceinline unsigned int Mod255(unsigned int x) {
    return (x * 0x1010102) >> 24;
}


int DECODE_RS(unsigned int*, unsigned char*, unsigned char*);


 /*
* Reed-solomon forward error correction
* Input: RSDims * 120 bytes
* Output: RSDims * 110 bytes
*/

#ifndef VIT_WRITE_LOGFILE
__attribute__((target("sse2")))
int RScheckSuperframe(unsigned char* p, int startIx,
    unsigned int RSDims, unsigned char* outVector)
{
    UNREFERENCED_PARAMETER(startIx); // not needed, it's always zero
    long long j, k, l;
    int result, errors = 0;
    align(64) unsigned int rsBlock[128];

    for (j = 0; j < RSDims; j++) {
        for (k = 0, l = 0; k < 120; k++, l += RSDims)
           rsBlock[k] = p[j + l];

        result = DECODE_RS(rsBlock, rsLUT->RS_ato_mod, rsLUT->RS_iof);

        if (result != -1) {
            errors += result;
            for (k = 0, l = 0; k < 110; k++, l += RSDims)
                outVector[j + l] = (unsigned char)rsBlock[k];
        }
        else {
            errors = -1;
            goto out;
        }
    }

out:
    return errors;
}
#else
extern char* double2str(double x, int WidthPrec, char* buffer);
extern HANDLE hVitLogFile;
extern unsigned int counter;
extern char *rsBitsLow, *rsBitsHig, *rsOutLow, *rsOutHig;
int rsEntry = 0;
ULARGE_INTEGER rsLastEntryTime;

__attribute__((target("sse2")))
int RScheckSuperframe(unsigned char* p, int startIx,
    unsigned int RSDims, unsigned char* outVector)
{
    UNREFERENCED_PARAMETER(startIx); // not needed, it's always zero
    DWORD numBytesWritten;
    ULARGE_INTEGER entryTime, leaveTime;
    FILETIME fte, ftl;
    SYSTEMTIME sysTime;
    unsigned long long stackHighLimit;
    char szBuffer[320], szDuration[32], szLastCallTime[32];
    double dDuration, dLastCallTime = 0;

    long long j, k, l;
    int result, errors = 0;
    align(64) unsigned int rsBlock[128];

    rsEntry++;
    GetSystemTimePreciseAsFileTime(&fte);

    for (j = 0; j < RSDims; j++) {
        for (k = 0, l = 0; k < 120; k++, l += RSDims)
            rsBlock[k] = p[j + l];

        result = DECODE_RS(rsBlock, rsLUT->RS_ato_mod, rsLUT->RS_iof);

        if (result != -1) {
            errors += result;
            for (k = 0, l = 0; k < 110; k++, l += RSDims)
                outVector[j + l] = (unsigned char)rsBlock[k];
        }
        else {
            errors = -1;
            goto out;
        }
    }

out:
    GetSystemTimePreciseAsFileTime(&ftl);
// Reading directly from the TIB to avoid GetCurrentThreadStackLimits()
// which is not supported in Windows 7.
// See https://en.wikipedia.org/wiki/Win32_Thread_Information_Block
    stackHighLimit = __readgsqword(0x8);
    entryTime.HighPart = fte.dwHighDateTime;
    entryTime.LowPart = fte.dwLowDateTime;
    leaveTime.HighPart = ftl.dwHighDateTime;
    leaveTime.LowPart = ftl.dwLowDateTime;
    dDuration = (leaveTime.QuadPart - entryTime.QuadPart) * 0.1;
    if (rsLastEntryTime.QuadPart)
        dLastCallTime = (double)(entryTime.QuadPart
            - rsLastEntryTime.QuadPart) / 10000.0; // ms

    rsLastEntryTime = entryTime;
    FileTimeToSystemTime(&fte, &sysTime);
// calculate microsecs of current second
    entryTime.QuadPart = (entryTime.QuadPart % 10000000uLL) / 10uLL;
    wsprintf(szBuffer, "%6u  %02u:%02u:%02u.%06u  dT: %s ms  "
        "TID: %5u  StU: %5i  rscs: %s us  "
        "ReE: %i  RSDim: %4i  Inp: 0x%p  Out: 0x%p  RetVal: %2i\n",
        counter++,
        sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
        entryTime.LowPart,
        double2str(dLastCallTime, MAKELONG(8, 3), szLastCallTime),
        GetCurrentThreadId(),
// Stack-usage of current thread. We use the shadow-space address of the
// register rcx (it's 'p' here) right above of the return address on the stack.
        (int)(stackHighLimit - (unsigned long long) &p),
        double2str(dDuration, MAKELONG(6, 1), szDuration),
// If another thread enters the function until we are here the value will be 1.
// Never seen, but this may change in the future.
        --rsEntry,
        RSDims,
        p,
        outVector,
        errors);
    WriteFile(hVitLogFile, szBuffer, lstrlen(szBuffer), &numBytesWritten, NULL);

    if ((char*)p < rsBitsLow) rsBitsLow = (char*)p;
    if ((char*)p > rsBitsHig) rsBitsHig = (char*)p;
    if ((char*)outVector < rsOutLow) rsOutLow = (char*)outVector;
    if ((char*)outVector > rsOutHig) rsOutHig = (char*)outVector;

    return errors;
}
#endif // !VIT_WRITE_LOGFILE



// The arrays 't', 'reg', 'loc' and 'omega' from the original sources
// have been removed. The remaining arrays are reused instead.
__attribute__((target("sse2")))
__forceinline int DECODE_RS(unsigned int* data, unsigned char* ato_mod,
    unsigned char* index_of) {
    align(16) unsigned char root[16];
    align(16) unsigned char lambda[16];
    align(16) unsigned char s[16];
    align(16) unsigned char b[16];
    unsigned int q, tmp, num1, num2, den, discr_r;
    int el, deg_lambda, deg_omega, syn_error, count, r, i, j;

    *(__m128i*)s = _mm_set1_epi8(data[0]);

    for (j = 1; j < c_nn - PAD; j++)
    {
        for (i = 0; i < c_nroots; i++)
        {
            if (s[i] == 0)
                s[i] = data[j];
            else
                s[i] = data[j] ^ ato_mod[index_of[s[i]] + i];
        }
    }

    // Convert syndromes to index form, checking for nonzero condition
    syn_error = 0;

    for (i = 0; i < c_nroots; i++)
        syn_error |= s[i];

    // if syndrome is zero, data[] is a codeword and there are no
    // errors to correct. So return data[] unmodified
    if (!syn_error)
        return 0;

    for (i = 0; i < c_nroots; i++)
        s[i] = index_of[s[i]];

    *(__m128i*)b = _mm_cmpeq_epi16(*(__m128i*)s, *(__m128i*)s); // all bits set
    *(__m128i*)lambda = _mm_setzero_si128();
    b[0] = 0;
    lambda[0] = 1;

    // Begin Berlekamp-Massey algorithm to determine error+erasure
    // locator polynomial
    r = el = 0;

    while (++r <= c_nroots) // r is the step number
    {
        discr_r = 0; // Compute discrepancy at the r-th step in poly-form
        for (i = 0; i < r; i++)
            if ((lambda[i] != 0) && (s[r - i - 1] != c_nn))
                discr_r ^= ato_mod[index_of[lambda[i]] + s[r - i - 1]];

        discr_r = index_of[discr_r]; // Index form
        if (discr_r == c_nn)
        {
            *(__m128i*)b = _mm_slli_si128(*(__m128i*)b, 1);
            b[0] = c_nn;
        }
        else
        {
            // 7 lines below: T(x) <-- lambda(x) - discr_r*x*b(x)
            //t[0] = lambda[0];
            root[0] = lambda[0];
            for (i = 0; i < c_nroots; i++)
            {
                root[i + 1] = lambda[i + 1];

                if (b[i] != c_nn)
                    root[i + 1] ^= ato_mod[discr_r + b[i]];
            }

            if (2 * el <= r - 1)
            {
                el = r - el;
                // 2 lines below: B(x) <-- inv(discr_r)
                // lambda(x)
                for (i = 0; i <= c_nroots; i++)
                    b[i] = (lambda[i] == 0) ? c_nn : Mod255(index_of[lambda[i]] - discr_r + c_nn);
            }
            else
            {
                // 2 lines below: B(x) <-- x*B(x)
                *(__m128i*)b = _mm_slli_si128(*(__m128i*)b, 1);
                b[0] = c_nn;
            }
            _mm_store_si128((__m128i*)lambda, _mm_load_si128((__m128i*)root));
        }
    }

    // Convert lambda to index form and compute deg(lambda(x))
    deg_lambda = 0;
    for (i = 0; i < c_nroots + 1; i++)
    {
        lambda[i] = index_of[lambda[i]];
        if (lambda[i] != c_nn)
            deg_lambda = i;
    }

    // Find roots of the error+erasure locator polynomial by Chien search
    _mm_store_si128((__m128i*)b, _mm_load_si128((__m128i*)lambda));
    count = 0; // Number of roots of lambda(x)

    for (i = 1; i <= c_nn; i++)
    {
        q = 1; // lambda[0] is always 0
        for (j = deg_lambda; j > 0; j--)
        {
            if (b[j] != c_nn)
            {
                b[j] = Mod255(b[j] + j);
                q ^= ato_mod[b[j]];
            }
        }
        if (q != 0) continue; // Not a root

        // store root (index-form) and error location number
        root[count] = i;

        // If we've already found max possible roots,
        // abort the search to save time

        if (++count == deg_lambda)
            break;
    }

    // deg(lambda) unequal to number of roots => uncorrectable
    // error detected

    if (deg_lambda != count)
        return -1;

    // Compute err+eras evaluator poly omega(x) = s(x)*lambda(x) (modulo
    // x**c_nroots). in index form. Also find deg(omega).

    deg_omega = deg_lambda - 1;

    for (i = 0; i <= deg_omega; i++)
    {
        tmp = 0;
        for (j = i; j >= 0; j--)
            if ((s[i - j] != c_nn) && (lambda[j] != c_nn))
                tmp ^= ato_mod[s[i - j] + lambda[j]];

        b[i] = index_of[tmp];
    }

    // Compute error values in poly-form. num1 = omega(inv(X(l))), num2 =
    // inv(X(l))**(FCR-1) and den = lambda_pr(inv(X(l))) all in poly-form

    for (j = count - 1; j >= 0; j--)
    {
        if (root[j] < PAD + 1) continue;

        num1 = 0;

        for (i = deg_omega; i >= 0; i--)
            if (b[i] != c_nn)
                num1 ^= ato_mod[Mod255(b[i] + i * root[j])];

        if (!num1) continue;

        num2 = ato_mod[c_nn - root[j]];
        den = 0;
      // lambda[i+1] for i even is the formal derivative lambda_pr of lambda[i]
        for (i = min(deg_lambda, c_nroots - 1) & ~1; i >= 0; i -= 2)
            if (lambda[i + 1] != c_nn)
                den ^= ato_mod[Mod255(lambda[i + 1] + i * root[j])];

        // Apply error to data
        tmp = (index_of[num1] + index_of[num2]) + (c_nn - index_of[den]);
static_assert(ATO_MOD_SIZE == 512 || ATO_MOD_SIZE == 768,
    "ATO_MOD_SIZE must be 512 or 768!");
#if (ATO_MOD_SIZE == 768)
        data[root[j] - 1 - PAD] ^= ato_mod[tmp];
#elif (ATO_MOD_SIZE == 512)
        data[root[j] - 1 - PAD] ^= ato_mod[Mod255(tmp)];
#endif
    }

    return count;
}
