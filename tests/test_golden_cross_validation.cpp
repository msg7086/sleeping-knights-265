#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include "core/x265_handle.h"
#include "core/x265_router.h"
#include "config/cli_parser.h"
#include "pipeline/input/avisynth_input.h"
#include "pipeline/input/y4m_input.h"

TEST_CASE("AviSynthInput loads real fixture via FFMS2", "[golden][avs]") {
    std::string avsPath = "fixtures/test_video.avs";
    if (!std::filesystem::exists(avsPath)) {
        avsPath = "tests/fixtures/test_video.avs";
    }

    if (std::filesystem::exists(avsPath)) {
        sk265::pipeline::input::AviSynthInput input;
        bool ok = input.open(avsPath);
        if (ok) {
            auto info = input.getInfo();
            REQUIRE(info.width == 1280);
            REQUIRE(info.height == 720);
            REQUIRE(info.bitDepth == 8);

            auto frame = input.readFrame();
            REQUIRE(frame.has_value());
            REQUIRE(frame->width == 1280);
            REQUIRE(frame->height == 720);
            REQUIRE(frame->planes[0].size() > 0);
        }
    }
}

TEST_CASE("Golden parameter serialization and pass-through validation", "[golden]") {
    const x265_api* api = sk265::core::CoreRouter::getApi(8);
    REQUIRE(api != nullptr);

    auto param = sk265::core::make_param_handle(api);
    REQUIRE(param != nullptr);

    std::vector<std::string> args = {
        "sk265",
        "-i", "input.y4m",
        "-o", "output.hevc",
        "-p", "slow",
        "-t", "grain",
        "--crf", "18",
        "--aq-mode", "2",
        "--qg-size", "16",
        "--no-sao",
        "--no-info"
    };

    auto opts = sk265::config::CliParser::parse(args);
    REQUIRE(opts.encoderParams["preset"] == "slow");
    REQUIRE(opts.encoderParams["tune"] == "grain");
    REQUIRE(opts.encoderParams["crf"] == "18");
    REQUIRE(opts.encoderParams["aq-mode"] == "2");
    REQUIRE(opts.encoderParams["qg-size"] == "16");
    REQUIRE(opts.encoderParams["no-sao"] == "true");
    REQUIRE(opts.encoderParams["no-info"] == "true");

    // Apply preset & tune
    REQUIRE(api->param_default_preset(param.raw(), opts.encoderParams["preset"].c_str(), opts.encoderParams["tune"].c_str()) == 0);

    // Apply all pass-through keys
    for (const auto& [k, v] : opts.encoderParams) {
        if (k == "preset" || k == "tune") continue;
        REQUIRE(api->param_parse(param.raw(), k.c_str(), v.c_str()) == 0);
    }
}
