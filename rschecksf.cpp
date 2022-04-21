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
* Changes:
* - using a lookup-table for "x modulo 255" values up to 2048
* - forcing at least 16-byte alignment for locals on stack
* - field-indices rounded up to next power of 2 ("c_nroots + 1" became 16) to
*   tell the compiler that there is little more unused space inside of the
*   arrays and it may use XMM-regs for whatever it want.
*  
* Compiler hints: (However, it still does what it wants to do.)
* - using "load-shift-store" with XMM-register as memmove replacement
* - using XMM-reg as memcpy replacement, if array-size is 16 bytes
*
* The compiler normally uses it's own inbuild-stuff for memset or memcpy,
* because it's not worth to call the C-Lib for some bytes to fill or move.
* The linker tells you what the compiler has done, because this DLL should
* be linked with the /NODEFAULTLIB switch.
* 
* The compiler should inlining all functions into "RScheckSuperframe".
*/


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <emmintrin.h>
#include "viterbi.h"

extern RS_LookUp * RSLU;


/*
* This wrapper limits the index into our lookup-table to 2047. The highest
* value for "x" I've ever recorded during my tests was 1271, so a table-size
* of 2048 bytes might be sufficient.
* 
* BUT, after some hours running QIRX and only in two cases in the past, the
* program crashed, because "x" was much higher as expected and pointed into
* nomansland behind the end of the memory-page in which the table lays. Until
* today, I was unable to reproduce this behavior.
* 
* This additional AND instruction doesn't hurt, because it will be optimized
* away by the compiler, if it's not really needed. Search for "2047" in
* the assembly-listing file, if you want.
*/
static inline unsigned char Mod255(unsigned int x){
	return RSLU->RS_mod255[x & 2047];
}

	
static inline int DECODE_RS(unsigned char* rsBlock){

 __declspec(align (16)) unsigned char lambda[16];
 __declspec(align (16)) unsigned char s[16];
 __declspec(align (16)) unsigned char b[16];
 __declspec(align (16)) unsigned char t[16];
 __declspec(align (16)) unsigned char reg[16];
 __declspec(align (16)) unsigned char root[16];
 __declspec(align (16)) unsigned char loc[16];
 __declspec(align (16)) unsigned char omega[16];
unsigned char q, tmp, num1, num2, den, discr_r;
int deg_lambda, el, deg_omega, i, j, r, k, syn_error, count;
unsigned char *index_of, *alpha_to;
    index_of = RSLU->RS_iof;
    alpha_to = RSLU->RS_ato;
	
// form the syndromes; i.e., evaluate data(x) at roots of g(x)

	memset(s, 0, 16);  

	for (j = 1; j < c_nn; j++) {

        for (i = 0; i < c_nroots; i++) {
        
			if (s[i] == 0) {
				s[i] = rsBlock[j];
            }
            else {
				s[i] = rsBlock[j] ^ alpha_to[  Mod255  (index_of[  s[i]  ] + i)   ];
            }
        }
    }

 // Convert syndromes to index form, checking for nonzero condition
	syn_error = 0;

    for (i = 0; i < c_nroots; i++) {
        syn_error |= s[i];
		s[i] = index_of[  s[i]  ];
    }

    if (!syn_error) {
        // if syndrome is zero, data[] is a codeword and there are no
        // errors to correct. So return data[] unmodified

        count = 0;
        goto finish;
    }

	memset(lambda, 0, 16);
	lambda[0] = 1;

    for (i = 0; i < c_nroots + 1; i++)
		b[i] = index_of[lambda[i]];


    // Begin Berlekamp-Massey algorithm to determine error+erasure
    // locator polynomial

    r = 0; // no_eras;
    el = 0;// no_eras;       

    while (++r <= c_nroots) {	// r is the step number 
      // Compute discrepancy at the r-th step in poly-form
        discr_r = 0;

        for (i = 0; i < r; i++) {
        
			if ((lambda[i] != 0) && (s[r - i - 1] != c_nn)) {
                discr_r ^= alpha_to[  Mod255  (  index_of [ lambda[i] ] + s [ r - i - 1 ] )    ];
            }
        }
        discr_r = index_of[discr_r];	// Index form 

        if (discr_r == c_nn) {
            // 2 lines below: B(x) <-- x*B(x)
             // memmove(&b[1],b,c_nroots*sizeof(b[0]));
			*(__m128i*)b = _mm_slli_si128(_mm_load_si128((__m128i*)b), 1);
			b[0] = c_nn;
        }
        else {
            // 7 lines below: T(x) <-- lambda(x) - discr_r*x*b(x)
			t[0] = lambda[0];

			for (i = 0; i < c_nroots; i++) {

				if (b[i] != c_nn)
					t[i + 1] = lambda[i + 1] ^ alpha_to[  Mod255( ( discr_r + b[i] ) )   ];
                else
					t[i + 1] = lambda[i + 1];
            }

            if (2 * el <= r - 1) {
                el = r - el;
          
                 // 2 lines below: B(x) <-- inv(discr_r) *
                 // lambda(x)

                for (i = 0; i <= c_nroots; i++)
					b[i] = (lambda[i] == 0) ? c_nn : Mod255((index_of[lambda[i]] 
                        - discr_r + c_nn));
            }
            else {
                // 2 lines below: B(x) <-- x*B(x)
                // memmove(&b[1],b,c_nroots*sizeof(b[0]));
				*(__m128i*)b = _mm_slli_si128(_mm_load_si128((__m128i*)b), 1);
				b[0] = c_nn;
            }
             //memcpy(lambda,t,(c_nroots+1)*sizeof(t[0]));
			_mm_store_si128((__m128i*)lambda, _mm_load_si128((__m128i*) t));

        }
    }

    //-------------------------------------------------------------
    // Convert lambda to index form and compute deg(lambda(x))
    deg_lambda = 0;

    for (i = 0; i < c_nroots + 1; i++) {
		lambda[i] = index_of[lambda[i]];

        if (lambda[i] != c_nn)
            deg_lambda = i;
    }
    // Find roots of the error+erasure locator polynomial by Chien search
     //memcpy(&reg[1], &lambda[1], c_nroots * sizeof(reg[0]));
     // -hv-  this overwrites reg[0], too but it's never used
	 _mm_store_si128((__m128i*)reg, _mm_load_si128((__m128i*)lambda));
     

    count = 0;		// Number of roots of lambda(x)
    for (i = 1, k = c_iprim - 1; i <= c_nn; i++, k = Mod255(k + c_iprim)) {
        q = 1; // lambda[0] is always 0 

        for (j = deg_lambda; j > 0; j--) { 
            if (reg[j] != c_nn) {
				reg[j] = Mod255(reg[j] + j);
                q ^= alpha_to[reg[j]];
            }
        }

        if (q != 0)
            continue; // Not a root
          // store root (index-form) and error location number

		root[count] = i;
		loc[count] = k;
       // If we've already found max possible roots,
       // abort the search to save time
         
        if (++count == deg_lambda)
            break;
    }

    if (deg_lambda != count) {
       // deg(lambda) unequal to number of roots => uncorrectable
       // error detected
        count = -1;
        goto finish;
    }

     // Compute err+eras evaluator poly omega(x) = s(x)*lambda(x) (modulo
     // x**c_nroots). in index form. Also find deg(omega).

    deg_omega = deg_lambda - 1;
    for (i = 0; i <= deg_omega; i++) {
        tmp = 0;
        for (j = i; j >= 0; j--) {
            if ((s[i - j] != c_nn) && (lambda[j] != c_nn))
                tmp ^= alpha_to[Mod255(s[i - j] + lambda[j])];
        }
		omega[i] = index_of[tmp];
    }


     // Compute error values in poly-form. num1 = omega(inv(X(l))), num2 =
     // inv(X(l))**(FCR-1) and den = lambda_pr(inv(X(l))) all in poly-form


    for (j = count - 1; j >= 0; j--) {
        num1 = 0;
        for (i = deg_omega; i >= 0; i--) {
            if (omega[i] != c_nn)
                num1 ^= alpha_to[Mod255(omega[i] + i * root[j])];
        }
        num2 = alpha_to[Mod255(root[j] * - 1 + c_nn)];
        den = 0;

        //lambda[i+1] for i even is the formal derivative lambda_pr of lambda[i] 
        for (i = min(deg_lambda, c_nroots - 1) & ~1; i >= 0; i -= 2) {
            if (lambda[i + 1] != c_nn)
                den ^= alpha_to[ Mod255( (lambda[i + 1] + i * root[j]) ) ];
        }

        // Apply error to data 
        if (num1 != 0 && loc[j] >= 0) {
           rsBlock[loc[j]] ^= alpha_to[Mod255((index_of[num1] + index_of[num2] 
               + c_nn - index_of[den]))];
        }
    }
finish:
    return count;
}


