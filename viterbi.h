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
* (c) 2021-2023 Heiko Vogel <hevog@gmx.de>
* 
*/

#define ALIGN(a) __declspec(align(a))
#define align(a) ALIGN(a)

// If VIT_WRITE_LOGFILE is defined, the DLL will be compiled with code
// that writes a lot of information about the viterbi-decoder and the
// RS-decoder into a file. This logfile contains, among some other things,
// timestamps, ThreadIDs and the addresses of the buffers transferred by
// QIRX. The file is located at 
//           ...\APPDATA\LOCAL\viterbi\YYYYMMDD_HHMMSS.log
// and its size increases at a rate of approximately 65 MB/hr, so don't
// forget to swap this build of the DLL back to the production DLL after 
// your investigations.
//#define VIT_WRITE_LOGFILE

// Includes information about the creation and termination of threads.
//#define VIT_INCLUDE_THREAD_ATTACH_DETACH

// Includes a short summary at the end of the logfile.
//#define VIT_INCLUDE_SUMMARY


// If VIT_WRITE_LOGFILE is defined you can define VIT_WRITE_SYMBOLS too.
// This creates a second file in which the symbols for the viterbi-decoder
// are stored in binary form (char). The file name is the same like above
// but has the extension *.sym. This file grows real fast!
//#define VIT_WRITE_SYMBOLS


// RENORMALIZE_THRESHOLD for the viterbi-decoders in 'deconvolve.cpp'.
// Use RENORMALIZE_THRESHOLD in 'const.inc' for the assembly-language parts.
// Keep the value below 180.
#define RENORMALIZE_THRESHOLD 150


// "decisions" for the viterbi decoder.
typedef struct {
    unsigned int w[2];
} decision_t;

// Some defines for the Reed-Solomon decoder
#define c_nn 255
#define c_gfpoly 285
#define c_nroots 10


// The Galois field LUT struct
#define ATO_MOD_SIZE 768 // must be 512 or 768
struct RS_LookUp {
    unsigned char RS_ato_mod[ATO_MOD_SIZE];
    unsigned char RS_iof[256];
};

struct DispatcherCMD {
    int InstruSet;  // wanted instruction-set (0-4)
    int ShowIRect;  // show info-rect on screen (0 or 1)
};

typedef int DECON (unsigned int, unsigned int*, int, unsigned char*);
typedef void WAKEUPYMM();
