#include "utils/cpu_features.h"

#if defined(__x86_64__) || defined(_M_X64)
#if defined(_MSC_VER)
#include <intrin.h>
#include <immintrin.h>
#else
#include <cpuid.h>
#include <immintrin.h>
#include <x86intrin.h>
#endif
#endif

namespace sk265::utils {

#if defined(__x86_64__) || defined(_M_X64)

static inline void run_cpuid(uint32_t eax, uint32_t ecx, uint32_t info[4]) noexcept {
#if defined(_MSC_VER)
    __cpuidex(reinterpret_cast<int*>(info), static_cast<int>(eax), static_cast<int>(ecx));
#else
    __cpuid_count(eax, ecx, info[0], info[1], info[2], info[3]);
#endif
}

static inline uint64_t run_xgetbv(uint32_t ecx) noexcept {
#if defined(_MSC_VER)
    return _xgetbv(ecx);
#elif defined(__GNUC__) || defined(__clang__)
    uint32_t eax, edx;
    __asm__ volatile(".byte 0x0f, 0x01, 0xd0" : "=a"(eax), "=d"(edx) : "c"(ecx));
    return (static_cast<uint64_t>(edx) << 32) | eax;
#else
    return 0;
#endif
}

#endif

CpuFeatures CpuFeatures::detect() noexcept {
    CpuFeatures feat;

#if defined(__x86_64__) || defined(_M_X64)
    uint32_t info[4] = {0};

    // 1. Max basic leaf check
    run_cpuid(0, 0, info);
    uint32_t maxBasicLeaf = info[0];
    if (maxBasicLeaf < 7) {
        return feat;
    }

    // 2. OSXSAVE check (CPUID.1:ECX[bit 27])
    run_cpuid(1, 0, info);
    bool hasOsXsave = (info[2] & (1U << 27)) != 0;
    if (!hasOsXsave) {
        return feat;
    }

    // 3. XCR0 register check for AVX-512 state:
    //    bit 1: SSE, bit 2: AVX, bit 5: opmask, bit 6: ZMM_Hi256, bit 7: Hi16_ZMM
    //    Total mask = 0xE6
    uint64_t xcr0 = run_xgetbv(0);
    if ((xcr0 & 0xE6ULL) != 0xE6ULL) {
        return feat;
    }
    feat.osXsaveEnabled = true;

    // 4. AVX-512 base suite check (CPUID.(EAX=7, ECX=0):EBX)
    //    F(bit 16), DQ(bit 17), CD(bit 28), BW(bit 30), VL(bit 31)
    //    Mask = (1<<16) | (1<<17) | (1<<28) | (1<<30) | (1<<31) = 0xD0030000
    run_cpuid(7, 0, info);
    constexpr uint32_t avx512BaseMask = (1U << 16) | (1U << 17) | (1U << 28) | (1U << 30) | (1U << 31);
    if ((info[1] & avx512BaseMask) == avx512BaseMask) {
        feat.hasAvx512Base = true;
    }

    // 5. AVX512_BF16 check (CPUID.(EAX=7, ECX=1):EAX[bit 5])
    run_cpuid(7, 1, info);
    if ((info[0] & (1U << 5)) != 0) {
        feat.hasAvx512Bf16 = true;
    }
#endif

    return feat;
}

} // namespace sk265::utils