static inline int rs_decode(unsigned char* rsBlock, unsigned char* rsIn, 
    unsigned char* rsOut)
{
    //See ETSI TS 102 563 V1.2.1 (2010-05) DAB-AAC. chapter 6.1
    static const unsigned int AdditionalBytes = 135;
    unsigned int i;

    // we have to fill the first 136 bytes of rsBlock with zeros.
    // rounding up to the next multiple of 16
    //__stosq((PDWORD64)rsBlock, 0, 17);
    for (i = 0; i < 144; i++)
        rsBlock[i] = 0;



    for (i = AdditionalBytes; i < c_nn; i++)
        rsBlock[i] = rsIn[i - AdditionalBytes];

    int decodedErrors = DECODE_RS(rsBlock);

    for (i = AdditionalBytes; i < 245; i++)
        rsOut[i - AdditionalBytes] = rsBlock[i];

    return decodedErrors;
}


/*  
* Reed-solomon forward error correction
* Input: RSDims * 120 bytes
* Output: RSDims * 110 bytes
*/
int __cdecl RScheckSuperframe(unsigned char* p, int startIx,
    unsigned int RSDims, unsigned char* outVector)
{
    UNREFERENCED_PARAMETER(startIx); // not needed, it's always zero 

__declspec(align (64)) unsigned char rsBlock[256];
__declspec(align (64)) unsigned char rsIn[128];
__declspec(align (64)) unsigned char rsOut[128];
 
    unsigned int j, k;
    int result, errors = 0;
    
    for (j = 0; j < RSDims; j++) {

        for (k = 0; k < 120; k++)
            rsIn[k] = p[(j + k * RSDims) % (RSDims * 120)];

        result = rs_decode(rsBlock, rsIn, rsOut);
        
        if (result != -1) {
            errors += result;
     
            for (k = 0; k < 110; k++)
                outVector[j + k * RSDims] = rsOut[k];
        }
        else {
            errors = -1;
            goto out;
        }
    }
out:
    return errors;
}

