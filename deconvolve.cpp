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
* Place - Suite 330, Boston, MA 02111-1307, USA and it is distributed under the
* GNU General Public License (GPL).
*
*
* Original DLL-version for QIRX (c) 2018 by Clem Schmidt
* (https://qirx.softsyst.com). The sources of this DLL can be found at Clem's
* website within the download-section, more likely inside of a Linux-package.
* 
* Original sceen-play (c) 2001++ by Phil Karn, KA9Q. Mr. Karn's software for
* Viterbi- and Reed-Solomon decoding can be found at
* http://www.ka9q.net/code/fec/ or at Github. It may be used under the terms
* of the GNU Lesser General Public License (LGPL).
*
*
*
* Here you will find all the macros necessary to create the DLL with the
* C-intrinsics and the skeleton for the multi-versioning of the function
* 'deconvolve'. Switch to 'Rel_cpp' in the 'Solution Configuration' drop-down
* list to enable the code below. It may be necessary to rearrange the
* intrinsics or introduce dummy variables so that the compiler can optimizing
* better. With the 128-bit decoders, the compiler is under high register-
* pressure because all constants and metrics should be kept in the registers,
* but only 16 SIMD registers are available. For example, pay attention to
* whether the compiler generates code to load one or two constants from memory
* within the main loop. This is most likely not what you want. 
* It's a playground - have fun.
* 
* (c) 2021-24 Heiko Vogel <hevog@gmx.de>
* 
*/


// The project is compiled for SSE2, but we need some more inrinsincs
// for multiversioning.
#define __SSSE3__
#define __SSE4_1__
#define __AVX__
#define __AVX2__
#include "viterbi.h"
#include <intrin.h>


// The pointer to the selected decoder version. See setupdll.cpp.
extern DECON *deconJumpTarget;

// The external LUT for SSE2
extern "C" int* symbols32LUT;

// Constants for the viterbi-decoder. The constants are well arranged in
// memory so that every function needs to read from two cache-lines only.
extern "C" const __m128i
m128_63_0,              // init old_metrics first field
m128_63,                // init old_metrics other fields and renormalizing
m128_1st_XOR_0_3_4_7,   // 1st XOR value for symbols 0, 3, 4 and 7
m128_2nd_XOR_0_3_4_7,   // 2nd XOR value for symbols 0, 3, 4 and 7
m128_XOR_1_5,           // XOR value for symbols 1 and 5
m128_XOR_2_6,           // XOR value for symbols 2 and 6
m128_16X_0x1;           // Factor for SSE4.1 (needs a third cache-line)
extern "C" const __m256i
m256_63_0,              // init old_metrics first field, points to m128_63_0
m256_XOR_0_3_4_7,       // XOR value for symbols 0, 3, 4 and 7
m256_XOR_1_5,           // XOR value for symbols 1 and 5
m256_XOR_2_6;           // XOR value for symbols 2 and 6


// Local variables and setup for all 128-bit versions
#define Locals \
__m128i  OldMet[4], NewMet[4], survivor0, survivor1, \
         decision0, decision1, metric, m_metric, m0, m1, m2, m3, \
         sym0, sym1, sym2, sym3, r1, r2;\
__m128i* old_metrics = (__m128i*)OldMet; \
__m128i* new_metrics = (__m128i*)NewMet; \
\
int nbits = (framebits + 6) / 2; \
decision_t decis[384 * 24 + 6];\
decision_t *decisions = decis; \
short int* d = (short int*)decis; \
int dec0, dec1, dec2, dec3;\
OldMet[0] = m128_63_0;\
OldMet[1] = m128_63;\
OldMet[2] = m128_63;\
OldMet[3] = m128_63;\


// Additional locals if the macro 'Butterfly2' is used.
#define Locals2 \
__m128i  survivor01, survivor11,\
         decision01, decision11, metric1, m_metric1,\
         m01, m11, m21, m31;

// Additional locals if you try the 'Load4SymsMultiply' macro below.
#define Locals_Multiply \
long long *pqData = (long long*) piData;\
long long q0, q1;


// Local variables and setup for the 256-bit versions
#define Locals256 \
__m256i old_metrics[2], new_metrics[2],\
        m256_sym0, m256_sym1, m256_sym2, m256_sym3,\
        m256_sym4, m256_sym5, m256_sym6, m256_sym7,\
        m256_metric, m256_m_metric, m256_survivor0, m256_survivor1,\
        m256_decision0, m256_decision1, m256_m0, m256_m1, m256_m2, m256_m3,\
        m256_metric1, m256_m_metric1, m256_survivor01, m256_survivor11,\
        m256_decision01, m256_decision11, m256_m01, m256_m11, m256_m21,\
        m256_m31, r256_63;\
\
int nbits = (framebits + 6) / 2;\
decision_t decis[384 * 24 + 6];\
decision_t* decisions = decis;\
int* ddd = (int*)decisions;\
old_metrics[0] = m256_63_0;\
r256_63 = _mm256_broadcastb_epi8(m128_63);\
old_metrics[1] = r256_63;


// Below are some macros for loading and broadcasting the 'symbols'.
// QIRX uses an array of 32-bit integers for the symbols. The symbols are
// still in the range from 0 to 255, of course. This means the upper three
// bytes of a symobl are always zero and we can try out other things for 
// broadcasting beside of _mm_set1_epi8().
// 
// UPDATE 2024-09: The last sentences above are not entirely correct. It
// turns out that under very rare conditions the “symbols” are greater
// than 255. This caused crashes inside of the DLL when the SSE2-decoder
// was used. The other decoders were not affected.
//
// Loads and broadcasts four symbols by using the compiler-defaults.
// Generates 'standard-sequences' for SSE2 or faster 'pshufb' instructions
// if SSSE3++ or AVX is used.
#define Load4Syms {\
    sym0 = _mm_set1_epi8(*piData++);\
    sym1 = _mm_set1_epi8(*piData++);\
    sym2 = _mm_set1_epi8(*piData++);\
    sym3 = _mm_set1_epi8(*piData++);\
}

