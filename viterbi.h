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
*/

// Some well known defines for the viterbi decoder.
#define K 7
#define NUMSTATES  64
#define COMPUTETYPE unsigned char
#define DECISIONTYPE unsigned char
#define DECISIONTYPE_BITSIZE 8
// You can set RENORMALIZE_THRESHOLD to higher values than 150. Values up
// around 200 will still work. But if you go too far (>230), the BER rises
// up very quickly and you'll lost the audio in QIRX. So it's better to stay
// save.
#define RENORMALIZE_THRESHOLD 150

// Switching the viterbi-decoder to "unsigned int" to fit QIRX's needs.
#define VITERBI_INPUT_UINT


typedef union {
    DECISIONTYPE t[NUMSTATES / DECISIONTYPE_BITSIZE];
    unsigned int w[NUMSTATES / 32];
} decision_t;



// Some defines for the Reed-Solomon decoder
#define c_nn 255
#define c_gfpoly 285
#define c_nroots 10
#define c_iprim 1

/*
* RSLookup allocated once at startup in DllMain.
*/
#define MOD255_TABLE_SIZE 2048
struct RS_LookUp {
    unsigned char RS_mod255[MOD255_TABLE_SIZE]; // The RS-members are
    unsigned char RS_iof[256];  // heavily used in
    unsigned char RS_ato[256];  // RScheckSuperframe
};

