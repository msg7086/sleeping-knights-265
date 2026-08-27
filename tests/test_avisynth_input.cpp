#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include "pipeline/input/avisynth_input.h"

TEST_CASE("AviSynthInput handles non-existent file gracefully without crashing", "[input]") {
    sk265::pipeline::input::AviSynthInput avsInput;
    REQUIRE_FALSE(avsInput.open("non_existent_script_12345.avs"));
    REQUIRE(avsInput.isEof());
    REQUIRE_FALSE(avsInput.readFrame().has_value());
}

TEST_CASE("AviSynthInput handles empty path", "[input]") {
    sk265::pipeline::input::AviSynthInput avsInput;
    REQUIRE_FALSE(avsInput.open(""));
    REQUIRE(avsInput.isEof());
}

TEST_CASE("AviSynthInput supports custom library path and seek offset", "[input]") {
    std::string avsPath = "fixtures/test_video.avs";
    if (!std::filesystem::exists(avsPath)) {
        avsPath = "tests/fixtures/test_video.avs";
    }

    if (std::filesystem::exists(avsPath)) {
        sk265::pipeline::input::AviSynthInput avsInput;
        avsInput.setSeekFrame(5);
        if (avsInput.open(avsPath)) {
            auto frame = avsInput.readFrame();
            REQUIRE(frame.has_value());
            REQUIRE(frame->pts == 5);
        }

        sk265::pipeline::input::AviSynthInput badLibInput;
        badLibInput.setCustomLibraryPath("non_existent_avs_dll.dll");
        REQUIRE_FALSE(badLibInput.open(avsPath));
    }
}

TEST_CASE("AviSynthInput handles non-ASCII / Unicode script paths", "[input]") {
    std::string avsPath = "fixtures/test_video.avs";
    if (!std::filesystem::exists(avsPath)) {
        avsPath = "tests/fixtures/test_video.avs";
    }

    if (std::filesystem::exists(avsPath)) {
        auto u8Path = std::filesystem::path(avsPath).parent_path() / "测试中文_unicode.avs";
        std::filesystem::copy_file(avsPath, u8Path, std::filesystem::copy_options::overwrite_existing);

        sk265::pipeline::input::AviSynthInput avsInput;
        bool ok = avsInput.open(u8Path.string());
        REQUIRE(ok);
        if (ok) {
            auto frame = avsInput.readFrame();
            REQUIRE(frame.has_value());
        }

        std::filesystem::remove(u8Path);
    }
}

TEST_CASE("AviSynthInput reads correct planar Y, U, V colors without plane mixing", "[input][avisynth][color]") {
    std::string testScript = "tests_color_verify.avs";
    {
        std::ofstream ofs(testScript);
        // Create 64x64 clip with distinct Y, U, V components: Y=235 (0xEB), U=32 (0x20), V=208 (0xD0)
        ofs << "BlankClip(length=5, width=64, height=64, fps=25, pixel_type=\"YV12\", color_yuv=$EB20D0)\n";
    }

    sk265::pipeline::input::AviSynthInput avsInput;
    if (avsInput.open(testScript)) {
        auto frameOpt = avsInput.readFrame();
        REQUIRE(frameOpt.has_value());
        const auto& frame = *frameOpt;

        REQUIRE(frame.planes[0].size() >= 64 * 64);
        REQUIRE(frame.planes[1].size() >= 32 * 32);
        REQUIRE(frame.planes[2].size() >= 32 * 32);

        // Plane 0 (Y) must be 235 (0xEB)
        CHECK(frame.planes[0][0] == 0xEB);
        // Plane 1 (U/Cb) must be 32 (0x20) - NOT corrupted by Y (235)
        CHECK(frame.planes[1][0] == 0x20);
        // Plane 2 (V/Cr) must be 208 (0xD0) - NOT corrupted by U (32) or Y (235)
        CHECK(frame.planes[2][0] == 0xD0);

        // Mutual plane distinctness check
        CHECK(frame.planes[0][0] != frame.planes[1][0]);
        CHECK(frame.planes[1][0] != frame.planes[2][0]);
    }
    std::filesystem::remove(testScript);
}