// Loads and broadcasts four symbols by using the LUT generated in DllMain
// at startup. For SSE2 only.
// UPDATE 2024-09: Clamping the symbols to 0xFF. Don't worry, there will be
// no additional AND instructions inside of the code.
#define Load4SymsLUT {\
    sym0 = _mm_shuffle_epi32(_mm_cvtsi32_si128(symbols32LUT[*piData++ & 0xFF]), 0);\
    sym1 = _mm_shuffle_epi32(_mm_cvtsi32_si128(symbols32LUT[*piData++ & 0xFF]), 0);\
    sym2 = _mm_shuffle_epi32(_mm_cvtsi32_si128(symbols32LUT[*piData++ & 0xFF]), 0);\
    sym3 = _mm_shuffle_epi32(_mm_cvtsi32_si128(symbols32LUT[*piData++ & 0xFF]), 0);\
}

// Loads and broadcasts four symbols by using two GPR and two 64-bit integer
// multiplications. Only for SSE2. It will be fast on older CPU's from Intel,
// so you may use it to avoid the LUT-method. However, use SSSE3 if supported.
#define Load4SymsMultiply { \
    q0 = *pqData++ * 0x1010101; \
    q1 = *pqData++ * 0x1010101; \
    sym0 = _mm_cvtsi64_si128(q0); \
    sym1 = _mm_shuffle_epi32(sym0, 0x55); \
    sym0 = _mm_shuffle_epi32(sym0, 0); \
    sym2 = _mm_cvtsi64_si128(q1); \
    sym3 = _mm_shuffle_epi32(sym2, 0x55); \
    sym2 = _mm_shuffle_epi32(sym2, 0); \
}

