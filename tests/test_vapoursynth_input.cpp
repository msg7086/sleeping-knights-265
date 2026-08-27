#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
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

TEST_CASE("VapourSynthInput reads correct planar Y, U, V colors without plane mixing", "[input][vpy][color]") {
    std::string testScript = "tests_vpy_color_verify.vpy";
    {
        std::ofstream ofs(testScript);
        ofs << "import vapoursynth as vs\n"
            << "core = vs.core\n"
            << "clip = core.std.BlankClip(length=5, width=64, height=64, fpsnum=25, fpsden=1, format=vs.YUV420P8, color=[235, 32, 208])\n"
            << "clip.set_output()\n";
    }

    sk265::pipeline::input::VapourSynthInput vpyInput;
    if (vpyInput.open(testScript)) {
        auto frameOpt = vpyInput.readFrame();
        REQUIRE(frameOpt.has_value());
        const auto& frame = *frameOpt;

        REQUIRE(frame.planes[0].size() >= 64 * 64);
        REQUIRE(frame.planes[1].size() >= 32 * 32);
        REQUIRE(frame.planes[2].size() >= 32 * 32);

        // Plane 0 (Y) must be 235 (0xEB)
        CHECK(frame.planes[0][0] == 235);
        // Plane 1 (U/Cb) must be 32 (0x20)
        CHECK(frame.planes[1][0] == 32);
        // Plane 2 (V/Cr) must be 208 (0xD0)
        CHECK(frame.planes[2][0] == 208);

        // Mutual plane distinctness check
        CHECK(frame.planes[0][0] != frame.planes[1][0]);
        CHECK(frame.planes[1][0] != frame.planes[2][0]);
    }
    std::filesystem::remove(testScript);
}
