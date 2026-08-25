#include <catch2/catch_test_macros.hpp>
#include "core/x265_handle.h"
#include "core/x265_router.h"

TEST_CASE("CoreRouter routes valid bit depths symmetrically", "[core]") {
    REQUIRE(sk265::core::CoreRouter::getApi(8) != nullptr);
    REQUIRE(sk265::core::CoreRouter::getApi(10) != nullptr);
    REQUIRE(sk265::core::CoreRouter::getApi(12) != nullptr);
    REQUIRE(sk265::core::CoreRouter::getApi(14) == nullptr);
    REQUIRE(sk265::core::CoreRouter::getApi(0) == nullptr);
}

TEST_CASE("X265ParamHandle and PictureHandle manage lifecycle via RAII", "[core]") {
    const x265_api* api = sk265::core::CoreRouter::getApi(8);
    REQUIRE(api != nullptr);

    auto param = sk265::core::make_param_handle(api);
    REQUIRE(param != nullptr);
    REQUIRE(param.api == api);
    REQUIRE(param.raw() != nullptr);

    auto pic = sk265::core::make_picture_handle(api);
    REQUIRE(pic != nullptr);
    REQUIRE(pic.api == api);
    REQUIRE(pic.raw() != nullptr);
}