// Another broadcast-macro for SSE2. Older CPU's do not like unaligned 128-bit
// reads from memory. Macro just FYI.   
#define Load4SymsRead128 { \
    sym3 = _mm_loadu_si128((__m128i_u*)piData); \
    piData += 4; \
    sym1 = sym3; \
    sym1 = _mm_unpacklo_epi8(sym1, sym3); \
    sym3 = _mm_unpackhi_epi8(sym3, sym3); \
    sym0 = _mm_shufflelo_epi16(sym1, 0); \
    sym2 = _mm_shufflelo_epi16(sym3, 0); \
    sym1 = _mm_shufflehi_epi16(sym1, 0); \
    sym3 = _mm_shufflehi_epi16(sym3, 0); \
    sym0 = _mm_shuffle_epi32(sym0, 0); \
    sym2 = _mm_shuffle_epi32(sym2, 0); \
    sym1 = _mm_shuffle_epi32(sym1, 170); \
    sym3 = _mm_shuffle_epi32(sym3, 170); \
}

// Loads and broadcasts four symbols by using the 'pmulld' instruction. For
// SSE4.1 only. The constant 'm128_16X_0x1' can easily created on the fly
// just before the main-loop, but the compiler doesn't like this and uses this
// additional constant instead. The SSE4.1 version not used in final release,
// because the SSSE3 version runs faster on all tested CPU's.
// If you wanna try SSE4.1 uncomment the "m128_16X_0x1" label in const.asm
#define Load4SymsSSE41 {\
    sym3 = _mm_mullo_epi32(m128_16X_0x1, *(__m128i_u*)piData);\
    sym0 = _mm_shuffle_epi32(sym3, 0);\
    sym1 = _mm_shuffle_epi32(sym3, 0x55);\
    sym2 = _mm_shuffle_epi32(sym3, 0xaa);\
    sym3 = _mm_shuffle_epi32(sym3, 0xff);\
    piData += 4;\
}

// Load8Syms256 used for AVX2 and AVX512 generates native 'vpbroadcastb'
// instructions. We (try to) load eight symbols at the top of the main-loop.
// The compiler may decide to schedule other upcoming instructions from the
// 'butterfly', such as 'vpxor' or 'vpavgb', in between, so some registers
// can be reused within this sequence.  
#define Load8Syms256 { \
    m256_sym0 = _mm256_set1_epi8(*piData++);\
    m256_sym1 = _mm256_set1_epi8(*piData++);\
    m256_sym2 = _mm256_set1_epi8(*piData++);\
    m256_sym3 = _mm256_set1_epi8(*piData++);\
    m256_sym4 = _mm256_set1_epi8(*piData++);\
    m256_sym5 = _mm256_set1_epi8(*piData++);\
    m256_sym6 = _mm256_set1_epi8(*piData++);\
    m256_sym7 = _mm256_set1_epi8(*piData++);\
}


// The standard butterfly for 128-bit.
#define ButterFly(old_metrics, new_metrics) {\
    m0 = _mm_xor_si128(sym0, m128_1st_XOR_0_3_4_7);\
    m1 = r1 = _mm_xor_si128(sym1, m128_XOR_1_5);\
    m2 = r2 = _mm_xor_si128(sym2, m128_XOR_2_6);\
    m3 = _mm_xor_si128(sym3, m128_1st_XOR_0_3_4_7);\
    m0 = _mm_avg_epu8(m0, m1);\
    m1 = _mm_avg_epu8(m2, m3);\
    metric = _mm_avg_epu8(m0, m1);\
    metric = _mm_srli_epi16(metric, 2);\
    metric = _mm_and_si128(metric, m128_63);\
    m_metric = _mm_subs_epu8(m128_63, metric);\
    m0 = _mm_adds_epu8(old_metrics[0], metric);\
    m1 = _mm_adds_epu8(old_metrics[2], m_metric);\
    m2 = _mm_adds_epu8(old_metrics[0], m_metric);\
    m3 = _mm_adds_epu8(old_metrics[2], metric);\
    survivor0 = _mm_min_epu8(m1, m0);\
    decision0 = _mm_cmpeq_epi8(survivor0, m1);\
    survivor1 = _mm_min_epu8(m3, m2);\
    decision1 = _mm_cmpeq_epi8(survivor1, m3);\
    dec0 = _mm_movemask_epi8(_mm_unpacklo_epi8(decision0, decision1));\
    dec1 = _mm_movemask_epi8(_mm_unpackhi_epi8(decision0, decision1));\
    new_metrics[0] = _mm_unpacklo_epi8(survivor0, survivor1);\
    new_metrics[1] = _mm_unpackhi_epi8(survivor0, survivor1);\
