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
*
*
* Original sceen-play (c) 2001++ by Phil Karn, KA9Q (http://www.ka9q.net/)
* Mr. Karn's software for Viterbi- and Reed-Solomon decoding can be found at
* http://www.ka9q.net/code/fec/.
*
* Original DLL-version for QIRX (c) 2018 by Clem Schmidt
* (https://qirx.softsyst.com). The sources of this DLL can be found at Clem's
* website within the download-section, more likely inside of a Linux-package.
*
* This DLL uses the 16-way-SSE2 intrinsics code from https://spiral.net
* for viterbi decoding, Copyright (c) 2005-2008, Carnegie Mellon University
* distributed under the GNU General Public License (GPL).
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <emmintrin.h>

#include "viterbi.h"


RS_LookUp* RSLU;
decision_t* desis;

static inline void CreateLookupTables() {
	int i, sr;
	unsigned char *index_of, *alpha_to;
	index_of = RSLU->RS_iof;
	alpha_to = RSLU->RS_ato;

	// Generate Galois field lookup tables 
	index_of[0] = c_nn; // log(zero) = -inf 
	alpha_to[c_nn] = 0; // alpha**-inf = 0 
	sr = 1;

	for (i = 0; i < c_nn; i++)
	{
		index_of[sr] = i;
		alpha_to[i] = sr;
		sr <<= 1;

		if (sr & 256)
			sr ^= c_gfpoly;
		sr &= c_nn;
	}

	for (i = 0; i < MOD255_TABLE_SIZE; i++)
		RSLU->RS_mod255[i] = i % c_nn;

}


// Not really needed, but we have to export the "function"
// to keep the Loader and QIRX happy.
bool __cdecl initialize()
{ 
	return true;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, 
	LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:

		RSLU = (RS_LookUp*) VirtualAlloc(0, sizeof(RS_LookUp),
			MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		if (!RSLU)
			return false;

		desis = (decision_t*) VirtualAlloc(0, 
			(384 * 24 + 6) * sizeof(decision_t), // space for 384 kbit/s. ;)
			MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		if (!desis) {
			VirtualFree(RSLU, 0, MEM_RELEASE);
			return false;
		}	
		
		CreateLookupTables();

		// kick up QIRX...
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
		break;


	case DLL_PROCESS_DETACH:

		if (RSLU)
			VirtualFree(RSLU, 0, MEM_RELEASE);

		if (desis)
			VirtualFree(desis, 0, MEM_RELEASE);
		
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return true;
}
