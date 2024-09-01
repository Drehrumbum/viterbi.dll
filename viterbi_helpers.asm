comment!#######################################################################
#                           This file is part of                              #
#           'viterbi.dll replacement for QIRX-SDR (Windows 64 Bit)'           #
#                                                                             #
#  viterbi_helpers.asm                                                        #
#                                                                             #
#  A little collection of low-level functions used in viterbi.dll.            #
#                                                                             #
#  (c) 2022-24 Heiko Vogel <hevog@gmx.de>                                     #
##############################################################################!

.nolist
include sehmac.inc
.list


comment!#######################################################################
#                                                                             #
#  GetCPUCaps checks the supported instruction-sets of the CPU and if the OS  #
#  handles them. The function assumes to run under Windows 7/64-Bit and above #
#  so we always have at least SSE2-support.                                   #
#                                                                             #
#  extern "C" int GetCPUCaps();                                               #
#                                                                             #
#  Returnvalue:                                                               #
#  The function returns the supported instruction-sets as defined below.      #
#  Include "getcpucaps.h" for the C-defines.                                  #
#                                                                             #
##############################################################################!

SSE2  equ 00000000y
SSE3  equ 00000001y
SSSE3 equ 00000010y
SSE41 equ 00000100y
SSE42 equ 00001000y
AVX1  equ 00010000y
AVX2  equ 00100000y
FMA3  equ 01000000y
AVX5  equ 10000000y ; bit only set if CPU supports AVX512F+BW+VL!



ShadowSpace = 0 ; no calls to other functions 
LocalSpace  = 0 ; no stack-space

.code
align 8
GetCPUCaps proc frame 
    SaveRegs rbx

    Retval     equ SaveInt0
    CpuHasAVX1 equ SaveInt1
    CpuHasAVX2 equ SaveInt2
    CpuHasAVX5 equ SaveInt3
    CpuHasFMA3 equ SaveInt4

    xor     eax, eax
    mov     Save_RCX, rax
    mov     Save_RDX, rax
    mov     Save__R8, rax
    
; get standard cpu features with eax = 1
    inc     eax
    cpuid
    mov     r8d, ecx        ; save ecx for later
    
    test    ecx, 1          ; SSE3
    jz      @f
    inc     Retval          ; set bit 0
@@:
    bt      ecx, 9          ; SSSE3
    jnc     @f
    add     Retval, SSSE3
@@:
    bt      ecx, 12         ; FMA3
    jnc     @f
    inc     CpuHasFMA3      ; this means nothing at the moment
@@:
    bt      ecx, 19         ; SSE4.1
    jnc     @f 
    add     Retval, SSE41
@@:
    bt      ecx, 20         ; SSE4.2
    jnc     @f
    add     Retval, SSE42
@@:
    bt      ecx, 28         ; AVX
    jnc     @f
    inc     CpuHasAVX1      ; this means nothing at the moment

@@:
; get cpu features with eax = 7 and ecx = 0
    mov     eax, 7
    xor     ecx, ecx
    cpuid

    bt      ebx, 5          ; AVX2
    jnc     @f
    inc     CpuHasAVX2      ; this means nothing at the moment

; For the AVX512 version of "deconvolve" the compiler needs additional
; AVX512BW & AVX512VL support from the CPU.
@@:
    bt      ebx, 16         ; AVX512F
    jnc     @f
    inc     CpuHasAVX5      ; this means nothing at the moment

    bt      ebx, 30         ; AVX512BW
    jnc     @f
    inc     CpuHasAVX5      ; this means nothing at the moment

    bt      ebx, 31         ; AVX512VL
    jnc     @f
    inc     CpuHasAVX5      ; this means nothing at the moment

@@:
; Check if the OS has set the bits for AVX and AVX512 support.

    bt      r8d, 27         ; check if the OS has set the OSXSAVE bit
    jnc     fini            ; and it's allowed to use "xgetbv".

    xor     ecx, ecx
    xgetbv

; check XMM state and YMM state for AVX
    mov     ecx, eax
    and     ecx, 6
    cmp     ecx, 6          ; test bits [2:1]
    jne     fini            ; not supported by OS (pre Windows7 SP1), jump out

    test    CpuHasAVX1, 1
    jz      @f
    add     Retval, AVX1    ; AVX1 okay, set the bit 
@@:
    test    CpuHasAVX2, 1
    jz      @f
    add     Retval, AVX2    ; AVX2 okay, set the bit
@@:
    test    CpuHasFMA3, 1
    jz      @f
    add     Retval, FMA3    ; FMA3 okay, set the bit

@@:
; check OPMASK state, upper 256-bit of ZMM0-ZMM15 and ZMM16-ZMM31 state
    and     eax, 0E0h
    cmp     eax, 0E0h       ; test bits [7:5]
    jne     fini            ; not supported by OS (pre Windows10), jump out

    cmp     CpuHasAVX5, 3
    jne     fini
    add     Retval, AVX5    ; AVX512F+BW+VL okay, set the bit

fini:
    mov     eax, Retval
    RestoreRegs
    ret
GetCPUCaps endp


comment!#######################################################################
#                                                                             #
#  Two labels used by "WakeUpYMM()" for powering-up the 256-bit-stages in     #
#  advance.                                                                   #
#                                                                             #
#  void WakeUpYMM();                                                          #
#                                                                             #
#  The function is declared as void, so the return value doesn't care in this #
#  case.                                                                      #
#                                                                             #
#  The label decon_savemode is used by the exception-handler as a new jump    #
#  target for the cpu-dispatcher of 'deconvolve' if it handles a exception in #
#  'deconvolve'. The return value tells QIRX that something went wrong. Well, #
#  it's almost impossible that you ever see such things during operation.     #
#                                                                             #
##############################################################################!

public xorYMM, noYMM, decon_savemode
align 8
xorYMM:
    vxorps   ymm0, ymm0, ymm0
    vzeroupper
    nop
noYMM:
decon_savemode:
    mov eax, 1
    ret

end