\
    m0 = _mm_xor_si128(sym0, m128_2nd_XOR_0_3_4_7); \
    m3 = _mm_xor_si128(sym3, m128_2nd_XOR_0_3_4_7); \
    m0 = _mm_avg_epu8(m0, r1);\
    m1 = _mm_avg_epu8(r2, m3);\
    metric = _mm_avg_epu8(m0, m1);\
    metric = _mm_srli_epi16(metric, 2);\
    metric = _mm_and_si128(metric, m128_63);\
    m_metric = _mm_subs_epu8(m128_63, metric);\
    m0 = _mm_adds_epu8(old_metrics[1], metric);\
    m1 = _mm_adds_epu8(old_metrics[3], m_metric);\
    m2 = _mm_adds_epu8(old_metrics[1], m_metric);\
    m3 = _mm_adds_epu8(old_metrics[3], metric);\
    survivor0 = _mm_min_epu8(m1, m0);\
    decision0 = _mm_cmpeq_epi8(survivor0, m1);\
    survivor1 = _mm_min_epu8(m3, m2);\
    decision1 = _mm_cmpeq_epi8(survivor1, m3);\
    dec2 = _mm_movemask_epi8(_mm_unpacklo_epi8(decision0, decision1));\
    dec3 = _mm_movemask_epi8(_mm_unpackhi_epi8(decision0, decision1));\
    *(int*)d++ = dec0;\
    *(int*)d++ = dec1;\
    *(int*)d++ = dec2;\
    *(int*)d++ = dec3;\
    new_metrics[2] = _mm_unpacklo_epi8(survivor0, survivor1);\
    new_metrics[3] = _mm_unpackhi_epi8(survivor0, survivor1);\
}

// Another 128-bit butterfly with some more variables.
#define ButterFly2(old_metrics, new_metrics) { \
    m0 = _mm_xor_si128(sym0, m128_1st_XOR_0_3_4_7);\
    m1 = r1 = _mm_xor_si128(sym1, m128_XOR_1_5);\
    m2 = r2 = _mm_xor_si128(sym2, m128_XOR_2_6);\
    m3 = _mm_xor_si128(sym3, m128_1st_XOR_0_3_4_7);\
    m0 = _mm_avg_epu8(m0, m1);\
    m01 = _mm_xor_si128(sym0, m128_2nd_XOR_0_3_4_7);\
    m1 = _mm_avg_epu8(m2, m3);\
    m31 = _mm_xor_si128(sym3, m128_2nd_XOR_0_3_4_7);\
    metric = _mm_avg_epu8(m0, m1);\
    metric = _mm_srli_epi16(metric, 2);\
    m01 = _mm_avg_epu8(m01, r1);\
    metric = _mm_and_si128(metric, m128_63);\
    m11 = _mm_avg_epu8(r2, m31);\
    m_metric = _mm_subs_epu8(m128_63, metric);\
    metric1 = _mm_avg_epu8(m01, m11);\
    m0 = _mm_adds_epu8(old_metrics[0], metric);\
    m1 = _mm_adds_epu8(old_metrics[2], m_metric);\
    m2 = _mm_adds_epu8(old_metrics[0], m_metric);\
    m3 = _mm_adds_epu8(old_metrics[2], metric);\
    survivor0 = _mm_min_epu8(m1, m0);\
    survivor1 = _mm_min_epu8(m3, m2);\
    decision0 = _mm_cmpeq_epi8(survivor0, m1);\
    decision1 = _mm_cmpeq_epi8(survivor1, m3);\
    metric1 = _mm_srli_epi16(metric1, 2);\
    metric1 = _mm_and_si128(metric1, m128_63);\
    m_metric1 = _mm_subs_epu8(m128_63, metric1);\
    dec0 = _mm_movemask_epi8(_mm_unpacklo_epi8(decision0, decision1));\
    dec1 = _mm_movemask_epi8(_mm_unpackhi_epi8(decision0, decision1));\
    new_metrics[0] = _mm_unpacklo_epi8(survivor0, survivor1);\
    new_metrics[1] = _mm_unpackhi_epi8(survivor0, survivor1);\
