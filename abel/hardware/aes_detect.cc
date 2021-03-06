// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//
// Created by liyinbin on 2020/3/4.
//

#include "abel/hardware/aes_detect.h"
#include <cstdint>
#include <cstring>
#include "abel/base/profile.h"

#if defined(ABEL_ARCH_X86_64)
#define ABEL_INTERNAL_USE_X86_CPUID
#elif defined(ABEL_ARCH_PPC) || defined(ABEL_ARCH_ARM) || \
    defined(ABEL_ARCH_AARCH64)
#if defined(__ANDROID__)
#define ABEL_INTERNAL_USE_ANDROID_GETAUXVAL
#define ABEL_INTERNAL_USE_GETAUXVAL
#elif defined(__linux__)
#define ABEL_INTERNAL_USE_LINUX_GETAUXVAL
#define ABEL_INTERNAL_USE_GETAUXVAL
#endif
#endif

#if defined(ABEL_INTERNAL_USE_X86_CPUID)
#if defined(_WIN32) || defined(_WIN64)
#include <intrin.h>  // NOLINT(build/include_order)
#pragma intrinsic(__cpuid)
#else

// MSVC-equivalent __cpuid intrinsic function.
static void __cpuid(int cpu_info[4], int info_type) {
    __asm__ volatile("cpuid \n\t"
    : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]),
    "=d"(cpu_info[3])
    : "a"(info_type), "c"(0));
}

#endif
#endif  // ABEL_INTERNAL_USE_X86_CPUID

// On linux, just use the c-library getauxval call.
#if defined(ABEL_INTERNAL_USE_LINUX_GETAUXVAL)

extern "C" unsigned long getauxval(unsigned long type);  // NOLINT(runtime/int)

static uint32_t get_auxval(uint32_t hwcap_type) {
  return static_cast<uint32_t>(getauxval(hwcap_type));
}

#endif

// On android, probe the system's C library for getauxval().
// This is the same technique used by the android NDK cpu features library
// as well as the google open-source cpu_features library.
//
// TODO(abel-team): Consider implementing a fallback of directly reading
// /proc/self/auxval.
#if defined(ABEL_INTERNAL_USE_ANDROID_GETAUXVAL)
#include <dlfcn.h>

static uint32_t get_auxval(uint32_t hwcap_type) {
  // NOLINTNEXTLINE(runtime/int)
  typedef unsigned long (*getauxval_func_t)(unsigned long);

  dlerror();  // Cleaning error state before calling dlopen.
  void* libc_handle = dlopen("libc.so", RTLD_NOW);
  if (!libc_handle) {
    return 0;
  }
  uint32_t result = 0;
  void* sym = dlsym(libc_handle, "getauxval");
  if (sym) {
    getauxval_func_t func;
    memcpy(&func, &sym, sizeof(func));
    result = static_cast<uint32_t>((*func)(hwcap_type));
  }
  dlclose(libc_handle);
  return result;
}
#endif

