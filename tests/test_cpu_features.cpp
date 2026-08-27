#include <catch2/catch_test_macros.hpp>
#include "utils/cpu_features.h"

TEST_CASE("CpuFeatures detect executes safely without throwing or crashing", "[cpu]") {
    auto features = sk265::utils::CpuFeatures::detect();

    // Verify boolean consistency
    if (features.shouldDefaultEnableAvx512()) {
        REQUIRE(features.hasAvx512Base);
        REQUIRE(features.hasAvx512Bf16);
        REQUIRE(features.osXsaveEnabled);
    }
}

TEST_CASE("CpuFeatures shouldDefaultEnableAvx512 evaluation logic", "[cpu]") {
    sk265::utils::CpuFeatures feat;
    feat.hasAvx512Base = false;
    feat.hasAvx512Bf16 = false;
    feat.osXsaveEnabled = false;
    REQUIRE_FALSE(feat.shouldDefaultEnableAvx512());

    feat.hasAvx512Base = true;
    feat.hasAvx512Bf16 = false;
    feat.osXsaveEnabled = true;
    REQUIRE_FALSE(feat.shouldDefaultEnableAvx512());

    feat.hasAvx512Base = true;
    feat.hasAvx512Bf16 = true;
    feat.osXsaveEnabled = false;
    REQUIRE_FALSE(feat.shouldDefaultEnableAvx512());

    feat.hasAvx512Base = true;
    feat.hasAvx512Bf16 = true;
    feat.osXsaveEnabled = true;
    REQUIRE(feat.shouldDefaultEnableAvx512());
}