\
    m01 = _mm_adds_epu8(old_metrics[1], metric1);\
    m11 = _mm_adds_epu8(old_metrics[3], m_metric1);\
    m21 = _mm_adds_epu8(old_metrics[1], m_metric1);\
    m31 = _mm_adds_epu8(old_metrics[3], metric1);\
    survivor01 = _mm_min_epu8(m11, m01);\
    survivor11 = _mm_min_epu8(m31, m21);\
    decision01 = _mm_cmpeq_epi8(survivor01, m11);\
    decision11 = _mm_cmpeq_epi8(survivor11, m31);\
    dec2 = _mm_movemask_epi8(_mm_unpacklo_epi8(decision01, decision11));\
    dec3 = _mm_movemask_epi8(_mm_unpackhi_epi8(decision01, decision11));\
    *(int*)d++ = dec0;\
    *(int*)d++ = dec1;\
    *(int*)d++ = dec2;\
    *(int*)d++ = dec3;\
    new_metrics[2] = _mm_unpacklo_epi8(survivor01, survivor11);\
    new_metrics[3] = _mm_unpackhi_epi8(survivor01, survivor11);\
}

// The 256-bit butterfly.
#define Butterfly256(old_metrics, new_metrics) {\
    m256_sym0 = _mm256_xor_si256(m256_sym0, m256_XOR_0_3_4_7);\
    m256_sym1 = _mm256_xor_si256(m256_sym1, m256_XOR_1_5);\
    m256_sym2 = _mm256_xor_si256(m256_sym2, m256_XOR_2_6);\
    m256_m0 = _mm256_avg_epu8(m256_sym0, m256_sym1);\
    m256_sym3 = _mm256_xor_si256(m256_sym3, m256_XOR_0_3_4_7);\
    m256_m1 = _mm256_avg_epu8(m256_sym2, m256_sym3);\
    m256_sym4 = _mm256_xor_si256(m256_sym4, m256_XOR_0_3_4_7);\
    m256_sym5 = _mm256_xor_si256(m256_sym5, m256_XOR_1_5);\
    m256_metric = _mm256_avg_epu8(m256_m0, m256_m1);\
    m256_sym6 = _mm256_xor_si256(m256_sym6, m256_XOR_2_6);\
    m256_sym7 = _mm256_xor_si256(m256_sym7, m256_XOR_0_3_4_7);\
    m256_m01 = _mm256_avg_epu8(m256_sym4, m256_sym5);\
    m256_metric = _mm256_srli_epi16(m256_metric, 2);\
    m256_m11 = _mm256_avg_epu8(m256_sym6, m256_sym7);\
    m256_metric = _mm256_and_si256(m256_metric, r256_63);\
    m256_m_metric = _mm256_subs_epu8(r256_63, m256_metric);\
    m256_metric1 = _mm256_avg_epu8(m256_m01, m256_m11);\
    m256_m0 = _mm256_adds_epu8(old_metrics[0], m256_metric);\
    m256_m1 = _mm256_adds_epu8(old_metrics[1], m256_m_metric);\
    m256_m2 = _mm256_adds_epu8(old_metrics[0], m256_m_metric);\
    m256_m3 = _mm256_adds_epu8(old_metrics[1], m256_metric);\
    m256_survivor0 = _mm256_min_epu8(m256_m1, m256_m0);\
    m256_survivor1 = _mm256_min_epu8(m256_m3, m256_m2);\
    m256_decision0 = _mm256_cmpeq_epi8(m256_survivor0, m256_m1);\
    m256_decision1 = _mm256_cmpeq_epi8(m256_survivor1, m256_m3);\
    new_metrics[0] = _mm256_unpacklo_epi8(m256_survivor0, m256_survivor1);\
    new_metrics[1] = _mm256_unpackhi_epi8(m256_survivor0, m256_survivor1);\
    new_metrics[0] = _mm256_permute4x64_epi64(new_metrics[0], 0b11'01'10'00);\
    new_metrics[1] = _mm256_permute4x64_epi64(new_metrics[1], 0b11'01'10'00);\
    m256_metric1 = _mm256_srli_epi16(m256_metric1, 2);\
    m256_metric1 = _mm256_and_si256(m256_metric1, r256_63);\
    m256_m_metric1 = _mm256_subs_epu8(r256_63, m256_metric1);\
    m256_m01 = _mm256_adds_epu8(new_metrics[0], m256_metric1);\
    m256_m11 = _mm256_adds_epu8(new_metrics[1], m256_m_metric1);\
    m256_m21 = _mm256_adds_epu8(new_metrics[0], m256_m_metric1);\
    m256_m31 = _mm256_adds_epu8(new_metrics[1], m256_metric1);\
    m256_survivor01 = _mm256_min_epu8(m256_m11, m256_m01);\
    m256_decision01 = _mm256_cmpeq_epi8(m256_survivor01, m256_m11);\
    m256_survivor11 = _mm256_min_epu8(m256_m31, m256_m21);\
    m256_decision11 = _mm256_cmpeq_epi8(m256_survivor11, m256_m31);\
    *ddd++ = _mm256_movemask_epi8(\
        _mm256_unpacklo_epi8(m256_decision0, m256_decision1));\
    *ddd++ = _mm256_movemask_epi8(\
        _mm256_unpackhi_epi8(m256_decision0, m256_decision1));\
    *ddd++ = _mm256_movemask_epi8(\
        _mm256_unpacklo_epi8(m256_decision01, m256_decision11));\
    *ddd++ = _mm256_movemask_epi8(\
        _mm256_unpackhi_epi8(m256_decision01, m256_decision11));\
    old_metrics[0] = _mm256_unpacklo_epi8(m256_survivor01, m256_survivor11);\
    old_metrics[1] = _mm256_unpackhi_epi8(m256_survivor01, m256_survivor11);\
    old_metrics[0] = _mm256_permute4x64_epi64(old_metrics[0], 0b11'01'10'00);\
    old_metrics[1] = _mm256_permute4x64_epi64(old_metrics[1], 0b11'01'10'00);\
}



