#include <catch2/catch_test_macros.hpp>
#include "config/cli_options.h"
#include "config/cli_parser.h"

TEST_CASE("CliParser extracts frontend flags and collects pass-through params", "[config]") {
    std::vector<std::string> args = {
        "sk265",
        "-i", "input.y4m",
        "-o", "output.hevc",
        "-D", "10",
        "--preset", "slow",
        "--tune", "grain",
        "--crf", "18",
        "--frames", "50",
        "--seek", "10",
        "--aq-mode", "3",
        "--qg-size", "16",
        "--no-info"
    };

    auto opts = sk265::config::CliParser::parse(args);
    REQUIRE(opts.inputPath == "input.y4m");
    REQUIRE(opts.outputPath == "output.hevc");
    REQUIRE(opts.bitDepth == 10);
    REQUIRE(opts.frameCount == 50);
    REQUIRE(opts.seekFrame == 10);
    REQUIRE(opts.encoderParams["preset"] == "slow");
    REQUIRE(opts.encoderParams["tune"] == "grain");
    REQUIRE(opts.encoderParams["crf"] == "18");
    REQUIRE(opts.encoderParams["aq-mode"] == "3");
    REQUIRE(opts.encoderParams["qg-size"] == "16");
    REQUIRE(opts.encoderParams["no-info"] == "true");
}

TEST_CASE("CliParser handles repeated parameters and later ones overwrite earlier ones", "[config]") {
    std::vector<std::string> args = {
        "sk265",
        "-i", "first_input.y4m",
        "-i", "second_input.y4m",
        "-p", "fast",
        "--preset", "veryslow",
        "--crf", "20",
        "--crf", "22",
        "--aq-mode", "1",
        "--aq-mode", "3"
    };

    auto opts = sk265::config::CliParser::parse(args);
    // Verified: later arguments overwrite earlier ones
    REQUIRE(opts.inputPath == "second_input.y4m");
    REQUIRE(opts.encoderParams["preset"] == "veryslow");
    REQUIRE(opts.encoderParams["crf"] == "22");
    REQUIRE(opts.encoderParams["aq-mode"] == "3");
}

TEST_CASE("CliParser parses --avs-lib option cleanly", "[config]") {
    std::vector<std::string> args = {
        "sk265",
        "-i", "test.avs",
        "-o", "out.hevc",
        "--avs-lib", "C:/AviSynthPlus/AviSynth.dll"
    };

    auto opts = sk265::config::CliParser::parse(args);
    REQUIRE(opts.inputPath == "test.avs");
    REQUIRE(opts.avsLibPath == "C:/AviSynthPlus/AviSynth.dll");
}

TEST_CASE("CliParser parses --vpy-lib option cleanly", "[config]") {
    std::vector<std::string> args = {
        "sk265",
        "-i", "script.vpy",
        "-o", "out.hevc",
        "--vpy-lib", "C:/VapourSynth/core/vsscript.dll"
    };

    auto opts = sk265::config::CliParser::parse(args);
    REQUIRE(opts.inputPath == "script.vpy");
    REQUIRE(opts.vpyLibPath == "C:/VapourSynth/core/vsscript.dll");
}

TEST_CASE("CliParser parses --muxer option cleanly", "[config]") {
    std::vector<std::string> args = {
        "sk265",
        "-i", "input.y4m",
        "-o", "out.mp4",
        "--muxer", "lavf"
    };

    auto opts = sk265::config::CliParser::parse(args);
    REQUIRE(opts.muxer == "lavf");
}

TEST_CASE("CliParser parses --stylish option cleanly", "[config]") {
    std::vector<std::string> args = {
        "sk265",
        "-i", "input.y4m",
        "-o", "out.mp4",
        "--stylish"
    };

    auto opts = sk265::config::CliParser::parse(args);
    REQUIRE(opts.bStylish);
}

TEST_CASE("CliParser supports key=value syntax", "[config]") {
    std::vector<std::string> args = {
        "sk265",
        "-i", "input.y4m",
        "-o", "output.hevc",
        "--crf=16",
        "--qg-size=32"
    };

    auto opts = sk265::config::CliParser::parse(args);
    REQUIRE(opts.encoderParams["crf"] == "16");
    REQUIRE(opts.encoderParams["qg-size"] == "32");
}
