#include <catch2/catch_test_macros.hpp>
#include <filesystem>
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