// Macros for re-normalizing.
// The original macro searches for the lowest byte > 0 inside of the given 
// metrics. This takes time and is not necessary here. I use a simplified macro
// that always subtracts the constant value of 63 from 'old_metrics' if
// RENORMALIZE_THRESHOLD triggers at the end of the main-loop. This is much
// faster and has no drawbacks. We'll see the same FER/BER as with the original
// macro, if RENORMALIZE_THRESHOLD stays below 180.
#define Renormalize128(metrics) {\
    if (((unsigned char*) metrics)[0] > RENORMALIZE_THRESHOLD) {\
        metrics[0] = _mm_subs_epu8(metrics[0], m128_63);\
        metrics[1] = _mm_subs_epu8(metrics[1], m128_63);\
        metrics[2] = _mm_subs_epu8(metrics[2], m128_63);\
        metrics[3] = _mm_subs_epu8(metrics[3], m128_63);\
    }\
}

#define Renormalize256(metrics) {\
    if (((unsigned char*) metrics)[0] > RENORMALIZE_THRESHOLD) {\
        metrics[0] = _mm256_subs_epu8(metrics[0], r256_63);\
        metrics[1] = _mm256_subs_epu8(metrics[1], r256_63);\
    }\
}


// The macro for chain-back 
#define ChainBack {\
    decisions += 6;\
    unsigned int k, kShl7, EndAddShift,\
    Mod32EndAddShift, EndAddShiftDiv32,\
    EndStateShr1, NbitsShr3, EndState = 0;\
    unsigned char* Outptr;\
