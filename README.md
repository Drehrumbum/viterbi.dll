# viterbi.dll
viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)

*“Need for speed” is not a game. ;)*

## Overview
The viterbi.dll is one of the workhorses of the QIRX-SDR (https://qirx.softsyst.com) and is responsible for Viterbi- and Reed-Solomon decoding/ error correction. The two exported functions will be called lots of times per second and should run as fast as possible. As a big fan of QIRX I've checked the sources of its "viterbi.dll", stripped out unnecessary parts and made some optimizations to get the whole thing running faster. In the past, the replacement DLL used the intrinsics-code from the SPIRAL-project for the viterbi-decoder. That is no longer the case. Instead, the code of the 16-way decoder from Phil Karn's example sources was used for further development. This alone has increased the speed of the decoder by 15% and reduced the BER/FER error rates too. The reason for this improvement is the "pavgb" instruction that Mr. Karn uses in his examples. This instruction can't be used for all kinds of viterbi-decoders, so it is not used by the code-generator from SPIRAL. Many thanks to "old-dab" (https://github.com/old-dab) who found this small but very important detail inside of the 16-way-decoder sources some time ago!

## What's new?
## September 2024:

### Calling __chkstk()  
The commonly used prologue code for the assembly-language parts of “deconvolve” has been changed so that the code there always calls __chkstk(). The previously used compiler flag /G has been removed so that the same applies to the compiler-generated decoders (if the “Rel_cpp” configuration is used). Calling __chkstk() on each decoder call takes little more time, but is safer and leaves the stacks of all other threads created during runtime alone.

### Vectored Exception Handler
Primarily intended for testing and logging purposes, a so called vectored exception-handler was added to "see" all exceptions within the entire process. You can enable this logging-feature in viterbi.h when the DLL is compiled for writing a log-file. But since the handler is already there, it handles possible exceptions within "deconvolve" or "RSCheckSuperframe" and returns safely to QIRX. A Messagebox pops up if this happens. However, it's almost impossible that you ever see such a Messagebox while running QIRX. The test-program will ask you at the end of its speed-tests on your machine, if you want to check the exception-handler. 

### Update of SSE2 decoder
Discovered by chance and occurring only under very rare (!) circumstances, the symbols passed by QIRX to the viterbi-decoder were not in the range 0 to 255. This caused a crash when using the viterbi-decoder for SSE2 CPU’s. This has been fixed here and will be fixed in upcoming versions of QIRX (likely v. 4.2.4), too. The viterbi-decoders for all other supported instructions-sets were not affected.


### Merging segments, alignment, speedup
The code section of the assembly-language parts has been merged with the code section of the compiler generated code. The starting points of the main loops of these decoders are now aligned at the beginning of a new cache line. The AVX2 version of the decoder runs 4-5% faster than before.


## November 2023
### "All in one" solution
The DLL contains viterbi-decoders for the SSE2, SSSE3, AVX, AVX2 and AVX512 instruction-sets. A CPU dispatcher checks the capabilities of the CPU at startup and selects the decoder with the highest instruction-set, so you normally don't need to think about it. However, you can manually override this automatic or simply run the small test-program. It checks the speed of all decoders supported by your CPU and stores the fastest decoder inside of the DLL's configuration-file. 


### 256-bit (32-way) viterbi-decoders for AVX2 and AVX512
For CPU's that support at least the AVX2 instruction-set a 32-way viterbi-decoder is now included. The number of instructions inside of such a decoder's main loop is significantly smaller than inside of the 128-bit (16-way) decoders, which of course leads to a much higher decoding speed. However, the viterbi-decoder is only one link in the whole DAB decoding chain and the impact of its speed on the overall performance of QIRX is no longer as serious as in the past. But, and that’s the most important part, the decoder helps keep the CPU's 256-bit stages running for 256-bit code that could later be executed in QIRX. And if the 256-bit stages of the CPU are already running when "deconvolve" is called, the decoder also benefits from this. In short, the more 256-bit SIMD code running in QIRX, the better the overall performance. For this reason I recommend using the replacement DLL from here.

### Improved 128-bit (16-way) versions of the viterbi-decoder
One of the challenges in optimizing the viterbi-decoder for CPU's without AVX2 is the lack of a native instruction that broadcasts one byte (a 'symbol') from memory to all bytes of an XMM-register. There are some ways to do that (see the sources), but after many tests and benchmarks only one method remained for SSE2 in the final release. This method uses a lookup table (LUT) in which the 256 possible symbols are stored as "ready to go" values. The disadvantages of this method are that the LUT should always stay in the L1-cache and of course the necessary additional memory accesses. The decoder for CPU's with SSSE3 support plays in the same league and the AVX decoder can have an additional boost, likely on older CPU's from Intel (like Ivy-Bridge). However, the measured speeds of the 128-bit decoders can vary from one CPU to another, due to different cache-sizes, memory-bandwidth etc.

Note: If you use older versions of QIRX (like v3.xx), I strongly recommend that you replace the viterbi.dll used there. This also applies to the last free version 3.22, which you can get from Clem's website and if you downloaded this package before January 2024. The installation package before January 2024 contains a viterbi.dll with the inefficient code from 2018.

      
### Improved function "RSCheckSuperframe"
The old lookup-table for modulo-values was replaced with an efficient Mod255()-function which fits our needs. Further optimizations have led to a remarkable increase in speed.


## Download / Setup
Download the zip-file which name ends with "...viterbi_final.zip" under "Releases" on the right and unpack it into a directory of your choice. In this directory you will then find the viterbi.dll, the test-program and a batch file in which you can change the command line options for the test-program if necessary. At this point, you can simply run the test-program to check the speed of the viterbi-decoders inside of the DLL on your computer. This takes a few seconds and in the end the fastest decoder version is written to the configuration file and will be used by QIRX. Next, rename the original viterbi.dll inside the QIRX program folder (usually located at C:\Program Files\softsyst\qirx...) and then copy the replacement DLL into this directory. You need write permission for this directory.

## DIY
If you want to compile the DLL and the test-program for Windows 64-Bit by yourself, download the zip-file which name ends with "...viterbi_project.zip" under "Releases" on the right and unpack it into a directory of your choice. It contains the complete solution, suitable for "Microsoft Visual Studio 2022". But before you start, please make sure, that the "LLVM" platform-toolset (includes the clang-compiler) is available in Visual Studio. If not installed yet, you can easily do this by using the Visual Studio Installer (Menu Extras > Tools & Features…). This compiler is generally recommended. The MSVC compiler will not work w/o changing the sources. When all of the preparation work is done, you're ready to go.

Double-click the "viterbi.sln" file and select "Rel_asm" in the "Solution Configuration" at the top of the main window and build the whole solution. The DLL and the test program can then be found in the "final" directory.

With the setting "Rel_asm" some "handcrafted" assembly-language parts will be used. This "step back to the roots" was necessary after some surprises with the compiler-generated code after updates of Visual Studio in the past. Although the source files and project settings were not changed, the code produced by the compiler changed - and runs slower. Needless to say that I don't like such surprises. So I selected the "best shots" from some older compiler listings files to "freeze" the code. To avoid installing other Assemblers in Visual Studio, the included MASM (ml64.exe) is used for the assembly-parts. And yes, sometimes it's easier to change the code directly in assembly language than to convince the compiler to do or not do certain things.

With the "Rel_cpp" setting, the assembly-parts will be disabled and the C-intrinsics inside of the file "deconvolve.cpp" will be compiled instead. With the 128-bit decoders, the compiler is under high register-pressure because all constants and metrics should be kept in the registers, but only 16 SIMD registers are available. For example, pay attention to whether the compiler generates code to load one or more constants from memory within the main loop. This is most likely not what you want, even if the constants are loaded from the L1 cache again and again. To get more speed play with the macros, crack them, interchange the intrinsics a little bit, use dummy-variables and so on.

It is not necessary to change the project settings. The DLL does not need a C-library because it is primarily intended for computing. Only a few functions from the Win32-API are needed, so the /NODEFAULTLIB flag for linking is used. Starting with QIRX version 4.0, the program uses multiple threads calling the decoder, so it is necessary to provide the "decisions" - array for each calling thread individually. The easiest way to do this is to use their stacks. The program's threads do not use their own stacks for larger amounts of dynamically allocated variables, meaning that their stack pointers do not change (significantly) during their lifetime. This applies to all versions of QIRX released so far.

In viterbi.h you can enable VIT_WRITE_LOGFILE and then recompile the DLL. When the DLL is then called by QIRX all calls are logged. But the content of such a file is only interesting for Clem (or if you're curious). With the define VIT_WRITE_SYMBOLS the symbols for the viterbi-decoder are also written to another file. If you evaluate such a file (with the Hexeditor “HxD”, for example) later, you can see the statistical distribution of the symbols. But that's just for the curious.

## License / Copyrights
Original scene-play (c) 2001++ by Phil Karn, KA9Q. Mr. Karn's software for Viterbi- and Reed-Solomon decoding can be found at http://www.ka9q.net/code/fec/ or at GitHub.

Original DLL-version for QIRX (c) 2018 by Clem Schmidt (https://qirx.softsyst.com). The sources of this DLL can be found at Clem's website within the download-section, more likely inside of a Linux-package.


## Thanks to
- Agner Fog for his optimizing guides. https://www.agner.org/optimize/
- stackoverflow.com for lots of information
- uops.info for their online "Code Analyzer" tool
- onlinegdb.com for their online IDE
- the guys at mikrocontroller.net for testing
