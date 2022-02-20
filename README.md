# viterbi.dll
viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)

“Need for speed” 

Preface

viterbi.dll is one of the workhorses of QIRX and is responsible for Viterbi- and Reed-Solomon decoding/ error correction. The two exported functions will be called lots of times per second. They should execute as fast as possible.

As a big fan of the well known QIRX-SDR (https://qirx.softsyst.com), I’ve checked the sources of it’s “viterbi.dll” during the last months, stripped out unnecessary parts and made some optimizations to get the whole thing running faster. It’s still “under development”, because I’m not really satisfied at all. Especially the function “RSCheckSuperframe” needs more attention in the future. This function is not so time-critical like the viterbi-decoder (the function “deconvolve”), but should also run as fast as possible. “Need for speed”, so to speak. However, I think the time is right to share the current state and the sources with you. 



What’s up?
 
At the moment and by using my old laptop (Intel Core i5 3340M “Ivy Bridge”), the viterbi-decoder runs round about 25 times faster than the code inside of the viterbi.dll bundled with QIRX, when SSE2-code is used. By using the AVX instruction-set, the whole thing runs 30 times faster. Compared with my latest version (from the end of October 2021, uses SSE2, was directly available from my dropbox account in the past), the current SSE2-version runs 25% faster, the AVX-version reaches +42%.

This gain of speed is just the naked benchmark, only related to the function “deconvolve”, of course. It doesn't mean that QIRX now runs faster with the new DLL (hey, hey, hey…), but you will see the difference, if you take a look at the “Symbol Time” on the QIRX main-window. This value depends on the CPU-core frequency, the bit-rate of the selected service and other things, but should always stay below 0.8 ms (green). By using the replacement DLL, this value will be at least 20% shorter than before. To give you an example, QIRX version 2.1.19 shows a relaxed “Symbol Time” of around 0.5 ms while decoding a 112 kBit - service at my laptop with the CPU throttled and fixed to 1.2 GHz and using the SSE2-Version of viterbi.dll. Doing the same with the original DLL results in 0.7 ms or even more.

As you can see, it’s worth to replace the original viterbi.dll. But because of licensing-reasons, this can’t be done automatically within new releases of QIRX. This is no real drawback, as long as you don’t forget to replace the installed viterbi.dll inside of the QIRX program folder (usually “C:\Program Files\softsyst\...”) with one of the releases from here. One advantage: You can choose a version, which supports the capabilities of your CPU best. Inside of the zip-file given here, you can find three DLL’s compiled for the SSE2, AVX and AVX512 instructions-sets. If you don’t know which one is the best for you, try one of the CPU identification-tools out there, or simply ask the task-manager for the name of your CPU and search for reviews or the technical data. If you are still in doubt which one to use, use the SSE2-version, please. Useless to say, that you’ll need administrator rights to get write-access to the program-folder.




DIY

If you want to compile the DLL for Windows 64-Bit by yourself, you can simply use the project-file, suitable for “Microsoft Visual Studio 2019”. But before you do this, please make sure, that at least the “LLVM” platform-toolset is available in Visual Studio, too. If not installed yet, you can easily do this by using the Visual Studio Installer (Menu Extras > Tools & Features…). Another option is the C++ compiler from Intel, of course. This compiler is part of the “oneAPI Base Toolkit”, which you can get from Intel’s website. The setup-program detects the presence of Visual Studio and after the installation (take some longer coffee-breaks), the compiler will be available in the general-project-settings of Visual Studio. 

You may ask, why I recommend you to install other compilers? The answer is short: “Need for speed” is the motto of this project and the MSVC compiler isn’t the best choice for that approach. Sorry, that’s the truth. The clang-compiler is one of the best compilers around. It’s real amazing to see what a today’s optimizing compiler does. You can use other compilers not mentioned herein, of course. But always look inside of the assembly-language listings! This is important. Don’t blame me, if your code runs slower as expected.

When all of the preparation work is done, you’re ready to go. Load the project into Visual Studio and simply recompile it. There is absolutely need to change the settings. Do not link against any C-Library, please. We don’t need it. The compiler should use it’s own inbuilt solutions for memset, memcpy or similar, instead. We only need some memory directly from the system and want kick up QIRX a little bit. Kernel32.dll is our best friend and always there.

To all penguins out there: I think, you'll get the needed stuff running w/o further information. 



License / Copyrights

Original scene-play (c) 2001++ by Phil Karn, KA9Q (http://www.ka9q.net/) Mr. Karn's software for Viterbi- and Reed-Solomon decoding can be found at http://www.ka9q.net/code/fec/.

Original DLL-version for QIRX (c) 2018 by Clem Schmidt (https://qirx.softsyst.com). The sources of this DLL can be found at Clem's website within the download-section, more likely inside of a Linux-package.

This DLL uses the 16-Way-SSE2 intrinsic code from https://spiral.net for viterbi decoding, (c) 2005-2008, Carnegie Mellon University distributed under the GNU General Public License (GPL).