\
    while (framebits--){\
        EndAddShift = EndState >> 2;\
        NbitsShr3 = framebits >> 3;\
        EndAddShiftDiv32 = EndAddShift >> 5;\
        EndStateShr1 = EndState >> 1;\
        Mod32EndAddShift = EndAddShift & 31;\
        Outptr = output + NbitsShr3;\
        k = (decisions[framebits].w[EndAddShiftDiv32] >> Mod32EndAddShift) & 1;\
        kShl7 = k << 7;\
        EndState = EndStateShr1 | kShl7;\
        *Outptr = (unsigned char)EndState;\
    }\
}


///////////////////////////////////////////////////////////////////////////////
// The code goes here.
///////////////////////////////////////////////////////////////////////////////


#ifdef _VIT_NO_ASM_

extern "C" {
    __attribute__((target("sse2")))
    int decon_sse2_lut32(unsigned int framebits,
        unsigned int* piData, int inputLength, unsigned char* output) {
        Locals
        Locals2
        while (nbits--) {
            Load4SymsLUT
            ButterFly2(old_metrics, new_metrics)
            Load4SymsLUT
            ButterFly2(new_metrics, old_metrics)
            Renormalize128(old_metrics)
        }
        ChainBack
        return 0;
    }


    __attribute__((target("ssse3")))
    int decon_ssse3(unsigned int framebits,
        unsigned int* piData, int inputLength, unsigned char* output) {

        Locals
        Locals2
        while (nbits--) {
            Load4Syms
            ButterFly2(old_metrics, new_metrics)
            Load4Syms
            ButterFly2(new_metrics, old_metrics)
            Renormalize128(old_metrics)
        }
        ChainBack
        return 0;
    }
/*
    __attribute__((target("sse4.1")))
    int decon_sse41(unsigned int framebits,
        unsigned int* piData, int inputLength, unsigned char* output) {

        Locals
        Locals2
        while (nbits--) {
            Load4SymsSSE41
            ButterFly2(old_metrics, new_metrics)
            Load4SymsSSE41
            ButterFly2(new_metrics, old_metrics)
            Renormalize128(old_metrics)
        }
        ChainBack
        return 0;
    }
*/
    __attribute__((target("avx")))
    int decon_avx(unsigned int framebits,
        unsigned int* piData, int inputLength, unsigned char* output) {
 
        Locals
        Locals2
        while (nbits--) {
            Load4Syms
            ButterFly2(old_metrics, new_metrics)
            Load4Syms
            ButterFly2(new_metrics, old_metrics)
            Renormalize128(old_metrics)
        }
        ChainBack
        return 0;
    }

    __attribute__((target("avx2")))
    int decon_avx2(unsigned int framebits,
        unsigned int* piData, int inputLength, unsigned char* output) {

        Locals256
        while (nbits--) {
            Load8Syms256
            Butterfly256(old_metrics, new_metrics)
            Renormalize256(old_metrics)
        }
        ChainBack
        return 0;
    }
} // extern "C"
#endif

// The AVX512 version will be used in both configurations, because
// I can't test the (disabled) assembly-language version. 
extern "C" {
    __attribute__((target("avx512f, avx512bw, avx512vl")))
    int decon_avx5(unsigned int framebits,
        unsigned int* piData, int inputLength, unsigned char* output) {
        Locals256
            while (nbits--) {
                Load8Syms256
                Butterfly256(old_metrics, new_metrics)
                Renormalize256(old_metrics)
            }
        ChainBack
        return 0;
    }
}



// CPU dispatcher
#ifndef VIT_WRITE_LOGFILE
int deconvolve(unsigned int framebits, unsigned int* piData,
    int inputLength, unsigned char* output) {
    return deconJumpTarget(framebits, piData, inputLength, output);
}
#else
extern char *double2str(double x, int WidthPrec, char* buffer);
extern HANDLE hVitLogFile;
extern char *deconSymLow, *deconSymHig, *deconOutLow, *deconOutHig;
#ifdef VIT_WRITE_SYMBOLS
extern HANDLE hVitLogFileSymbols;
extern unsigned int* symBuf;
#endif

