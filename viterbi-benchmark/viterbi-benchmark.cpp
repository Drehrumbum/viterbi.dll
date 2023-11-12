/*
* This file is part of
*
* viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)
*
* 
* viterbi.dll test & benchmark
* 
* The program checks the replacement viterbi.dll on your computer and
* updates the "instruction-set" setting in "viterbi.txt" accordingly.
* It is best for the test if the CPU runs at a fixed core frequency. This can
* be set using the "Energy Options" or with the help of an external tool. It is
* not necessary to run the CPU at full speed. However, if the CPU is running
* "free" (this is more likely the case), we have to load it and wait a little
* until the CPU has hopefully reached its final speed. In an ideal computer
* world, the measured times should increase proportionally like in this example.
*
* Bitrate:    32  Framebits:   768  Time:   0.1000 sec
* Bitrate:    64  Framebits:  1536  Time:   0.2000 sec
* Bitrate:    96  Framebits:  2304  Time:   0.3000 sec
* Bitrate:   128  Framebits:  3072  Time:   0.4000 sec
*
* If the timings are looking "noisy", check for background-tasks and/or increase
* the number of warm-up frames via the commandline. Changing the number of loops
* for the final speed-test is normally not necessary.
* 
* /f  Number of frames for warm-up. Range 100 to 25000, default 5000 
* /t  Number of loops for speed-test. Range 100 to 500000, default 10000  
*
*
* 
* This program uses parts of the fec-3.0.1 package (c) 2006 by Phil Karn, KA9Q.
* Mr. Karn's software for Viterbi- and Reed-Solomon decoding can be found at
* http://www.ka9q.net/code/fec/ or at Github. It may be used under the terms of
* the GNU Lesser General Public License (LGPL).
*
* 2023 Heiko Vogel <hevog@gmx.de>
* 
*/



#undef UNICODE
#define _CRT_SECURE_NO_WARNINGS // We don't fly to the moon...

#include <windows.h>
#include <stdio.h>
#include <math.h>

#define MALLOC(a) _aligned_malloc(a, 64)
#define FREE(a) _aligned_free(a)
#define COMPUTETYPE unsigned int
#define K 7
#define RATE 4
#define MAXFRAMEBITS (24 * 384) // max bitrate 384 kBit/s for old DAB
#define BITLEN ((MAXFRAMEBITS + (K - 1)) / 8 + 1)
#define OFFSET (127.5)
#define CLIP 255
#define EBN0 (3.0)
#define GAIN (32.0)
#define	random() rand()
#define	srandom(time) srand(time)
#define POLYS { 109, 79, 83, 109 }


char szDllName[] = "viterbi.dll";
char szIniName[] = "viterbi.txt";
char szFnDeconvolve[] = "deconvolve";


typedef int (WINAPI* DECONVOLVE)(int frameBits, COMPUTETYPE* symbols,
    int unused, unsigned char* decodedBits);
typedef int (WINAPI* GETCPUCAPS)();
typedef int (WINAPI* INITIALIZE)();
typedef void (WINAPI* TOUCHSTACK)(int iLocalBytes);

INITIALIZE initialize;
DECONVOLVE deconvolve;
GETCPUCAPS GetCPUCaps;
TOUCHSTACK TouchStack;


// Bitmasks returned by GetCPUCaps
#define CpuHasSSE2  0b00000000
#define CpuHasSSE3  0b00000001
#define CpuHasSSSE3 0b00000010
#define CpuHasSSE41 0b00000100
#define CpuHasSSE42 0b00001000
#define CpuHasAVX1  0b00010000
#define CpuHasAVX2  0b00100000
#define CpuHasFMA3  0b01000000
// CpuHasAVX5 is only set if the CPU supports AVX512F+BW+VL!
#define CpuHasAVX5  0b10000000



int GetProcessorInfo();
int GetUserLocalAppDataFolder(char* szPath);
int SetIniFileValue(char* szFullIniFilePath, int val);
int GetIniFileValue(char* szFullIniFilePath);
int popcount32(unsigned int x);
int parityb(unsigned char x);
double normal_rand(double mean, double std_dev);
unsigned char addnoise(int sym, double amp, double gain,
    double offset, int clip);


