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
* This file contains the code for setting up the DLL. 
* 
* (c) 2021-2023 Heiko Vogel <hevog@gmx.de>
*
*/

#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "viterbi.h"

const char
szDllName[] = "viterbi",
szExtDLL[] = ".dll",
szExtTXT[] = ".txt",
szSSE2Lut[] = "SSE2",
szSSSE3[] = "SSSE3",
//szSSE41[] = "SSE4.1",
szAVX[] = "AVX",
szAVX2[] = "AVX2",
szAVX5[] = "AVX512";


// extern decoder-functions in deconvolve.cpp or in the assembly-parts.
extern "C" DECON decon_sse2_lut32, decon_ssse3, decon_avx,
           decon_avx2, decon_avx5;

// setting the default decoder-function
DECON* deconJumpTarget = decon_sse2_lut32;

extern "C" WAKEUPYMM xorYMM;
extern WAKEUPYMM* wakeUpYMMJumpTarget;

// external CPU-check defines
#include "getcpucaps.h"

int  GetUserLocalAppDataFolder(char* szLocalAppDataPath);
int  CreateUserLocalAppDataFolder(char* szLocalAppDataPath);
void CreateIniFile(char* szLocalAppDataPath);
int  ReadIniFile(char* szLocalAppDataPath, DispatcherCMD* dcmd);
void SetupCpuDispatcher(DispatcherCMD* dcmd);
void ScreenWriteXY(int xpos, int ypos, char* buffer);

#include "inifiletext.h"


void SetupDLL() {
    char szAppDataPath[MAX_PATH];
    int iPathStatus, retval = 0;
    DispatcherCMD dcmd;

// setting the defaults
    dcmd.ShowIRect = 1; // show the Info-Rectangle on screen
    dcmd.InstruSet = 5; // invalid value, triggers automatic dispatching

    iPathStatus = GetUserLocalAppDataFolder(szAppDataPath);
// 1: the path "...\AppData\Local\" exists
// 2: the path "...\AppData\Local\viterbi\" exists
    if (2 == iPathStatus)
       retval = ReadIniFile(szAppDataPath, &dcmd);

    SetupCpuDispatcher(&dcmd);

// Try to create our directory and the config-file
    if (!retval && iPathStatus) {
        retval = 1; // reusing variable

        if (1 == iPathStatus)
            retval = CreateUserLocalAppDataFolder(szAppDataPath);

        if (retval)
            CreateIniFile(szAppDataPath);
    }
}

// GetUserLocalAppDataFolderiPathStatus values:
// 0 : env-variable not set, path too long or a file "viterbi" exists
//     in the "...\AppData\Local\" directory for unknown reasons
// 1 : the path "...\AppData\Local\" exists
// 2 : the path "...\AppData\Local\viterbi\" exists
int GetUserLocalAppDataFolder(char* szLocalAppDataPath) {
    char tempbuf[MAX_PATH];
    WIN32_FIND_DATA wfd;
    HANDLE hSearch;
    int PathLen, iPathStatus = 0; // assume error

// If GetEnvironmentVariable() fails or if the current path is too long
// for appending our directory and the filename ("\viterbi\viterbi.txt")
// later, we'll better fail at this point.
    PathLen = GetEnvironmentVariable("LOCALAPPDATA", tempbuf, MAX_PATH - 21);

    if (!PathLen || PathLen >= MAX_PATH - 21) {
        *szLocalAppDataPath = 0;
        return iPathStatus;
    }

// Otherwise we returning at least the "AppData\Local" path, so our own
// directory can be created later w/o asking for the path again.
    lstrcpy(szLocalAppDataPath, tempbuf);
    iPathStatus = 1;

// Append our own directory-name to the path and search for it
    wsprintf(tempbuf, "%s\\%s", szLocalAppDataPath, szDllName);
    hSearch = FindFirstFile(tempbuf, &wfd);

    if (INVALID_HANDLE_VALUE != hSearch) {
// We have a match, but this can be a file. This is unlikely, but
// who knows? Let's check it.

        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // It's a directory...
            lstrcpy(szLocalAppDataPath, tempbuf); // copy the path
            iPathStatus = 2; // all is fine
            SetFileAttributes(tempbuf, FILE_ATTRIBUTE_NORMAL);
        }
        FindClose(hSearch);
    }
    return iPathStatus;
}

inline int CreateUserLocalAppDataFolder(char* szLocalAppDataPath) {
    int retval = 0;
    char tempbuf[MAX_PATH];
    wsprintf(tempbuf, "%s\\%s", szLocalAppDataPath, szDllName);

    if (CreateDirectory(tempbuf, NULL)) {
        lstrcpy(szLocalAppDataPath, tempbuf);
        retval++;
    }
    return retval;
}

// Try to create the config-file. Normally this shouldn't
// go wrong. If so, there is nothing we can do and the
// automatic comes into play again (file not found -> auto).
inline void CreateIniFile(char* szLocalAppDataPath) {
    char tempbuf[MAX_PATH];
    DWORD NumBytesWritten;
    wsprintf(tempbuf, "%s\\%s%s", szLocalAppDataPath, szDllName,
        szExtTXT);

    HANDLE hIni = CreateFile(tempbuf, GENERIC_WRITE,
        0, 0, CREATE_ALWAYS, 0, 0);

    if (INVALID_HANDLE_VALUE != hIni) {
        WriteFile(hIni, szIniFileText, lstrlen(szIniFileText),
            &NumBytesWritten, NULL);
        CloseHandle(hIni);
    }
}

