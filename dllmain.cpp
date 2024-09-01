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
* Original DLL-version for QIRX (c) 2018 by Clem Schmidt
* (https://qirx.softsyst.com). The sources of this DLL can be found at Clem's
* website within the download-section, more likely inside of a Linux-package.
*
* Original sceen-play (c) 2001++ by Phil Karn, KA9Q. Mr. Karn's software for 
* Viterbi- and Reed-Solomon decoding can be found at
* http://www.ka9q.net/code/fec/ or at Github.
*
* (c) 2021-2024 Heiko Vogel <hevog@gmx.de>
*
*/


#include "viterbi.h"


extern LONG WINAPI VecExceptionHandler(PEXCEPTION_POINTERS);
extern void SetupDLL();

// LUT pointer for Reed-Solomon
RS_LookUp* rsLUT;

// Experimental export. Powering up the 256-bit SIMD stages in advance.
// Can be called by QIRX right before 256-bit code should be executed.
// The procedure does nothing if the CPU doesn't supports AVX. 
// See "Warm-up period for YMM and ZMM vector instructions" in 
// "The microarchitecture of Intel, AMD, and VIA CPUs" by Agner Fog
// (www.agner.org/optimize/) for more information.  
extern "C" WAKEUPYMM noYMM;
WAKEUPYMM* wakeUpYMMJumpTarget = noYMM; // default

void WakeUpYMM() {
    wakeUpYMMJumpTarget();
}


extern "C" {
    int* symbols32LUT; // LUT pointer for SSE2
}


// variables and helpers for logging.
#ifdef VIT_WRITE_LOGFILE
HANDLE hVitLogFile = INVALID_HANDLE_VALUE;
#ifdef VIT_WRITE_SYMBOLS
HANDLE hVitLogFileSymbols = INVALID_HANDLE_VALUE;
unsigned int* symBuf;
#endif

char* deconSymLow = (char*)-1LL;
char* deconSymHig = 0;
char* deconOutLow = (char*)-1LL;
char* deconOutHig = 0;
char* rsBitsLow = (char*)-1LL;
char* rsBitsHig = 0;
char* rsOutLow = (char*)-1LL;
char* rsOutHig = 0;


void OpenLogFile() {
    char szLogFile[MAX_PATH];
    char szBaseName[MAX_PATH];
    FILETIME ft;
    SYSTEMTIME sysTime;

    GetSystemTimePreciseAsFileTime(&ft);
    FileTimeToSystemTime(&ft, &sysTime);
    wsprintf(szBaseName, "%s\\%s\\%u%02u%02u_%02u%02u%02u.",
        pVDM->szLocalAppDataBasePath, szDllName,
        sysTime.wYear, sysTime.wMonth, sysTime.wDay,
        sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
 
    wsprintf(szLogFile, "%slog", szBaseName);

    hVitLogFile = CreateFile(szLogFile, GENERIC_WRITE,
        FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

#ifdef VIT_WRITE_SYMBOLS
    wsprintf(szLogFile, "%ssym", szBaseName);
    hVitLogFileSymbols = CreateFile(szLogFile, GENERIC_WRITE,
        FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
#endif

}
#endif // VIT_WRITE_LOGFILE



// Calculating some LUT's for the Reed-Solomon decoder and the SSE2-version
// of the viterbi-decoder.
//
// The Galois field LUT for 'alpha_to' is now enlarged and contains the
// results of the term 'alpha_to[Mod255(x)]', which was used many times in
// the original code. This larger LUT makes a lot of modulo-ops superfluous
// and gives a significant speed-up.
//
// The last LUT contains 256 pre-calculated 32-bit integers for the viterbi
// decoder if SSE2 is used. These values are later used by the 'pshufd'
// instructions for broadcasting a 'symbol' into a XMM-register. Using a LUT
// is fast if it stays inside of the L1-cache. If not, the perfomance drops
// down.
void CreateLookupTables() {
    int i, sr;
    unsigned char tmp_alpha_to[256];
    unsigned char* index_of = rsLUT->RS_iof;

    // Generate Galois field lookup tables
    index_of[0] = c_nn; // log(zero) = -inf
    tmp_alpha_to[c_nn] = 0; // alpha**-inf = 0
    sr = 1;

    for (i = 0; i < c_nn; i++) {
        index_of[sr] = i;
        tmp_alpha_to[i] = sr;
        sr <<= 1;

        if (sr & 256)
            sr ^= c_gfpoly;

        sr &= c_nn;
    }

    for (i = 0; i < ATO_MOD_SIZE; i++)
        rsLUT->RS_ato_mod[i] = tmp_alpha_to[i % 255];

    for (i = 0; i < 256; i++)
        symbols32LUT[i] = i * 0x01010101;
}


// QIRX calls this function every time the user starts the DAB-receiver.
// It is used here to read the config in "viterbi.txt", so changes
// there can be applied w/o re-starting QIRX.
bool WINAPI initialize() {
    pVDM->exceptCounter = 0;
    SetupDLL();
    return true;
}


void FreeDLLMemory() {
    if (rsLUT) VFREE(rsLUT);
    if (symbols32LUT) VFREE(symbols32LUT);
    if (pVDM) VFREE(pVDM);
#if defined(VIT_WRITE_SYMBOLS) && defined(VIT_WRITE_LOGFILE)
    if (symBuf) VFREE(symBuf);
#endif
}

BOOL GetDLLMemory() {
    BOOL retVal = false;
    rsLUT = (RS_LookUp*)VCALLOC(sizeof(RS_LookUp));
    symbols32LUT = (int*)VCALLOC(256 * sizeof(int));
    pVDM = (VITDLLMEM*)VCALLOC(sizeof(VITDLLMEM));
#if defined(VIT_WRITE_SYMBOLS) && defined(VIT_WRITE_LOGFILE)
    symBuf = (unsigned char*)VCALLOC(4 * (24 * 384ll + 6));
    if (rsLUT && symbols32LUT && pVDM && symBuf)
#else
    if (rsLUT && symbols32LUT && pVDM)
#endif
        retVal = true;
    else
        FreeDLLMemory();
    return retVal;
}


BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call, LPVOID lpReserved) {
    BOOL retVal = true;
    MODULEINFO modInfo;

#ifdef VIT_WRITE_LOGFILE
#if defined(VIT_INCLUDE_THREAD_ATTACH_DETACH) || defined(VIT_INCLUDE_SUMMARY)
    unsigned long long stackLowLimit, stackHighLimit, stackLen;
    char szBuffer[320];
    DWORD numBytesWritten;
#endif
#ifdef VIT_INCLUDE_THREAD_ATTACH_DETACH
    ULARGE_INTEGER entryTime;
    FILETIME ftEntry;
    SYSTEMTIME sysTime;
#endif
#endif
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        retVal = GetDLLMemory();

        if (retVal) {
            if (GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO))) {
                pVDM->dllBaseAddress = (DWORD64)modInfo.lpBaseOfDll;
                pVDM->dllEndAddress = (DWORD64)modInfo.lpBaseOfDll + modInfo.SizeOfImage;
                pVDM->hExHandler = AddVectoredExceptionHandler(1, VecExceptionHandler);
            }

            if (pVDM->hExHandler) {
                CreateLookupTables();
                SetupDLL();
#ifdef VIT_WRITE_LOGFILE
                OpenLogFile();
#endif


#if defined(VIT_INCLUDE_THREAD_ATTACH_DETACH) && defined(VIT_WRITE_LOGFILE)
                if (INVALID_HANDLE_VALUE != hVitLogFile) {
                    stackHighLimit = __readgsqword(0x8);
                    stackLowLimit = __readgsqword(0x1478);
                    stackLen = stackHighLimit - stackLowLimit;
                    GetSystemTimePreciseAsFileTime(&ftEntry);
                    entryTime.HighPart = ftEntry.dwHighDateTime;
                    entryTime.LowPart = ftEntry.dwLowDateTime;
                    FileTimeToSystemTime(&ftEntry, &sysTime);
                    // calculate microsecs of current second
                    entryTime.QuadPart = (entryTime.QuadPart % 10000000uLL) / 10uLL;
                    wsprintf(szBuffer, "LOAD_L  %02u:%02u:%02u.%06u  TID: %5u  "
                        "StackHighLimit: 0x%p  StackLowLimit: 0x%p  StackSize: "
                        "%7i bytes  StackUsage: %5i\n",
                        sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
                        entryTime.LowPart,
                        GetCurrentThreadId(),
                        stackHighLimit,
                        stackLowLimit,
                        (int)stackLen,
                        (int)(stackHighLimit - (unsigned long long) &hModule));
                    WriteFile(hVitLogFile, szBuffer, lstrlen(szBuffer),
                        &numBytesWritten, NULL);
                }
#endif
            }
            else {
                FreeDLLMemory();
                retVal = false;            
            }
        }
        break;
        
#if defined(VIT_INCLUDE_THREAD_ATTACH_DETACH) && defined(VIT_WRITE_LOGFILE)
    case DLL_THREAD_ATTACH:
// Reading directly from the TIB to avoid GetCurrentThreadStackLimits()
// which is not supported in Windows 7.
// See https://en.wikipedia.org/wiki/Win32_Thread_Information_Block

        if (INVALID_HANDLE_VALUE != hVitLogFile) {
            stackHighLimit = __readgsqword(0x8);
            stackLowLimit = __readgsqword(0x1478);
            stackLen = stackHighLimit - stackLowLimit;
            GetSystemTimePreciseAsFileTime(&ftEntry);
            entryTime.HighPart = ftEntry.dwHighDateTime;
            entryTime.LowPart = ftEntry.dwLowDateTime;
            FileTimeToSystemTime(&ftEntry, &sysTime);
            // calculate microsecs of current second
            entryTime.QuadPart = (entryTime.QuadPart % 10000000uLL) / 10uLL;
            wsprintf(szBuffer, "THREAD  %02u:%02u:%02u.%06u  TID: %5u  "
                "StackHighLimit: 0x%p  StackLowLimit: 0x%p  StackSize: "
                "%7i bytes  StackUsage: %5i\n",
                sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
                entryTime.LowPart,
                GetCurrentThreadId(),
                stackHighLimit,
                stackLowLimit,
                (int)stackLen,
                (int)(stackHighLimit - (unsigned long long) &hModule));
            WriteFile(hVitLogFile, szBuffer, lstrlen(szBuffer),
                &numBytesWritten, NULL);
        }
        break;

    case DLL_THREAD_DETACH:

        if (INVALID_HANDLE_VALUE != hVitLogFile) {
            GetSystemTimePreciseAsFileTime(&ftEntry);
            entryTime.HighPart = ftEntry.dwHighDateTime;
            entryTime.LowPart = ftEntry.dwLowDateTime;
            FileTimeToSystemTime(&ftEntry, &sysTime);
            entryTime.QuadPart = (entryTime.QuadPart % 10000000uLL) / 10uLL;
            wsprintf(szBuffer, "THREAD  %02u:%02u:%02u.%06u  TID: %5u  "
                "DETACH\n",
                sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
                entryTime.LowPart,
                GetCurrentThreadId());
            WriteFile(hVitLogFile, szBuffer, lstrlen(szBuffer),
                &numBytesWritten, NULL);
        }
        break;
#endif

    case DLL_PROCESS_DETACH:
#if defined(VIT_INCLUDE_THREAD_ATTACH_DETACH) && defined(VIT_WRITE_LOGFILE)
        if (INVALID_HANDLE_VALUE != hVitLogFile) {
            GetSystemTimePreciseAsFileTime(&ftEntry);
            entryTime.HighPart = ftEntry.dwHighDateTime;
            entryTime.LowPart = ftEntry.dwLowDateTime;
            FileTimeToSystemTime(&ftEntry, &sysTime);
            entryTime.QuadPart = (entryTime.QuadPart % 10000000uLL) / 10uLL;
            wsprintf(szBuffer, "UNLOAD  %02u:%02u:%02u.%06u  TID: %5u  "
                "DETACH\n",
                sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
                entryTime.LowPart,
                GetCurrentThreadId());
            WriteFile(hVitLogFile, szBuffer, lstrlen(szBuffer),
                &numBytesWritten, NULL);

#ifdef VIT_INCLUDE_SUMMARY
            wsprintf(szBuffer,
                "\r\n\r\n"
                "Buffer check for function \"deconvolve\"\r\n"
                "\"input\" buffer min/max : 0x%p/0x%p\r\n"
                "Footprint in virtual address-space: %9u bytes\r\n"
                "\"output\" buffer min/max: 0x%p/0x%p\r\n"
                "Footprint in virtual address-space: %9u bytes\r\n",
                deconSymLow,
                deconSymHig,
                (unsigned int)(deconSymHig - deconSymLow),
                deconOutLow,
                deconOutHig,
                (unsigned int)(deconOutHig - deconOutLow));
            WriteFile(hVitLogFile, szBuffer, lstrlen(szBuffer),
                &numBytesWritten, NULL);

            wsprintf(szBuffer,
                "\r\n\r\n"
                "Buffer check for function \"RSCheckSuperframe\":\r\n"
                "\"input\" buffer min/max : 0x%p/0x%p\r\n"
                "Footprint in virtual address-space: %9u bytes\r\n"
                "\"output\" buffer min/max: 0x%p/0x%p\r\n"
                "Footprint in virtual address-space: %9u bytes\r\n",
                rsBitsLow,
                rsBitsHig,
                (unsigned int)(rsBitsHig - rsBitsLow),
                rsOutLow,
                rsOutHig,
                (unsigned int)(rsOutHig - rsOutLow));
            WriteFile(hVitLogFile, szBuffer, lstrlen(szBuffer),
                &numBytesWritten, NULL);
#endif
            CloseHandle(hVitLogFile);
        }
#endif

#if defined(VIT_WRITE_SYMBOLS) && defined(VIT_WRITE_LOGFILE)
        if (INVALID_HANDLE_VALUE != hVitLogFileSymbols)
            CloseHandle(hVitLogFileSymbols);
#endif      

        RemoveVectoredExceptionHandler(pVDM->hExHandler);        
        FreeDLLMemory();
        break;
    }
    return retVal;
}
