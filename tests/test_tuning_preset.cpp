#include <catch2/catch_test_macros.hpp>
#include "core/tuning_preset.h"
#include "core/x265_handle.h"
#include "core/x265_router.h"

TEST_CASE("TuningPreset detects custom and standard tuning names", "[core][tune]") {
    REQUIRE(sk265::core::TuningPreset::isCustomTune("vcb-s"));
    REQUIRE(sk265::core::TuningPreset::isCustomTune("vcbs"));
    REQUIRE(sk265::core::TuningPreset::isCustomTune("vcb-s++"));
    REQUIRE(sk265::core::TuningPreset::isCustomTune("vcbs++"));
    REQUIRE(sk265::core::TuningPreset::isCustomTune("lp"));
    REQUIRE(sk265::core::TuningPreset::isCustomTune("littlepox"));
    REQUIRE(sk265::core::TuningPreset::isCustomTune("lp++"));
    REQUIRE(sk265::core::TuningPreset::isCustomTune("littlepox++"));

    REQUIRE_FALSE(sk265::core::TuningPreset::isCustomTune("grain"));
    REQUIRE_FALSE(sk265::core::TuningPreset::isCustomTune("animation"));
    REQUIRE_FALSE(sk265::core::TuningPreset::isCustomTune("film"));
    REQUIRE_FALSE(sk265::core::TuningPreset::isCustomTune(""));
}

TEST_CASE("TuningPreset applies vcb-s parameters accurately", "[core][tune]") {
    const x265_api* api = sk265::core::CoreRouter::getApi(8);
    REQUIRE(api != nullptr);

    auto param = sk265::core::make_param_handle(api);
    api->param_default_preset(param.raw(), "medium", nullptr);

    sk265::core::TuningPreset::apply(param.raw(), "vcb-s");

    REQUIRE(param->maxCUSize == 32);
    REQUIRE(param->maxTUSize == 32);
    REQUIRE(param->rc.qgSize == 8);
    REQUIRE(param->cbQpOffset == -2);
    REQUIRE(param->crQpOffset == -2);
    REQUIRE(param->bEnableSAO == 0);
    REQUIRE(param->bEnableStrongIntraSmoothing == 0);
    REQUIRE(param->deblockingFilterBetaOffset == -1);
    REQUIRE(param->deblockingFilterTCOffset == -1);
    REQUIRE(param->rc.rfConstant == 18.0);
    REQUIRE(param->psyRd == 1.8);
    REQUIRE(param->psyRdoq == 1.0);
}

TEST_CASE("TuningPreset applies lp (littlepox) parameters accurately", "[core][tune]") {
    const x265_api* api = sk265::core::CoreRouter::getApi(8);
    REQUIRE(api != nullptr);

    auto param = sk265::core::make_param_handle(api);
    api->param_default_preset(param.raw(), "medium", nullptr);

    sk265::core::TuningPreset::apply(param.raw(), "lp");

    REQUIRE(param->maxCUSize == 32);
    REQUIRE(param->maxTUSize == 32);
    REQUIRE(param->rc.qgSize == 8);
    REQUIRE(param->cbQpOffset == -2);
    REQUIRE(param->crQpOffset == -2);
    REQUIRE(param->bEnableSAO == 0);
    REQUIRE(param->rc.rfConstant == 20.0);
    REQUIRE(param->psyRd == 1.5);
    REQUIRE(param->psyRdoq == 0.8);
}