// Try to read the first three bytes of "viterbi.txt" and update "dcmd",
// if possible. We'll get something like this: "x:x"
inline int ReadIniFile(char* szLocalAppDataPath, DispatcherCMD* dcmd) {
    char tempbuf[MAX_PATH];
    unsigned char chFileInput[4]{};
    DWORD dNumBytesRead;
    HANDLE hIn;
    int retval = 0;

    wsprintf(tempbuf, "%s\\%s%s", szLocalAppDataPath, szDllName, szExtTXT);
    hIn = CreateFile(tempbuf, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);

    if (INVALID_HANDLE_VALUE != hIn) {
        ReadFile(hIn, &chFileInput, 3, &dNumBytesRead, NULL);
        CloseHandle(hIn);
        retval = 1;
// Check if we got data and if they are valid. If not, do nothing (using the
// defaults).
        if (3 == dNumBytesRead) {

            if (chFileInput[0] >= '0' && chFileInput[0] < '5')
                dcmd->InstruSet = chFileInput[0] - '0';   // 0 to 4
            if (chFileInput[2] >= '0' && chFileInput[2] < '2')
                dcmd->ShowIRect = (chFileInput[2] - '0'); // 0 or 1
        }
    }
    return retval;
}

inline void SetupCpuDispatcher(DispatcherCMD* dcmd) {
    char szMessageText[64];
    int iCPUCaps;

    wsprintf(szMessageText, "%s%s: ", szDllName, szExtDLL);

    iCPUCaps = GetCPUCaps();

 // Set the highest possible instruction set (automatic)
    int iSelect = 0; // assume SSE2

    if (iCPUCaps & CpuHasSSSE3) iSelect = 1;

    if (iCPUCaps & CpuHasAVX1) {
        iSelect = 2;
        wakeUpYMMJumpTarget = xorYMM; // enable YMM wake-up
    }
    if (iCPUCaps & CpuHasAVX2)  iSelect = 3;
    if (iCPUCaps & CpuHasAVX5)  iSelect = 4;

 // Now set the user-requested instruction set.
 // Selecting a *lower* instruction-set is allowed, of course.

    switch (dcmd->InstruSet) {

    case 1:
        if (iCPUCaps & CpuHasSSSE3) iSelect = 1;
        break;

    case 2:
        if (iCPUCaps & CpuHasAVX1) iSelect = 2;
        break;
    case 3:
        if (iCPUCaps & CpuHasAVX2) iSelect = 3;
        break;
    case 4:
        if (iCPUCaps & CpuHasAVX5) iSelect = 4;
        break;
    default:
        iSelect = 0;
        break;
    }

    // Updating the jump-target
    switch (iSelect) {

    case 1:
        lstrcat(szMessageText, szSSSE3);
        deconJumpTarget = decon_ssse3;
        break;

    case 2:
        lstrcat(szMessageText, szAVX);
        deconJumpTarget = decon_avx;
        break;

    case 3:
        lstrcat(szMessageText, szAVX2);
        deconJumpTarget = decon_avx2;
        break;

    case 4:
        lstrcat(szMessageText, szAVX5);
        deconJumpTarget = decon_avx5;
        break;

    default:
        lstrcat(szMessageText, szSSE2Lut);
        deconJumpTarget = decon_sse2_lut32;
        break;
    }

    if (dcmd->ShowIRect)
        ScreenWriteXY(10, 10, szMessageText);

}

// I know, writing directly on the screen is bad... You can disable it
// in viterbi.txt.
void ScreenWriteXY(int xpos, int ypos, char* buffer) {
    HFONT hFont;
    RECT rc{};
    HGDIOBJ OldPen, OldBrush, OldFont;
    HDC hdc = GetDC(0); // screen

    hFont = CreateFont(22, 0, 0, 0, 0, FALSE, FALSE,
        0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS, NULL);

    OldPen = SelectObject(hdc, GetStockObject(DC_PEN));
    SetDCPenColor(hdc, RGB(0xcc, 0xcc, 0xcc)); // light-grey border

    OldBrush = SelectObject(hdc, GetStockObject(DC_BRUSH));
    SetDCBrushColor(hdc, RGB(0x66, 0, 0));  // dark-red for background
    OldFont = SelectObject(hdc, hFont);

    DrawText(hdc, buffer, -1, &rc, DT_CALCRECT);
    Rectangle(hdc, xpos, ypos, xpos + rc.right + 20,
        ypos + rc.bottom + 10);

    SetTextColor(hdc, RGB(0xff, 0xff, 0)); // yellow
    SetBkMode(hdc, TRANSPARENT);
    SetRect(&rc, xpos + 10, ypos + 5, rc.right + 10, rc.bottom + 10);
    DrawText(hdc, buffer, -1, &rc, DT_NOCLIP | DT_SINGLELINE);

    SelectObject(hdc, OldPen);
    SelectObject(hdc, OldBrush);
    SelectObject(hdc, OldFont);
    DeleteObject(hFont);
    ReleaseDC(0, hdc);
}
