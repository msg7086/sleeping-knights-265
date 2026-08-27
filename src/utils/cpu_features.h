#pragma once
#include <cstdint>

namespace sk265::utils {

struct CpuFeatures {
    bool hasAvx512Base{false}; // AVX512F, DQ, CD, BW, VL
    bool hasAvx512Bf16{false}; // AVX512_BF16 (CPUID.(EAX=7, ECX=1):EAX[bit 5])
    bool osXsaveEnabled{false}; // OS XSAVE enabled and XCR0 allows ZMM

    [[nodiscard]] constexpr bool shouldDefaultEnableAvx512() const noexcept {
        return hasAvx512Base && hasAvx512Bf16 && osXsaveEnabled;
    }

    static CpuFeatures detect() noexcept;
};

} // namespace sk265::utils