unsigned int counter, entry;
ULARGE_INTEGER lastEntryTime;


int deconvolve(unsigned int framebits, unsigned int* piData,
    int inputLength, unsigned char* output) {
    int retval;
    DWORD numBytesWritten;
    ULARGE_INTEGER entryTime, leaveTime;
    FILETIME fte, ftl;
    SYSTEMTIME sysTime;
    unsigned long long stackHighLimit;
    char szBuffer[320], szDuration[32], szLastCallTime[32];
    double dDuration, dLastCallTime = 0;

    if (INVALID_HANDLE_VALUE != hVitLogFile) {
        entry++;

        GetSystemTimePreciseAsFileTime(&fte);
        retval = deconJumpTarget(framebits, piData, inputLength, output);
        GetSystemTimePreciseAsFileTime(&ftl);
// Reading directly from the TIB to avoid GetCurrentThreadStackLimits()
// which is not supported in Windows 7.
// See https://en.wikipedia.org/wiki/Win32_Thread_Information_Block
        stackHighLimit =__readgsqword(0x8);
        entryTime.HighPart = fte.dwHighDateTime;
        entryTime.LowPart = fte.dwLowDateTime;
        leaveTime.HighPart = ftl.dwHighDateTime;
        leaveTime.LowPart = ftl.dwLowDateTime;
        dDuration = (leaveTime.QuadPart - entryTime.QuadPart) * 0.1;
        if (lastEntryTime.QuadPart)
            dLastCallTime = (double)(entryTime.QuadPart
                - lastEntryTime.QuadPart) / 10000.0; // ms

        lastEntryTime = entryTime;
        FileTimeToSystemTime(&fte, &sysTime);
// calculate microsecs of current second
        entryTime.QuadPart = (entryTime.QuadPart % 10000000uLL) / 10uLL;
        wsprintf(szBuffer, "%6u  %02u:%02u:%02u.%06u  dT: %s ms  "
            "TID: %5u  StU: %5i  deco: %s us  "
            "ReE: %i  fBits: %4i  Sym: 0x%p  Out: 0x%p\n",
            counter++,
            sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
            entryTime.LowPart,
            double2str(dLastCallTime, MAKELONG(8, 3),
                szLastCallTime),
            GetCurrentThreadId(),
// Stack-usage of current thread. We use the shadow-space address of the
// register rcx (it's 'framebits' here) right above of the return address on
// the stack.
            (int)(stackHighLimit - (unsigned long long) &framebits),
            double2str(dDuration, MAKELONG(6, 1), szDuration),
// If another thread enters the function until we are here the value will be 1.
// Never seen, but this may change in the future.
            --entry,
            framebits,
            piData,
            output);
        WriteFile(hVitLogFile, szBuffer, lstrlen(szBuffer),
            &numBytesWritten, NULL);

        if ((char*)piData < deconSymLow) deconSymLow = (char*)piData;
        if ((char*)piData > deconSymHig) deconSymHig = (char*)piData;
        if ((char*)output < deconOutLow) deconOutLow = (char*)output;
        if ((char*)output > deconOutHig) deconOutHig = (char*)output;


#ifdef VIT_WRITE_SYMBOLS
        if (INVALID_HANDLE_VALUE != hVitLogFileSymbols) {
            //for (int i = 0; i < ((framebits + 6) * 4); i++)
            //    symBuf[i] = *(unsigned char*)&piData[i];

            //WriteFile(hVitLogFileSymbols, symBuf, (framebits + 6) * 4,
            //    &numBytesWritten, NULL);

// Write the 32-bit values directly to the log. 
            WriteFile(hVitLogFileSymbols, piData, (framebits + 6) * 4 * sizeof(int),
                 &numBytesWritten, NULL);
        }
#endif
    }
    else
        retval = deconJumpTarget(framebits, piData, inputLength, output);

    return retval;
}
#endif
