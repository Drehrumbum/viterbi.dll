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
* 
* The "Spiral-Code" is the "workhorse" of this DLL and should run as fast as
* possible. The "Need for speed" motto, you know. ;) It's absolutely important
* to chose the right compiler and settings. Then you will see some kind of
* "today's compiler-magic", if you take a look into the assembly-language
* listing the compiler writes for you. And you should look into it.
* 
* If you use the SSE or AVX instruction-sets, your compiler MUST USE all of
* the 16 XMM-registers available. Otherwise your code will be very slow - no
* chance to win only an flowerpot with it.
*
* Because the contents of our little "BranchTable" is known at compile-time,
* the compiler loads (and keeps) the needed values in XMM-registers at
* function-entry to avoid further access within the main-loop, if possible. 
* Well, there's a lot of "register-pressure" with SSE or AVX, so two reads 
* per round from our table are still necessary.
* 
* By using the AVX512 instruction-set, the compiler is able to generate code
* that holds much more data inside of the CPU. The clang-compiler, for 
* instance, uses the registers XMM0 - XMM5 (always scratch) and XMM16 - XMM26
* (no need to save and restore them) for this purpose. The Intel-compiler does
* almost the same.
*
* Please note: The current "spiral-code" is not the best solution for AVX
* and AVX512, because the full bit-width of the registers is not used for 
* calculations.
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <emmintrin.h>

#include "viterbi.h"
#include "branchtable.h"

extern decision_t * desis;