const char* FuncTable[] = {

    "SSE2",
    "SSSE3",
    "AVX",
    "AVX2",
    "AVX512F"
};



int main(int argc, char* argv[]) {

    HMODULE hViterbi;
    COMPUTETYPE* inputInts;
    unsigned char *originalBits, *decodedBits;
    double dQpfPeriod, gain, esn0, execTime, maxPercent = 0.0;
    LARGE_INTEGER qpf, start, finish;
    long long compval, compare[16]{};
    int i, j, tr, bitrate, framebits, sr, errcnt, badframes,
        tot_errs, iFnNum, iCpuCaps, iPathStatus, iBest = 0;
    int polys[RATE] = POLYS;
    char szFullIniPath[260], szTmp[260];
    int iTestLoops = 10000;
    int iFrames = 5000;

    QueryPerformanceFrequency(&qpf);
    dQpfPeriod = 1.0 / qpf.QuadPart;


#if (1)
   
// Get the commandline if any
   for (i = 1; i < argc; i++) {
       if (argv[i][0] == '/' || argv[i][0] == '-') {
           switch (argv[i][1]) {

           case 't':
           case 'T':
               j = abs(atoi(argv[i] + 3));
               if (j >= 100 && j <= 500000)
                   iTestLoops = j;
               break;
           case 'f':
           case 'F':
               j = abs(atoi(argv[i] + 3));
               if (j >= 100 && j <= 25000)
                   iFrames = j;
               break;
           default:
               break;
           }
       }
   }

// Wait a little bit until the console-window appeared on screen.
   Sleep(500); 
   printf("viterbi - test & benchmark\n");

// Load the viterbi.dll and get the exported proc-addresses.
// If this is the first run of the DLL, it creates its config-file
// in LOCALAPPDATA during this call and selects the highest possible
// instruction-set.
    hViterbi = LoadLibrary(szDllName);
    if (!hViterbi) {
        printf("Could't load %s!\n", szDllName);
        goto NoViterbi;
    }

    initialize = (INITIALIZE)GetProcAddress(hViterbi, "initialize");
    if (!initialize) {
        printf("Entrypoint 'initialize' not found.\n");
        goto NoProc;
    }
    
    deconvolve = (DECONVOLVE)GetProcAddress(hViterbi, szFnDeconvolve);
    if (!deconvolve) {
        printf("Entrypoint '%s' not found.\n", szFnDeconvolve);
        goto NoProc;
    }

    GetCPUCaps = (GETCPUCAPS)GetProcAddress(hViterbi, "GetCPUCaps");
    if (!GetCPUCaps) {
        printf("Entrypoint 'GetCPUCaps' not found. Wrong DLL?\n");
        goto NoProc;
    }

    TouchStack = (TOUCHSTACK)GetProcAddress(hViterbi, "TouchStack");
    if (!TouchStack) {
        printf("Entrypoint 'TouchStack' not found. Wrong DLL?\n");
        goto NoProc;
    }

// Show the CPU-Info
    iCpuCaps = GetProcessorInfo();

// Search the (newly created) config-file
    iPathStatus = GetUserLocalAppDataFolder(szFullIniPath);

    if (2 == iPathStatus) {
        sprintf(szTmp, "%s\\%s", szFullIniPath, szIniName);
        strcpy(szFullIniPath, szTmp);
        int oldIniValue = GetIniFileValue(szFullIniPath);

        if (oldIniValue != -2) {
            iPathStatus = 1; // file exists
        }
    }
    else
        iPathStatus = 0; // something went wrong

    if (!iPathStatus) { // This is unlikely
        printf("\n\nThe %s configuration file was not found!\n"
            "The DLL uses automatic CPU dispatching until this file "
            "can be created on disk!\n\n", szDllName);
        goto NoProc;
    }
 
// Get the memory for the arrays
    inputInts = (COMPUTETYPE*)MALLOC(RATE * 
        (MAXFRAMEBITS + (K - 1)) * sizeof(COMPUTETYPE));

    if (!inputInts) {
        printf("Allocation of inputInts array failed\n");
        goto NoProc;
    }

    originalBits = (unsigned char*)MALLOC(BITLEN);
    if (!originalBits) {
        printf("Allocation of originalBits array failed\n");
        FREE(inputInts);
        goto NoProc;
    }

    decodedBits = (unsigned char*)MALLOC(BITLEN);
    if (!decodedBits) {
        printf("Allocation of decodedBits array failed\n\n");
        FREE(originalBits);
        FREE(inputInts);
        goto NoProc;
    }

// We only have one thread in this test program and it calls "deconvolve",
// so we must prepare the program's stack once.
    TouchStack(0x14000); // 20 pages

// Boost
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

// Set the config-file to SSE2...
    SetIniFileValue(szFullIniPath, 0);
    initialize(); // ...and update the CPU-dispatcher inside of the DLL
    printf("\nTesting function \"%s\" (%s) from %s with %d calls...\n",
        szFnDeconvolve, FuncTable[0], szDllName, iTestLoops);

    esn0 = EBN0 + 10 * log10(1. / ((double)RATE));
    gain = 1. / sqrt(0.5 / pow(10., esn0 / 10.));

    bitrate = 128;
    framebits = bitrate * 24;
    tot_errs = 0;
    badframes = 0;
    sr = 0;
    srandom(0);

    for (tr = 0; tr < iFrames; tr++) {
        for (i = 0; i < framebits + (K - 1); i++) {
            int bit = (i < framebits) ? (random() & 1) : 0;
            sr = (sr << 1) | bit;
            originalBits[i / 8] = sr & 0xff;
            for (j = 0; j < RATE; j++)
                inputInts[RATE * i + j] = addnoise(parityb(sr & polys[j]), gain,
                    GAIN, OFFSET, CLIP);
        }

        deconvolve(framebits, inputInts, 0, decodedBits);
        errcnt = 0;

        for (int i = 0; i < framebits / 8; i++) {
            int e = popcount32(decodedBits[i] ^ originalBits[i]);
            errcnt += e;
            tot_errs += e;
        }

        if (errcnt != 0)
            badframes++;
    }

    printf("BER %d/%d (%10.3g) FER %d/%d (%10.3g)\n",
        tot_errs, framebits * (tr), tot_errs / ((double)framebits * (tr)),
        badframes, tr, (double)badframes / (tr));


    for (bitrate = 32, j = 0; bitrate <= 128; bitrate += 32, j++) {
        framebits = bitrate * 24;
        printf("Bitrate: %5d\tFramebits: %5d\t", bitrate, framebits);

        for (tr = 0; tr < iTestLoops / 2; tr++) // warm up
            deconvolve(framebits, inputInts, 0, decodedBits);

        QueryPerformanceCounter(&start);
        for (tr = 0; tr < iTestLoops; tr++)
            deconvolve(framebits, inputInts, 0, decodedBits);
        QueryPerformanceCounter(&finish);

        compare[j] = finish.QuadPart - start.QuadPart;
        execTime = compare[j] * dQpfPeriod;
        printf("Time: %8.4f sec\n", execTime);

    }

#endif
     
    printf("\n-------------------------------------------------------------------------------\n");

// Now check the other versions from SSSE3 to AVX512F

#if (1)
    for (iFnNum = 1; iFnNum < 5; iFnNum++) {
        if (((iFnNum == 1) && !(iCpuCaps & CpuHasSSSE3)) ||
            ((iFnNum == 2) && !(iCpuCaps & CpuHasAVX1)) ||
            ((iFnNum == 3) && !(iCpuCaps & CpuHasAVX2)) ||
            ((iFnNum == 4) && !(iCpuCaps & CpuHasAVX5)))
            continue; // skip unsupported instruction sets

        SetIniFileValue(szFullIniPath, iFnNum);
        initialize();
        printf("\nTesting function \"%s\" (%s) from %s with %d calls...\n",
            szFnDeconvolve, FuncTable[iFnNum], szDllName, iTestLoops);

        bitrate = 128;
        framebits = bitrate * 24;
        tot_errs = 0;
        badframes = 0;
        sr = 0;
        srandom(0);

        for (tr = 0; tr < iFrames; tr++) {
            for (i = 0; i < framebits + (K - 1); i++) {
                int bit = (i < framebits) ? (random() & 1) : 0;
                sr = (sr << 1) | bit;
                originalBits[i / 8] = sr & 0xff;
                for (j = 0; j < RATE; j++)
                    inputInts[RATE * i + j] = addnoise(parityb(sr & polys[j]),
                        gain, GAIN, OFFSET, CLIP);
            }

            deconvolve(framebits, inputInts, 0, decodedBits);
            errcnt = 0;

            for (int i = 0; i < framebits / 8; i++) {
                int e = popcount32(decodedBits[i] ^ originalBits[i]);
                errcnt += e;
                tot_errs += e;
            }

            if (errcnt != 0)
                badframes++;
        }

        printf("BER %d/%d (%10.3g) FER %d/%d (%10.3g)\n",
            tot_errs, framebits * (tr), tot_errs / ((double)framebits * (tr)),
            badframes, tr, (double)badframes / (tr));

        for (bitrate = 32, j = 0; bitrate <= 128; bitrate += 32, j++) {
            framebits = bitrate * 24;
            printf("Bitrate: %5d\tFramebits: %5d\t", bitrate, framebits);

            for (tr = 0; tr < iTestLoops / 2; tr++) // warm up
                deconvolve(framebits, inputInts, 0, decodedBits);

            QueryPerformanceCounter(&start);
            for (tr = 0; tr < iTestLoops; tr++)
                deconvolve(framebits, inputInts, 0, decodedBits);
            QueryPerformanceCounter(&finish);

            compval = finish.QuadPart - start.QuadPart;
            execTime = compval * dQpfPeriod;
            printf("Time: %8.4f sec", execTime);
            if (compval < compare[j]) {
                execTime = compare[j] / (double)compval;
                printf("\t%6.3f x FASTER\n", execTime);

                if (execTime > maxPercent) {
                    maxPercent = execTime;
                    iBest = iFnNum;
                }
            }
            else
                printf("\n");
        }
    }

// Updating the config-file
        printf("\n\nUpdating \"%s\" to \"%s\".\n",
            szFullIniPath, FuncTable[iBest]);
        SetIniFileValue(szFullIniPath, iBest);
#endif


    FREE(originalBits);
    FREE(decodedBits);
    FREE(inputInts);
NoProc:
    FreeLibrary(hViterbi);
NoViterbi:
    fprintf(stderr,"\nProgram finished!\n");
    fprintf(stderr, "Rename the \"%s\" inside of the QIRX "
        "program folder (usually \"C:\\Program Files\\softsyst\\...\")\n"
	    "and then copy the replacement DLL into it.\n\nPress 'enter'.", szDllName);
    getchar();
    return 0;
}


