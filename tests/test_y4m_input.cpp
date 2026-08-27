#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include "pipeline/input/y4m_input.h"

TEST_CASE("Y4mInput parses 8-bit standard Y4M streams and frames", "[input]") {
    // 4x4 YUV420 8-bit frame: Y=16, U=4, V=4 bytes (total 24 bytes)
    std::string header8 = "YUV4MPEG2 W4 H4 F30:1 Ip A1:1 C420\nFRAME\n";
    std::string payload8(24, static_cast<char>(128));
    std::istringstream stream8(header8 + payload8);

    sk265::pipeline::input::Y4mInput input8;
    REQUIRE(input8.openFromStream(stream8));
    REQUIRE(input8.getInfo().width == 4);
    REQUIRE(input8.getInfo().height == 4);
    REQUIRE(input8.getInfo().bitDepth == 8);
    REQUIRE(input8.getInfo().fpsNum == 30);
    REQUIRE(input8.getInfo().fpsDen == 1);

    auto frame8 = input8.readFrame();
    REQUIRE(frame8.has_value());
    REQUIRE(frame8->width == 4);
    REQUIRE(frame8->height == 4);
    REQUIRE(frame8->bitDepth == 8);
    REQUIRE(frame8->planes[0].size() == 16);
    REQUIRE(frame8->planes[1].size() == 4);
    REQUIRE(frame8->planes[2].size() == 4);

    // End of stream
    auto eofFrame = input8.readFrame();
    REQUIRE_FALSE(eofFrame.has_value());
    REQUIRE(input8.isEof());
}

TEST_CASE("Y4mInput parses 10-bit Y4M streams", "[input]") {
    // 4x4 YUV420 10-bit frame (2 bytes per sample): Y=32, U=8, V=8 bytes (total 48 bytes)
    std::string header10 = "YUV4MPEG2 W4 H4 F24:1 Ip A1:1 C420p10\nFRAME\n";
    std::string payload10(48, static_cast<char>(200));
    std::istringstream stream10(header10 + payload10);

    sk265::pipeline::input::Y4mInput input10;
    REQUIRE(input10.openFromStream(stream10));
    REQUIRE(input10.getInfo().width == 4);
    REQUIRE(input10.getInfo().height == 4);
    REQUIRE(input10.getInfo().bitDepth == 10);
    REQUIRE(input10.getInfo().fpsNum == 24);
    REQUIRE(input10.getInfo().fpsDen == 1);

    auto frame10 = input10.readFrame();
    REQUIRE(frame10.has_value());
    REQUIRE(frame10->bitDepth == 10);
    REQUIRE(frame10->planes[0].size() == 32);
    REQUIRE(frame10->planes[1].size() == 8);
    REQUIRE(frame10->planes[2].size() == 8);
}

TEST_CASE("Y4mInput preserves distinct planar Y, U, V color values", "[input][y4m][color]") {
    // 4x4 YUV420: Y=16 bytes of 0xEB, U=4 bytes of 0x20, V=4 bytes of 0xD0
    std::string header = "YUV4MPEG2 W4 H4 F25:1 Ip A1:1 C420\nFRAME\n";
    std::string yData(16, static_cast<char>(0xEB));
    std::string uData(4, static_cast<char>(0x20));
    std::string vData(4, static_cast<char>(0xD0));
    std::istringstream stream(header + yData + uData + vData);

    sk265::pipeline::input::Y4mInput input;
    REQUIRE(input.openFromStream(stream));

    auto frameOpt = input.readFrame();
    REQUIRE(frameOpt.has_value());
    const auto& frame = *frameOpt;

    CHECK(frame.planes[0][0] == 0xEB);
    CHECK(frame.planes[1][0] == 0x20);
    CHECK(frame.planes[2][0] == 0xD0);

    CHECK(frame.planes[0][0] != frame.planes[1][0]);
    CHECK(frame.planes[1][0] != frame.planes[2][0]);
}