int __cdecl deconvolve(unsigned int framebits,  

#ifdef VITERBI_INPUT_UINT	
	unsigned int*  pDataSource,
#else
	unsigned char* pDataSource,
#endif

	int inputLength, unsigned char* output)
{
	UNREFERENCED_PARAMETER(inputLength);

__declspec(align (64)) COMPUTETYPE OldMet[NUMSTATES];
__declspec(align (64)) COMPUTETYPE NewMet[NUMSTATES];

// init old metric 
	memset(OldMet, 63, sizeof(OldMet));
	OldMet[0] = 0;

	decision_t *desicions = desis;

	unsigned int SpiralLoops = framebits / 2 + 2;
/*
* Trying to fetch the first 64 bytes from the input-buffer into the cache.
* Unfortunately, QIRX's buffers are not well aligned (8 bytes) and not fixed
* in memory, so a input-buffer may sometimes start only eight bytes away from
* the end of a cache-line and we'll get absolutely no "Need for speed"-points
* for our prefetch-hint.
*/
	_mm_prefetch((const char*)pDataSource, _MM_HINT_T0);



// "Spiral-Code" goes here. Run Fury, run!!! ;)

	for (unsigned long long i9 = 0; i9 <= SpiralLoops; i9++) {

#ifdef VITERBI_INPUT_UINT
		unsigned int a139, a149, a159, a169, a365, a375, a385, a396;
		unsigned int *a138, *a148, *a158, *a168, *b14, *a364, 
			*a374, *a384, *a395, *b35;
#else
		unsigned char a139, a149, a159, a169, a365, a375, a385, a396;
		unsigned char *a138, *a148, *a158, *a168, *b14, *a364,
			*a374, *a384, *a395, *b35;
#endif // VITERBI_INPUT_UINT

		unsigned long long a137;
		short int  s20, s21, s26, s27;

		short int* a183, * a184, * a185, * a223, * a224;
		__m128i* a135, * a136, * a141, * a151, * a161, * a171, * a186
			, * a187, * a188, * a189, * a190, * a197, * a204, * a211, * a225
			, * a226;
		__m128i a144, a145, a154, a155, a164, a165, a174
			, a175, a178, a179, a193, a194, a200, a201, a207
			, a208, a214, a215, a218, a219;
		__m128i a140, a142, a143, a146, a147, a150, a152
			, a153, a156, a157, a160, a162, a163, a166, a167
			, a170, a172, a173, a176, a177, a180, a181, a182
			, a191, a192, a195, a196, a198, a199, a202, a203
			, a205, a206, a209, a210, a212, a213, a216, a217
			, a220, a221, a222, b15, b16, b17, b18, d10
			, d11, d12, d9, m23, m24, m25, m26, m27
			, m28, m29, m30, s18, s19, s22, s23, s24
			, s25, s28, s29, t13, t14, t15, t16, t17
			, t18;
		a135 = ((__m128i*) OldMet);
		s18 = *(a135);
		a136 = (a135 + 2);
		s19 = *(a136);
		a137 = (8 * i9);
		a138 = (pDataSource + a137);
		a139 = *(a138);
		a140 = _mm_set1_epi8(a139);
		a141 = ((__m128i*) BranchTable);
		a142 = *(a141);
		a143 = _mm_xor_si128(a140, a142);
		a144 = ((__m128i) a143);
		a145 = _mm_srli_epi16(a144, 2);
		a146 = ((__m128i) a145);

		a147 = _mm_and_si128(a146, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63, 
			63, 63, 63, 63, 63, 63, 63, 63));

		b14 = (a137 + pDataSource);
		a148 = (b14 + 1);
		a149 = *(a148);
		a150 = _mm_set1_epi8(a149);
		a151 = (a141 + 2);
		a152 = *(a151);
		a153 = _mm_xor_si128(a150, a152);
		a154 = ((__m128i) a153);
		a155 = _mm_srli_epi16(a154, 2);
		a156 = ((__m128i) a155);

		a157 = _mm_and_si128(a156, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63,
			63, 63, 63, 63, 63, 63, 63, 63));

		a158 = (b14 + 2);
		a159 = *(a158);
		a160 = _mm_set1_epi8(a159);
		a161 = (a141 + 4);
		a162 = *(a161);
		a163 = _mm_xor_si128(a160, a162);
		a164 = ((__m128i) a163);
		a165 = _mm_srli_epi16(a164, 2);
		a166 = ((__m128i) a165);

		a167 = _mm_and_si128(a166, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63, 
			63, 63, 63, 63, 63, 63, 63, 63));

		a168 = (b14 + 3);
		a169 = *(a168);
		a170 = _mm_set1_epi8(a169);
		a171 = (a141 + 6);
		a172 = *(a171);
		a173 = _mm_xor_si128(a170, a172);
		a174 = ((__m128i) a173);
		a175 = _mm_srli_epi16(a174, 2);
		a176 = ((__m128i) a175);

		a177 = _mm_and_si128(a176, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63, 
			63, 63, 63, 63, 63, 63, 63, 63));

		b15 = _mm_adds_epu8(a147, a157);
		b16 = _mm_adds_epu8(b15, a167);
		t13 = _mm_adds_epu8(b16, a177);
		a178 = ((__m128i) t13);
		a179 = _mm_srli_epi16(a178, 2);
		a180 = ((__m128i) a179);

		t14 = _mm_and_si128(a180, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63,
			63, 63, 63, 63, 63, 63, 63, 63));
		t15 = _mm_subs_epu8(_mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63, 
			63, 63, 63, 63, 63, 63, 63, 63), t14);

		m23 = _mm_adds_epu8(s18, t14);
		m24 = _mm_adds_epu8(s19, t15);
		m25 = _mm_adds_epu8(s18, t15);
		m26 = _mm_adds_epu8(s19, t14);
		a181 = _mm_min_epu8(m24, m23);
		d9 = _mm_cmpeq_epi8(a181, m24);
		a182 = _mm_min_epu8(m26, m25);
		d10 = _mm_cmpeq_epi8(a182, m26);
		s20 = _mm_movemask_epi8(_mm_unpacklo_epi8(d9, d10));
		a183 = ((short int*)desicions);
		a184 = (a183 + a137);
		*(a184) = s20;
		s21 = _mm_movemask_epi8(_mm_unpackhi_epi8(d9, d10));
		a185 = (a184 + 1);
		*(a185) = s21;
		s22 = _mm_unpacklo_epi8(a181, a182);
		s23 = _mm_unpackhi_epi8(a181, a182);
		a186 = ((__m128i*) NewMet);
		*(a186) = s22;
		a187 = (a186 + 1);
		*(a187) = s23;
		a188 = (a135 + 1);
		s24 = *(a188);
		a189 = (a135 + 3);
		s25 = *(a189);
		a190 = (a141 + 1);
		a191 = *(a190);
		a192 = _mm_xor_si128(a140, a191);
		a193 = ((__m128i) a192);
		a194 = _mm_srli_epi16(a193, 2);
		a195 = ((__m128i) a194);

		a196 = _mm_and_si128(a195, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63,
			63, 63, 63, 63, 63, 63, 63, 63));

		a197 = (a141 + 3);
		a198 = *(a197);
		a199 = _mm_xor_si128(a150, a198);
		a200 = ((__m128i) a199);
		a201 = _mm_srli_epi16(a200, 2);
		a202 = ((__m128i) a201);

		a203 = _mm_and_si128(a202, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63, 
			63, 63, 63, 63, 63, 63, 63, 63));

		a204 = (a141 + 5);
		a205 = *(a204);
		a206 = _mm_xor_si128(a160, a205);
		a207 = ((__m128i) a206);
		a208 = _mm_srli_epi16(a207, 2);
		a209 = ((__m128i) a208);

		a210 = _mm_and_si128(a209, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63,
			63, 63, 63, 63, 63, 63, 63, 63));

		a211 = (a141 + 7);
		a212 = *(a211);
		a213 = _mm_xor_si128(a170, a212);
		a214 = ((__m128i) a213);
		a215 = _mm_srli_epi16(a214, 2);
		a216 = ((__m128i) a215);

		a217 = _mm_and_si128(a216, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63,
			63, 63, 63, 63, 63, 63, 63, 63));

		b17 = _mm_adds_epu8(a196, a203);
		b18 = _mm_adds_epu8(b17, a210);
		t16 = _mm_adds_epu8(b18, a217);
		a218 = ((__m128i) t16);
		a219 = _mm_srli_epi16(a218, 2);
		a220 = ((__m128i) a219);

		t17 = _mm_and_si128(a220, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63,
			63, 63, 63, 63, 63, 63, 63, 63));
		t18 = _mm_subs_epu8(_mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63, 
			63, 63, 63, 63, 63, 63, 63, 63), t17);

		m27 = _mm_adds_epu8(s24, t17);
		m28 = _mm_adds_epu8(s25, t18);
		m29 = _mm_adds_epu8(s24, t18);
		m30 = _mm_adds_epu8(s25, t17);
		a221 = _mm_min_epu8(m28, m27);
		d11 = _mm_cmpeq_epi8(a221, m28);
		a222 = _mm_min_epu8(m30, m29);
		d12 = _mm_cmpeq_epi8(a222, m30);
		s26 = _mm_movemask_epi8(_mm_unpacklo_epi8(d11, d12));
		a223 = (a184 + 2);
		*(a223) = s26;
		s27 = _mm_movemask_epi8(_mm_unpackhi_epi8(d11, d12));
		a224 = (a184 + 3);
		*(a224) = s27;
		s28 = _mm_unpacklo_epi8(a221, a222);
		s29 = _mm_unpackhi_epi8(a221, a222);
		a225 = (a186 + 2);
		*(a225) = s28;
		a226 = (a186 + 3);
		*(a226) = s29;

		if ((((unsigned char*)NewMet)[0] > RENORMALIZE_THRESHOLD)) {
			__m128i m5, m6;
			m5 = ((__m128i*) NewMet)[0];
			m5 = _mm_min_epu8(m5, ((__m128i*)  NewMet)[1]);
			m5 = _mm_min_epu8(m5, ((__m128i*)  NewMet)[2]);
			m5 = _mm_min_epu8(m5, ((__m128i*)  NewMet)[3]);
			__m128i m7;
			m7 = _mm_min_epu8(_mm_srli_si128(m5, 8), m5);
			m7 = ((__m128i) _mm_min_epu8(((__m128i) _mm_srli_epi64(m7, 32)), ((__m128i) m7)));
			m7 = ((__m128i) _mm_min_epu8(((__m128i) _mm_srli_epi64(m7, 16)), ((__m128i) m7)));
			m7 = ((__m128i) _mm_min_epu8(((__m128i) _mm_srli_epi64(m7, 8)), ((__m128i) m7)));
			m7 = _mm_unpacklo_epi8(m7, m7);
			m7 = _mm_shufflelo_epi16(m7, _MM_SHUFFLE(0, 0, 0, 0));
			m6 = _mm_unpacklo_epi64(m7, m7);
			((__m128i*)  NewMet)[0] = _mm_subs_epu8(((__m128i*)  NewMet)[0], m6);
			((__m128i*)  NewMet)[1] = _mm_subs_epu8(((__m128i*)  NewMet)[1], m6);
			((__m128i*)  NewMet)[2] = _mm_subs_epu8(((__m128i*)  NewMet)[2], m6);
			((__m128i*)  NewMet)[3] = _mm_subs_epu8(((__m128i*)  NewMet)[3], m6);
		}


		unsigned long long a363;
		short int s48, s49, s54, s55;
		short int* a410, * a411, * a412, * a450, * a451, * b38;
		__m128i* a361, * a362, * a367, * a377, * a388, * a398, * a413
			, * a414, * a415, * a416, * a417, * a424, * a431, * a438, * a452
			, * a453;
		__m128i a370, a371, a380, a381, a391, a392, a401
			, a402, a405, a406, a420, a421, a427, a428, a434
			, a435, a441, a442, a445, a446;
		__m128i a366, a368, a369, a372, a373, a376, a378
			, a379, a382, a383, a387, a389, a390, a393, a394
			, a397, a399, a400, a403, a404, a407, a408, a409
			, a418, a419, a422, a423, a425, a426, a429, a430
			, a432, a433, a436, a437, a439, a440, a443, a444
			, a447, a448, a449, b36, b37, b39, b40, d17
			, d18, d19, d20, m39, m40, m41, m42, m43
			, m44, m45, m46, s46, s47, s50, s51, s52
			, s53, s56, s57, t25, t26, t27, t28, t29
			, t30;
		a361 = ((__m128i*)  NewMet);
		s46 = *(a361);
		a362 = (a361 + 2);
		s47 = *(a362);
		a363 = (8 * i9);
		b35 = (a363 + pDataSource);
		a364 = (b35 + 4);
		a365 = *(a364);
		a366 = _mm_set1_epi8(a365);
		a367 = ((__m128i*) BranchTable);
		a368 = *(a367);
		a369 = _mm_xor_si128(a366, a368);
		a370 = ((__m128i) a369);
		a371 = _mm_srli_epi16(a370, 2);
		a372 = ((__m128i) a371);

		a373 = _mm_and_si128(a372, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63,
			63, 63, 63, 63, 63, 63, 63, 63));

		a374 = (b35 + 5);
		a375 = *(a374);
		a376 = _mm_set1_epi8(a375);
		a377 = (a367 + 2);
		a378 = *(a377);
		a379 = _mm_xor_si128(a376, a378);
		a380 = ((__m128i) a379);
		a381 = _mm_srli_epi16(a380, 2);
		a382 = ((__m128i) a381);

		a383 = _mm_and_si128(a382, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63, 
			63, 63, 63, 63, 63, 63, 63, 63));

		a384 = (b35 + 6);
		a385 = *(a384);
		a387 = _mm_set1_epi8(a385);
		a388 = (a367 + 4);
		a389 = *(a388);
		a390 = _mm_xor_si128(a387, a389);
		a391 = ((__m128i) a390);
		a392 = _mm_srli_epi16(a391, 2);
		a393 = ((__m128i) a392);

		a394 = _mm_and_si128(a393, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63,
			63, 63, 63, 63, 63, 63, 63, 63));

		a395 = (b35 + 7);
		a396 = *(a395);
		a397 = _mm_set1_epi8(a396);
		a398 = (a367 + 6);
		a399 = *(a398);
		a400 = _mm_xor_si128(a397, a399);
		a401 = ((__m128i) a400);
		a402 = _mm_srli_epi16(a401, 2);
		a403 = ((__m128i) a402);

		a404 = _mm_and_si128(a403, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63, 
			63, 63, 63, 63, 63, 63, 63, 63));

		b36 = _mm_adds_epu8(a373, a383);
		b37 = _mm_adds_epu8(b36, a394);
		t25 = _mm_adds_epu8(b37, a404);
		a405 = ((__m128i) t25);
		a406 = _mm_srli_epi16(a405, 2);
		a407 = ((__m128i) a406);

		t26 = _mm_and_si128(a407, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63, 
			63, 63, 63, 63, 63, 63, 63, 63));
		t27 = _mm_subs_epu8(_mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63, 
			63, 63, 63, 63, 63, 63, 63, 63), t26);

		m39 = _mm_adds_epu8(s46, t26);
		m40 = _mm_adds_epu8(s47, t27);
		m41 = _mm_adds_epu8(s46, t27);
		m42 = _mm_adds_epu8(s47, t26);
		a408 = _mm_min_epu8(m40, m39);
		d17 = _mm_cmpeq_epi8(a408, m40);
		a409 = _mm_min_epu8(m42, m41);
		d18 = _mm_cmpeq_epi8(a409, m42);
		s48 = _mm_movemask_epi8(_mm_unpacklo_epi8(d17, d18));
		a410 = ((short int*)desicions);
		b38 = (a410 + a363);
		a411 = (b38 + 4);
		*(a411) = s48;
		s49 = _mm_movemask_epi8(_mm_unpackhi_epi8(d17, d18));
		a412 = (b38 + 5);
		*(a412) = s49;
		s50 = _mm_unpacklo_epi8(a408, a409);
		s51 = _mm_unpackhi_epi8(a408, a409);
		a413 = ((__m128i*)  OldMet);
		*(a413) = s50;
		a414 = (a413 + 1);
		*(a414) = s51;
		a415 = (a361 + 1);
		s52 = *(a415);
		a416 = (a361 + 3);
		s53 = *(a416);
		a417 = (a367 + 1);
		a418 = *(a417);
		a419 = _mm_xor_si128(a366, a418);
		a420 = ((__m128i) a419);
		a421 = _mm_srli_epi16(a420, 2);
		a422 = ((__m128i) a421);

		a423 = _mm_and_si128(a422, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63,
			63, 63, 63, 63, 63, 63, 63, 63));

		a424 = (a367 + 3);
		a425 = *(a424);
		a426 = _mm_xor_si128(a376, a425);
		a427 = ((__m128i) a426);
		a428 = _mm_srli_epi16(a427, 2);
		a429 = ((__m128i) a428);

		a430 = _mm_and_si128(a429, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63, 
			63, 63, 63, 63, 63, 63, 63, 63));

		a431 = (a367 + 5);
		a432 = *(a431);
		a433 = _mm_xor_si128(a387, a432);
		a434 = ((__m128i) a433);
		a435 = _mm_srli_epi16(a434, 2);
		a436 = ((__m128i) a435);

		a437 = _mm_and_si128(a436, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63, 
			63, 63, 63, 63, 63, 63, 63, 63));

		a438 = (a367 + 7);
		a439 = *(a438);
		a440 = _mm_xor_si128(a397, a439);
		a441 = ((__m128i) a440);
		a442 = _mm_srli_epi16(a441, 2);
		a443 = ((__m128i) a442);

		a444 = _mm_and_si128(a443, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63, 
			63, 63, 63, 63, 63, 63, 63, 63));

		b39 = _mm_adds_epu8(a423, a430);
		b40 = _mm_adds_epu8(b39, a437);
		t28 = _mm_adds_epu8(b40, a444);
		a445 = ((__m128i) t28);
		a446 = _mm_srli_epi16(a445, 2);
		a447 = ((__m128i) a446);

		t29 = _mm_and_si128(a447, _mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63,
			63, 63, 63, 63, 63, 63, 63, 63));
		t30 = _mm_subs_epu8(_mm_set_epi8(63, 63, 63, 63, 63, 63, 63, 63, 
			63, 63, 63, 63, 63, 63, 63, 63), t29);

		m43 = _mm_adds_epu8(s52, t29);
		m44 = _mm_adds_epu8(s53, t30);
		m45 = _mm_adds_epu8(s52, t30);
		m46 = _mm_adds_epu8(s53, t29);
		a448 = _mm_min_epu8(m44, m43);
		d19 = _mm_cmpeq_epi8(a448, m44);
		a449 = _mm_min_epu8(m46, m45);
		d20 = _mm_cmpeq_epi8(a449, m46);
		s54 = _mm_movemask_epi8(_mm_unpacklo_epi8(d19, d20));
		a450 = (b38 + 6);
		*(a450) = s54;
		s55 = _mm_movemask_epi8(_mm_unpackhi_epi8(d19, d20));
		a451 = (b38 + 7);
		*(a451) = s55;
		s56 = _mm_unpacklo_epi8(a448, a449);
		s57 = _mm_unpackhi_epi8(a448, a449);
		a452 = (a413 + 2);
		*(a452) = s56;
		a453 = (a413 + 3);
		*(a453) = s57;

		if ((((unsigned char*)OldMet)[0] > RENORMALIZE_THRESHOLD)) {
			__m128i m12, m13;
			m12 = ((__m128i*)  OldMet)[0];
			m12 = _mm_min_epu8(m12, ((__m128i*)  OldMet)[1]);
			m12 = _mm_min_epu8(m12, ((__m128i*)  OldMet)[2]);
			m12 = _mm_min_epu8(m12, ((__m128i*)  OldMet)[3]);
			__m128i m14;
			m14 = _mm_min_epu8(_mm_srli_si128(m12, 8), m12);
			m14 = ((__m128i) _mm_min_epu8(((__m128i) _mm_srli_epi64(m14, 32)), ((__m128i) m14)));
			m14 = ((__m128i) _mm_min_epu8(((__m128i) _mm_srli_epi64(m14, 16)), ((__m128i) m14)));
			m14 = ((__m128i) _mm_min_epu8(((__m128i) _mm_srli_epi64(m14, 8)), ((__m128i) m14)));
			m14 = _mm_unpacklo_epi8(m14, m14);
			m14 = _mm_shufflelo_epi16(m14, _MM_SHUFFLE(0, 0, 0, 0));
			m13 = _mm_unpacklo_epi64(m14, m14);
			((__m128i*)  OldMet)[0] = _mm_subs_epu8(((__m128i*)  OldMet)[0], m13);
			((__m128i*)  OldMet)[1] = _mm_subs_epu8(((__m128i*)  OldMet)[1], m13);
			((__m128i*)  OldMet)[2] = _mm_subs_epu8(((__m128i*)  OldMet)[2], m13);
			((__m128i*)  OldMet)[3] = _mm_subs_epu8(((__m128i*)  OldMet)[3], m13);
		}
	}


// Viterbi chainback
// ADDSHIFT and SUBSHIFT make sure that the thing returned is a byte.

#define ADDSHIFT 2

	// Make room beyond the end of the encoder register so we can
	// accumulate a full byte of decoded data
	
	desicions += 6; // Look past tail

	/* 
	* The store into data[] only needs to be done every 8 bits.
	* But this avoids a conditional branch, and the writes will
	* combine in the cache anyway
	*/

	unsigned int k, kShl7, EndAddShift, Mod32EndAddShift, EndAddShiftDiv32, 
		EndStateShr1, EndState = 0;
	unsigned int NbitsShr3;
	unsigned char* Outptr;

	while (framebits-- != 0) {
		EndAddShift = EndState >> ADDSHIFT;
		NbitsShr3 = framebits >> 3;
		EndAddShiftDiv32 = EndAddShift >> 5;
		EndStateShr1 = EndState >> 1;
		Mod32EndAddShift = EndAddShift & 31;
		Outptr = output + NbitsShr3;
		k = (desicions[framebits].w[EndAddShiftDiv32] >> Mod32EndAddShift) & 1;
		kShl7 = k << 7;
		EndState = EndStateShr1 | kShl7;
		*Outptr = (unsigned char)EndState;
	}
	return 0;
}