int GetProcessorInfo() {
    char szTmp[256], szTmp2[256];
    DWORD dwSize = 256;
    int retVal = GetCPUCaps(); 
 
    *szTmp = '\0';
    *szTmp2 = '\0';

    GetEnvironmentVariable("PROCESSOR_IDENTIFIER", szTmp, 256);
    strcat(szTmp, "\nCPU Level: ");
    GetEnvironmentVariable("PROCESSOR_LEVEL", szTmp2, 256);
    strcat(szTmp, szTmp2);
    strcat(szTmp, " / Revision: ");
    GetEnvironmentVariable("PROCESSOR_REVISION", szTmp2, 256);
    strcat(szTmp, szTmp2);
    printf("\n%s\n", szTmp);

    LSTATUS reg = RegGetValue(HKEY_LOCAL_MACHINE,
        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 
        "ProcessorNameString", RRF_RT_REG_SZ, NULL, szTmp, &dwSize);
    if (ERROR_SUCCESS == reg)
        printf("%s\n", szTmp);

    strcpy(szTmp, "Instruction sets: SSE2");

    if (retVal & CpuHasSSE3)  strcat(szTmp, ", SSE3");
    if (retVal & CpuHasSSSE3) strcat(szTmp, ", SSSE3");
    if (retVal & CpuHasSSE41) strcat(szTmp, ", SSE4.1");
    if (retVal & CpuHasSSE42) strcat(szTmp, ", SSE4.2");
    if (retVal & CpuHasAVX1)  strcat(szTmp, ", AVX");
    if (retVal & CpuHasAVX2)  strcat(szTmp, ", AVX2");
    if (retVal & CpuHasFMA3)  strcat(szTmp, ", FMA3");
    if (retVal & CpuHasAVX5)  strcat(szTmp, ", AVX512F+BW+VL");
    
    printf("%s\n", szTmp);

    return retVal;
}

