#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include "pipeline/input/lavf_input.h"

TEST_CASE("LavfInput handles non-existent file and empty path gracefully", "[input][lavf]") {
    sk265::pipeline::input::LavfInput input;
    REQUIRE_FALSE(input.open(""));
    REQUIRE(input.isEof());

    REQUIRE_FALSE(input.open("non_existent_file_xyz_123.mp4"));
    REQUIRE(input.isEof());
    REQUIRE_FALSE(input.readFrame().has_value());
}

TEST_CASE("LavfInput opens real video file directly and reads frames", "[input][lavf]") {
    std::string videoPath = "fixtures/14946398.mp4";
    if (!std::filesystem::exists(videoPath)) {
        videoPath = "tests/fixtures/14946398.mp4";
    }

    if (std::filesystem::exists(videoPath)) {
        sk265::pipeline::input::LavfInput input;
        bool ok = input.open(videoPath);
        REQUIRE(ok);
        if (ok) {
            auto info = input.getInfo();
            REQUIRE(info.width == 1280);
            REQUIRE(info.height == 720);
            REQUIRE(info.bitDepth == 8);
            REQUIRE(info.colorSpace == 1); // I420

            auto frame = input.readFrame();
            REQUIRE(frame.has_value());
            REQUIRE(frame->width == 1280);
            REQUIRE(frame->height == 720);
            REQUIRE(frame->planes[0].size() > 0);
        }
    }
}
