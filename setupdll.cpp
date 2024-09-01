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
* (c) 2021-2024 Heiko Vogel <hevog@gmx.de>
*
*/



#include "viterbi.h"


// extern decoder-functions in deconvolve.cpp or in the assembly-parts.
extern "C" DECON decon_sse2_lut32, decon_ssse3, decon_avx,
           decon_avx2, decon_avx5;

// setting the default decoder-function
DECON* deconJumpTarget = decon_sse2_lut32;

extern "C" WAKEUPYMM xorYMM;
extern WAKEUPYMM* wakeUpYMMJumpTarget;

// external CPU-check defines
#include "getcpucaps.h"


int  GetUserLocalAppDataBasePath(char* szLocalAppDataBasePath);
int  GetSubFolder(char* szInoutBasePath, const char* szSubFolder);
int  CreateSubFolder(char* szInoutBasePath, const char* szNewFolder);
void CreateIniFile();
int  GetIniFile(DispatcherCMD* dcmd);
void SetupCpuDispatcher(DispatcherCMD* dcmd);
void ScreenWriteXY(int xpos, int ypos, char* buffer);


void SetupDLL() {
    char temp[MAX_PATH];
    int status;

    pVDM->dcmd.ShowIRect = 1; // show the Info-Rectangle on screen
    pVDM->dcmd.InstruSet = 5; // invalid value, triggers automatic dispatching

    if (!pVDM->haveAppDataBasePath) {
        pVDM->haveAppDataBasePath =
            GetUserLocalAppDataBasePath(pVDM->szLocalAppDataBasePath);
        pVDM->cpuCaps = GetCPUCaps();
    }

    if (pVDM->haveAppDataBasePath) {

        if (!pVDM->haveVitConfig) {
            wsprintf(pVDM->szVitFullConfigFileName, "%s\\%s\\%s%s",
                pVDM->szLocalAppDataBasePath, szDllName, szDllName, szExtTXT);
            pVDM->haveVitConfig = 1;
        }

        if (!GetIniFile(&pVDM->dcmd)) {
            lstrcpyn(temp, pVDM->szLocalAppDataBasePath, MAX_PATH);
            status = GetSubFolder(temp, szDllName);
            if (!status)
                CreateSubFolder(temp, szDllName);
            CreateIniFile();
        }
        SetupCpuDispatcher(&pVDM->dcmd);
    }
}

__forceinline int GetUserLocalAppDataBasePath(char* szLocalAppDataBasePath) {
    char tempbuf[MAX_PATH];
    int PathLen, ret = 1; // assume success

    // If GetEnvironmentVariable() fails or if the current path is too long
    // for appending our directory and the filename ("\viterbi\viterbi.txt")
    // later, we'll better fail at this point.
    PathLen = GetEnvironmentVariable("LOCALAPPDATA", tempbuf, MAX_PATH - 22);

    if (!PathLen || PathLen >= MAX_PATH - 22) {
        *szLocalAppDataBasePath = 0;
        ret = 0;
    }
    else
        lstrcpyn(szLocalAppDataBasePath, tempbuf, MAX_PATH);

    return ret;
}

__forceinline int CheckPathExists(char* path) {
    int ret = 0;

    if (path[0] == 0)
        return ret;

    DWORD att = GetFileAttributes(path);

    if (INVALID_FILE_ATTRIBUTES != att) {
        if (att & FILE_ATTRIBUTE_DIRECTORY) {
            ret++;
        }
    }
    return ret;
}

__forceinline int GetSubFolder(char* szInoutPath, const char* szSubFolder) {
    char buff[MAX_PATH];
    int ret = 0;

    wsprintf(buff, "%s\\%s", szInoutPath, szSubFolder);
    if (CheckPathExists(buff)) {
        lstrcpyn(szInoutPath, buff, MAX_PATH); // return the existing path
        ret++;
    }
    return ret;
}

__forceinline int CreateSubFolder(char* szInoutPath, const char* szNewFolder) {
    int ret = 0;
    char buff[MAX_PATH];
    wsprintf(buff, "%s\\%s", szInoutPath, szNewFolder);

    if (CreateDirectory(buff, NULL)) {
        lstrcpyn(szInoutPath, buff, MAX_PATH);
        ret++;
    }
    return ret;
}

// Try to create the config-file. Normally this shouldn't
// go wrong. If so, there is nothing we can do at the moment
// and the automatic comes into play again (file not found -> auto).
#include "inifiletext.h"
__forceinline void CreateIniFile() {
    DWORD dwNumBytesWritten;
    HANDLE hIni = CreateFile(pVDM->szVitFullConfigFileName, GENERIC_WRITE,
        0, 0, CREATE_ALWAYS, 0, 0);

    if (INVALID_HANDLE_VALUE != hIni) {
        WriteFile(hIni, szIniFileText, lstrlen(szIniFileText),
            &dwNumBytesWritten, NULL);
        CloseHandle(hIni);
    }
}

__forceinline int ReadConfigFile(void* buffer, int bytesToRead) {
    int ret = 0;
    HANDLE hIn;
    DWORD dwNumBytesRead;

    hIn = CreateFile(pVDM->szVitFullConfigFileName, GENERIC_READ,
        0, 0, OPEN_EXISTING, 0, 0);

    if (INVALID_HANDLE_VALUE != hIn) {
        ReadFile(hIn, buffer, bytesToRead, &dwNumBytesRead, 0);
        CloseHandle(hIn);
        if (dwNumBytesRead == bytesToRead)
            ret++;
    }
    return ret;
}

// Try to read the first 3 bytes of "viterbi.txt" and update "dcmd",
// if possible. We'll get something like this: "x:x"
__forceinline int GetIniFile(DispatcherCMD* dcmd) {
    unsigned char chFileInput[8]{};
    int res = ReadConfigFile(chFileInput, 3);

    if (res){
        if (chFileInput[0] >= '0' && chFileInput[0] < '5')
            dcmd->InstruSet = chFileInput[0] - '0';     // 0 to 4
        if (chFileInput[2] == '0') dcmd->ShowIRect = 0; // 0 or 1
    }
    return res;
}

__forceinline void SetupCpuDispatcher(DispatcherCMD* dcmd) {
    char szMessageText[64];
    int iCPUCaps;

    wsprintf(szMessageText, "%s%s: ", szDllName, szExtDLL);

    iCPUCaps = pVDM->cpuCaps;

// Set the highest possible instruction set (automatic)
// iSelect will always be in the range from 0 to 4.
    int iSelect = 0; // assume SSE2

    if (iCPUCaps & CpuHasSSSE3) iSelect = 1;

    if (iCPUCaps & CpuHasAVX1) {
        iSelect = 2;
        wakeUpYMMJumpTarget = xorYMM; // enable YMM wake-up
    }
    if (iCPUCaps & CpuHasAVX2)  iSelect = 3;
    if (iCPUCaps & CpuHasAVX5)  iSelect = 4;

// Now try to set the user-requested instruction set.
// Selecting a *lower* (supported) instruction-set is allowed,
// of course.

    switch (dcmd->InstruSet) {
    case 0:
        iSelect = 0;
        break;
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
    }

// Finally we select the jump-target according
// to the value of iSelect.
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
        lstrcat(szMessageText, szSSE2);
        deconJumpTarget = decon_sse2_lut32;
        break;
    }

    if (dcmd->ShowIRect)
        ScreenWriteXY(5, 5, szMessageText);
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