// GetUserLocalAppDataFolder return values:
// 0 : env-variable not set, path too long or a file "viterbi" exists
//     in the "...\AppData\Local\" directory for unknown reasons
// 1 : the path "...\AppData\Local\" exists
// 2 : the path "...\AppData\Local\viterbi\" exists
// Taken from the viterbi.dll sources
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
    strcpy(szLocalAppDataPath, tempbuf);
    iPathStatus = 1;

// Append our own directory-name to the path and search for it
    sprintf(tempbuf, "%s\\%s", szLocalAppDataPath, "viterbi");
    hSearch = FindFirstFile(tempbuf, &wfd);

    if (INVALID_HANDLE_VALUE != hSearch) {
        // We have a match, but this can be a file. This is unlikely, but
        // who knows? Let's check it.

        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // It's a directory...
            strcpy(szLocalAppDataPath, tempbuf); // copy the path
            iPathStatus = 2; // all is fine
            SetFileAttributes(tempbuf, FILE_ATTRIBUTE_NORMAL);
        }
        FindClose(hSearch);
    }
    return iPathStatus;
}

// Set the "instruction set override" value (first byte in viterbi.txt)
int SetIniFileValue(char* szFullIniFilePath, int val) {
    char fib = '0';
    int retVal = 0;
    if (val > 4) return -1;

    FILE* fini = fopen(szFullIniFilePath, "rb+");
    if (fini) {

        if (val >= 0)
            fib += char(val);
        else
            fib = 'x';

        fwrite(&fib, 1, 1, fini);
        fclose(fini);
        retVal = 1;
    }
    return retVal;
}

