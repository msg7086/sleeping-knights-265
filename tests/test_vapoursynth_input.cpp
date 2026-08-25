#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include "pipeline/input/vapoursynth_input.h"

TEST_CASE("VapourSynthInput handles non-existent file and empty path gracefully", "[input][vpy]") {
    sk265::pipeline::input::VapourSynthInput input;
    REQUIRE_FALSE(input.open(""));
    REQUIRE(input.isEof());

    REQUIRE_FALSE(input.open("non_existent_script_123.vpy"));
    REQUIRE(input.isEof());
    REQUIRE_FALSE(input.readFrame().has_value());
}

TEST_CASE("VapourSynthInput handles custom bad library path gracefully", "[input][vpy]") {
    sk265::pipeline::input::VapourSynthInput input;
    input.setCustomLibraryPath("non_existent_vsscript.dll");
    REQUIRE_FALSE(input.open("dummy.vpy"));
}

TEST_CASE("VapourSynthInput loads real fixture via FFMS2", "[input][vpy]") {
    std::string vpyPath = "fixtures/test_video_ffms2.vpy";
    if (!std::filesystem::exists(vpyPath)) {
        vpyPath = "tests/fixtures/test_video_ffms2.vpy";
    }

    if (std::filesystem::exists(vpyPath)) {
        sk265::pipeline::input::VapourSynthInput input;
        bool ok = input.open(vpyPath);
        if (ok) {
            auto info = input.getInfo();
            REQUIRE(info.width == 1280);
            REQUIRE(info.height == 720);
            REQUIRE(info.bitDepth == 8);
            REQUIRE(info.colorSpace == 1);

            auto frame = input.readFrame();
            REQUIRE(frame.has_value());
            REQUIRE(frame->width == 1280);
            REQUIRE(frame->height == 720);
            REQUIRE(frame->planes[0].size() > 0);
        }
    }
}