namespace abel {

// The default return at the end of the function might be unreachable depending
// on the configuration. Ignore that warning.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code-return"
#endif

// cpu_supports_randen_hw_aes returns whether the CPU is a microarchitecture
// which supports the crpyto/aes instructions or extensions necessary to use the
// accelerated randen_hw_aes implementation.
//
// 1. For x86 it is sufficient to use the CPUID instruction to detect whether
//    the cpu supports AES instructions. Done.
//
// Fon non-x86 it is much more complicated.
//
// 2. When ABEL_INTERNAL_USE_GETAUXVAL is defined, use getauxval() (either
//    the direct c-library version, or the android probing version which loads
//    libc), and read the hardware capability bits.
//    This is based on the technique used by boringssl uses to detect
//    cpu capabilities, and should allow us to enable crypto in the android
//    builds where it is supported.
//
// 3. Use the default for the compiler architecture.
//

bool is_supports_aes() {
#if defined(ABEL_INTERNAL_USE_X86_CPUID)
    // 1. For x86: Use CPUID to detect the required AES instruction set.
    int regs[4];
    __cpuid(reinterpret_cast<int *>(regs), 1);
    return regs[2] & (1 << 25);  // AES

#elif defined(ABEL_INTERNAL_USE_GETAUXVAL)
    // 2. Use getauxval() to read the hardware bits and determine
        // cpu capabilities.

#define AT_HWCAP 16
#define AT_HWCAP2 26
#if defined(ABEL_ARCH_PPC)
        // For Power / PPC: Expect that the cpu supports VCRYPTO
        // See https://members.openpowerfoundation.org/document/dl/576
        // VCRYPTO should be present in POWER8 >= 2.07.
        // Uses Linux kernel constants from arch/powerpc/include/uapi/asm/cputable.h
        static const uint32_t kVCRYPTO = 0x02000000;
        const uint32_t hwcap = get_auxval(AT_HWCAP2);
        return (hwcap & kVCRYPTO) != 0;

#elif defined(ABEL_ARCH_ARM)
        // For ARM: Require crypto+neon
        // http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0500f/CIHBIBBA.html
        // Uses Linux kernel constants from arch/arm64/include/asm/hwcap.h
        static const uint32_t kNEON = 1 << 12;
        uint32_t hwcap = get_auxval(AT_HWCAP);
        if ((hwcap & kNEON) == 0) {
          return false;
        }

        // And use it again to detect AES.
        static const uint32_t kAES = 1 << 0;
        const uint32_t hwcap2 = get_auxval(AT_HWCAP2);
        return (hwcap2 & kAES) != 0;

#elif defined(ABEL_ARCH_AARCH64)
        // For AARCH64: Require crypto+neon
        // http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0500f/CIHBIBBA.html
        static const uint32_t kNEON = 1 << 1;
        static const uint32_t kAES = 1 << 3;
        const uint32_t hwcap = get_auxval(AT_HWCAP);
        return ((hwcap & kNEON) != 0) && ((hwcap & kAES) != 0);
#endif

#else  // ABEL_INTERNAL_USE_GETAUXVAL
        // 3. By default, assume that the compiler default.
        return ABEL_HAVE_ACCELERATED_AES ? true : false;

#endif
    // NOTE: There are some other techniques that may be worth trying:
    //
    // * Use an environment variable: ABEL_RANDOM_USE_HWAES
    //
    // * Rely on compiler-generated target-based dispatch.
    // Using x86/gcc it might look something like this:
    //
    // int __attribute__((target("aes"))) HasAes() { return 1; }
    // int __attribute__((target("default"))) HasAes() { return 0; }
    //
    // This does not work on all architecture/compiler combinations.
    //
    // * On Linux consider reading /proc/cpuinfo and/or /proc/self/auxv.
    // These files have lines which are easy to parse; for ARM/AARCH64 it is quite
    // easy to find the Features: line and extract aes / neon. Likewise for
    // PPC.
    //
    // * Fork a process and test for SIGILL:
    //
    // * Many architectures have instructions to read the ISA. Unfortunately
    //   most of those require that the code is running in ring 0 /
    //   protected-mode.
    //
    //   There are several examples. e.g. Valgrind detects PPC ISA 2.07:
    //   https://github.com/lu-zero/valgrind/blob/master/none/tests/ppc64/test_isa_2_07_part1.c
    //
    //   MRS <Xt>, ID_AA64ISAR0_EL1 ; Read ID_AA64ISAR0_EL1 into Xt
    //
    //   uint64_t val;
    //   __asm __volatile("mrs %0, id_aa64isar0_el1" :"=&r" (val));
    //
    // * Use a CPUID-style heuristic database.
    //
    // * On Apple (__APPLE__), AES is available on Arm v8.
    //   https://stackoverflow.com/questions/45637888/how-to-determine-armv8-features-at-runtime-on-ios
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
}  // namespace abel