// Get the instruction set value from viterbi.txt.
// Returns -2 if the file was not found
int GetIniFileValue(char* szFullIniFilePath) {
    char fib = 0;
    int retVal = -2; // reval if file not found
    FILE* fini = fopen(szFullIniFilePath, "rb+");
    if (fini) {

        fread(&fib, 1, 1, fini);
        fclose(fini);
        if (fib >= '0' && fib < '5')
            retVal = fib - '0';   // 0 to 4
        else
            retVal = -1; // automatic
    }
    return retVal;
}

// popcount32 algo taken from 
// https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
int popcount32(unsigned int v) {
    v = v - ((v >> 1) & 0x55555555); // reuse input as temporary
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333); // temp
    return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
}

// Determine parity of argument: 1 = odd, 0 = even
static inline int parityb(unsigned char x)
{
    __asm__ __volatile__("test %1,%1;setpo %0" : "=g"(x) : "r" (x));
    return x;
}

// Generate gaussian random double with specified mean and std_dev
static inline double normal_rand(double mean, double std_dev)
{
    double fac, rsq, v1, v2;
    static double gset;
    static int iset;

    if (iset)
    {
        iset = 0;
        return mean + std_dev * gset;
    }

    do
    {
        v1 = 2.0 * (double)random() / RAND_MAX - 1;
        v2 = 2.0 * (double)random() / RAND_MAX - 1;
        rsq = v1 * v1 + v2 * v2;
    } while (rsq >= 1.0 || rsq == 0.0);
    fac = sqrt(-2.0 * log(rsq) / rsq);
    gset = v1 * fac;
    iset++;
    return mean + std_dev * v2 * fac;
}

static inline unsigned char addnoise(int sym, double amp, double gain,
    double offset, int clip)
{
    int sample;

    sample = offset + gain * normal_rand(sym ? amp : -amp, 1.0);

    if (sample < 0)
        sample = 0;
    else if (sample > clip)
        sample = clip;
    return sample;
}
