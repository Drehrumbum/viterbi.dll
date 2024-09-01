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
* exc_handler.cpp
* 
* The vectored exception handler (VEH) is called every time an exception is
* thrown anywhere in the entire process. Beside of the logging-stuff (if 
* enabled), it keeps an eye on possible STACK_OVERFLOW and ACCESS_VIOLATION
* exceptions thrown in 'deconvolve' or 'RScheckSuperframe'. In the event of the
* first exception thrown inside of the DLL, the exception-handler looks for a
* way back to QIRX and 'short circuits' the function 'deconvolve'. This means,
* it replaces the current decoder (let's say AVX) with a 'secure' one, so all
* subsequent calls to the function return immediately to QIRX. This is
* sufficient, even if the exception was triggered in 'RScheckSuperframe', 
* because this function will not called again, until QIRX has (collected and)
* decoded five new frames with the help of 'deconvolve'. 
* 
* Remember that it is almost impossible for these exceptions to ever occur
* during operation. The handler here is therefore some kind of a 'Proof of 
* Concept' thing.
* 
* (c) 2024 Heiko Vogel <hevog@gmx.de>
*
*/

#include "viterbi.h"

#if defined(VIT_INCLUDE_EXCEPTIONS) && defined(VIT_WRITE_LOGFILE)
extern char* double2str(double x, int WidthPrec, char* buffer);
extern HANDLE hVitLogFile;
extern void ScreenWriteXY(int xpos, int ypos, char* buffer);
#endif


// The pointer to a specific decoder version when 'deconvolve' is called. The
// target depends on the supported instruction-sets of the CPU. 
// See setupdll.cpp.
extern DECON* deconJumpTarget;

// The address of a secure decoder in viterbi_helpers.asm. This is just a piece
// of reused code, which returns an error-code to QIRX.
extern "C" DECON decon_savemode;



// If it crashes inside __chkstk() due to low stack space and the return
// address on the stack points into the DLL, we know one of our deconvolve
// functions was to blame. At this point, only a few non-volatile GPRs have
// been pushed onto the stack and we need to pop them all off the stack before
// we can return to QIRX. To do this, we need the address of the first 
// instruction immediately after the
// 
//     add rsp, imm32
// 
// instruction at the end of a deconvolve function. The rax register contains
// the imm32 value, so we can create the full opcode, which is seven bytes in
// size, on the fly. This opcode MUST exist in memory, a few hundred bytes away
// from the return address found on the stack. And this is the fixed part of the
// opcode:
const unsigned int c_AddRspOpcode = 0xC48148;


// As long as we don't change the code in chainback.inc we'll find the 
// following code-sequence at the end of every assembly deconvolve function:
// 
// mov byte ptr[r9 + rdx], cl
// jb  @b
// xor eax, eax
//
// We need the address of the instruction right after 'xor eax, eax' and
// looking for these eight bytes in memory.

const DWORD64 c_DeconRetAssembly = 0xC033'D272'110C8841;
//___________________________________|xor|_jb_|__mov...



// If the solution-config "Rel_cpp" is used, the compiler currently generates
// the following code-sequence at the end of every deconvolve function:
//
// dec rax
// test r8, r8
// jne @b
// xor eax, eax 
//
// We need the address of the instruction right after xor eax, eax.
// Of course, the generated code may change after compiler updates (e.g. other 
// register-allocation stragety), so it is important to watch out for this and
// to adjust this constant below accordingly. 

const DWORD64 c_DeconRetCompiler = 0xC031'C775'C0854D'C8;
//___________________________________|xor|jne_|_test_|_last_byte_from 'dec rax'



// If it crashes in 'RScheckSuperframe' we are looking for the following code 
// sequence. It's unlikely that this code changes after compiler updates, but
// better watch out...
//
// mov eax, -1
// lea rsp, [rbp + imm32]
//
const DWORD64 c_RetRSCheckCode  = 0xA58D48'FFFFFFFFB8;
//__________________________________|_lea_|_mov eax,-1



// This messagebox should never appear on the screen...
const char szStackOverflow[] =
"STACK OVERFLOW";
const char szAccessViolation[] =
"ACCESS VIOLATION";
const char szEndPart[] =
" detected! The DLL goes into save mode to prevent QIRX from crashing. "
"DAB decoding will not work until you restart the receiver. If this "
"Messagebox still pops up, we have a bigger problem.";

void ShowErrMsg(int msgSel) {
    char szMess[0x100];

    if (msgSel) lstrcpyn(szMess, szAccessViolation, 0x100);
    else lstrcpyn(szMess, szStackOverflow, 0x100);

    lstrcat(szMess, szEndPart);
    MessageBox(0, szMess, szDllName, MB_SETFOREGROUND | MB_ICONSTOP);
}



LONG WINAPI VecExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {

    DWORD64 callerAdd, address, opcode, mem;
    PCONTEXT context;
    LONG retVal = EXCEPTION_CONTINUE_SEARCH;
    DWORD exCode = pExceptionInfo->ExceptionRecord->ExceptionCode;
    context = pExceptionInfo->ContextRecord;

#if defined (VIT_WRITE_LOGFILE) && defined(VIT_INCLUDE_EXCEPTIONS)
    pVDM->exceptCounter++;
    char szBuffer[256];
    DWORD numBytesWritten;
    FILETIME ftEntry;
    ULARGE_INTEGER entryTime;
    SYSTEMTIME sysTime;
    if (INVALID_HANDLE_VALUE != hVitLogFile) {
        GetSystemTimePreciseAsFileTime(&ftEntry);
        FileTimeToSystemTime(&ftEntry, &sysTime);
        // calculate microsecs of current second
        entryTime.HighPart = ftEntry.dwHighDateTime;
        entryTime.LowPart = ftEntry.dwLowDateTime;
        entryTime.QuadPart = (entryTime.QuadPart % 10000000uLL) / 10uLL;
        wsprintf(szBuffer, "EXCEPT  %02u:%02u:%02u.%06u  No: %5i        TID: %5u  "
            "Exception code: 0x%08X at offset 0x%p\n",
            sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
            entryTime.LowPart,
            pVDM->exceptCounter,
            GetCurrentThreadId(),
            exCode,
            pExceptionInfo->ExceptionRecord->ExceptionAddress);
        WriteFile(hVitLogFile, szBuffer, lstrlen(szBuffer),
            &numBytesWritten, NULL);
        
        if (EXCEPTION_ACCESS_VIOLATION == exCode) {
            szBuffer[lstrlen(szBuffer) - 1] = 0; // cut the "\n"
            ScreenWriteXY(200, 5, szBuffer);
        }
    }
#endif

   
    switch (exCode) {

    case EXCEPTION_ACCESS_VIOLATION:
// check if the exception belongs to the DLL...
        if (context->Rip >= pVDM->dllBaseAddress && context->Rip < pVDM->dllEndAddress) {
// find the exit
            for (address = context->Rip; address < pVDM->dllEndAddress; address++) {
                mem = *(DWORD64*)address;


                if ((c_DeconRetCompiler == mem) || (c_DeconRetAssembly == mem)) {

                    context->Rip = address + 8;
                    context->Rax = 1; // return error-value
                    break;
                }

                if (c_RetRSCheckCode == mem) {
                    context->Rip = address;
                    break;
                }
            }

            deconJumpTarget = decon_savemode;
            ShowErrMsg(1);
            retVal = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    
    case EXCEPTION_STACK_OVERFLOW:
// When such an exception occurs, it is usually thrown within a call to 
// __chkstk() exported by a runtime library. We are looking for the return
// address to the caller on the crashed threads stack. __chkstk() preserves r10
// and r11 before it walks down the stack, so the return address to the caller
// is located at rsp + 16 on its stack. If this address points to the DLLs code
// we have a 'hit'. There is no need to restore the touched GUARD PAGE, because
// the current thread will never call __chkstk() again.

        callerAdd = *(DWORD64*)(context->Rsp + 16);

        if (callerAdd >= pVDM->dllBaseAddress && callerAdd < pVDM->dllEndAddress) {
            opcode = c_AddRspOpcode + (context->Rax << 24);
            address = callerAdd + 0x200; // skip some code

            for (; address < pVDM->dllEndAddress; address++) {
                mem = (*(DWORD64*)address) & 0x00'FFFFFF'FFFFFFFF;
                if (opcode == mem)
                    break;
            }

            context->Rip = address + 7;
            context->Rsp += 24; // "return" from __chkstk() 
            deconJumpTarget = decon_savemode;
            ShowErrMsg(0);
            retVal = EXCEPTION_CONTINUE_EXECUTION;            
        }
        break;
    }
    return retVal;
}