#include <catch2/catch_test_macros.hpp>
#include "utils/log_format.h"

TEST_CASE("LogFormat formatFrameRange handles various frame configurations", "[log]") {
    using sk265::utils::LogFormat;

    SECTION("Known total frames, no seek, all frames") {
        std::string s = LogFormat::formatFrameRange(257339, 0, 0);
        REQUIRE(s == "frames 0 - 257338 of 257339");
    }

    SECTION("Known total frames, with seek and count") {
        std::string s = LogFormat::formatFrameRange(1000, 100, 200);
        REQUIRE(s == "frames 100 - 299 of 1000");
    }

    SECTION("Known total frames, count exceeds total") {
        std::string s = LogFormat::formatFrameRange(500, 400, 200);
        REQUIRE(s == "frames 400 - 499 of 500");
    }

    SECTION("Unknown total frames, no limit specified") {
        std::string s = LogFormat::formatFrameRange(0, 0, 0);
        REQUIRE(s == "unknown frame count");
    }

    SECTION("Unknown total frames, explicit count specified") {
        std::string s = LogFormat::formatFrameRange(0, 50, 300);
        REQUIRE(s == "frames 50 - 349");
    }
}

TEST_CASE("LogFormat formatInputBanner generates standard mod input log", "[log]") {
    using sk265::utils::LogFormat;

    sk265::pipeline::input::InputInfo info;
    info.width = 1920;
    info.height = 1080;
    info.fpsNum = 30000;
    info.fpsDen = 1001;
    info.colorSpace = 1; // i420
    info.bitDepth = 8;
    info.totalFrames = 257339;

    std::string line = LogFormat::formatInputBanner("avs+", info, 0, 0, 0, 0);
    REQUIRE(line == "avs+ [info]: 1920x1080 fps 30000/1001 i420p8 frames 0 - 257338 of 257339");

    // With SAR
    std::string lineSar = LogFormat::formatInputBanner("y4m ", info, 0, 0, 1, 1);
    REQUIRE(lineSar == "y4m  [info]: 1920x1080 fps 30000/1001 i420p8 sar 1:1 frames 0 - 257338 of 257339");
}

TEST_CASE("LogFormat formatPresetTuneBanner generates preset and tune log", "[log]") {
    using sk265::utils::LogFormat;

    REQUIRE(LogFormat::formatPresetTuneBanner("medium", "vcbs") == "x265 [info]: Using preset medium & tune vcbs");
    REQUIRE(LogFormat::formatPresetTuneBanner("slow", "") == "x265 [info]: Using preset slow");
}

TEST_CASE("LogFormat formatOutputBanner generates output file log", "[log]") {
    using sk265::utils::LogFormat;

    REQUIRE(LogFormat::formatOutputBanner("gop+", "path/mv.gop") == "gop+ [info]: output file: path/mv.gop");
    REQUIRE(LogFormat::formatOutputBanner("mp4 ", "out.mp4") == "mp4  [info]: output file: out.mp4");
}

#include "pipeline/input/y4m_input.h"
#include "pipeline/input/avisynth_input.h"
#include "pipeline/input/vapoursynth_input.h"
#include "pipeline/input/lavf_input.h"
#include "pipeline/output/raw_output.h"
#include "pipeline/output/mp4_output.h"
#include "pipeline/output/lavf_output.h"
#include "pipeline/output/gop_output.h"
#include "pipeline/output/async_output.h"

TEST_CASE("Input and Output plugin tags are strictly 4 characters", "[log]") {
    sk265::pipeline::input::Y4mInput y4m;
    REQUIRE(y4m.getTag() == "y4m ");
    REQUIRE(y4m.getTag().size() == 4);

    sk265::pipeline::input::AviSynthInput avs;
    REQUIRE(avs.getTag() == "avs+");
    REQUIRE(avs.getTag().size() == 4);

    sk265::pipeline::input::VapourSynthInput vpy;
    REQUIRE(vpy.getTag() == "vpy ");
    REQUIRE(vpy.getTag().size() == 4);

    sk265::pipeline::input::LavfInput lavfIn;
    REQUIRE(lavfIn.getTag() == "lavf");
    REQUIRE(lavfIn.getTag().size() == 4);

    sk265::pipeline::output::RawOutput rawOut;
    REQUIRE(rawOut.getTag() == "raw ");
    REQUIRE(rawOut.getTag().size() == 4);

    sk265::pipeline::output::Mp4Output mp4Out;
    REQUIRE(mp4Out.getTag() == "mp4 ");
    REQUIRE(mp4Out.getTag().size() == 4);

    sk265::pipeline::output::LavfOutput lavfOut;
    REQUIRE(lavfOut.getTag() == "lavf");
    REQUIRE(lavfOut.getTag().size() == 4);

    sk265::pipeline::output::GopOutput gopOut;
    REQUIRE(gopOut.getTag() == "gop+");
    REQUIRE(gopOut.getTag().size() == 4);

    auto rawInner = std::make_unique<sk265::pipeline::output::RawOutput>();
    sk265::pipeline::output::AsyncOutput asyncOut(std::move(rawInner));
    REQUIRE(asyncOut.getTag() == "raw ");
    REQUIRE(asyncOut.getTag().size() == 4);
}
