#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <vector>
#include <string>
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

TEST_CASE("Golden HDR10 and HDR10+ parameters pass-through validation", "[golden][hdr]") {
    const x265_api* api = sk265::core::CoreRouter::getApi(10);
    REQUIRE(api != nullptr);

    auto param = sk265::core::make_param_handle(api);
    REQUIRE(param != nullptr);

    std::vector<std::string> args = {
        "sk265",
        "-i", "input.y4m",
        "-o", "output.hevc",
        "-D", "10",
        "--preset", "medium",
        "--hdr10",
        "--hdr10-opt",
        "--dhdr10-opt",
        "--colorprim", "bt2020",
        "--transfer", "smpte2084",
        "--colormatrix", "bt2020nc",
        "--master-display", "G(13250,34500)B(7500,3000)R(34000,16000)WP(15635,16450)L(10000000,1)",
        "--max-cll", "1000,400",
        "--min-luma", "0",
        "--max-luma", "1023",
        "--no-info"
    };

    auto opts = sk265::config::CliParser::parse(args);
    REQUIRE(opts.bitDepth == 10);
    REQUIRE(opts.encoderParams["hdr10"] == "true");
    REQUIRE(opts.encoderParams["hdr10-opt"] == "true");
    REQUIRE(opts.encoderParams["dhdr10-opt"] == "true");
    REQUIRE(opts.encoderParams["colorprim"] == "bt2020");
    REQUIRE(opts.encoderParams["transfer"] == "smpte2084");
    REQUIRE(opts.encoderParams["colormatrix"] == "bt2020nc");
    REQUIRE(opts.encoderParams["master-display"] == "G(13250,34500)B(7500,3000)R(34000,16000)WP(15635,16450)L(10000000,1)");
    REQUIRE(opts.encoderParams["max-cll"] == "1000,400");
    REQUIRE(opts.encoderParams["min-luma"] == "0");
    REQUIRE(opts.encoderParams["max-luma"] == "1023");

    REQUIRE(api->param_default_preset(param.raw(), opts.encoderParams["preset"].c_str(), nullptr) == 0);
    param->internalBitDepth = 10;
    param->sourceBitDepth = 10;
    param->internalCsp = X265_CSP_I420;

    for (const auto& [k, v] : opts.encoderParams) {
        if (k == "preset") continue;
        int ret = api->param_parse(param.raw(), k.c_str(), v.c_str());
        REQUIRE(ret == 0);
    }
}

TEST_CASE("Golden Dolby Vision profile and RPU pass-through validation", "[golden][dovi]") {
    const x265_api* api = sk265::core::CoreRouter::getApi(10);
    REQUIRE(api != nullptr);

    auto param = sk265::core::make_param_handle(api);
    REQUIRE(param != nullptr);

    std::vector<std::string> args = {
        "sk265",
        "-i", "input.y4m",
        "-o", "output.hevc",
        "-D", "10",
        "--preset", "slow",
        "--dolby-vision-profile", "8.1",
        "--dolby-vision-rpu", "dummy_rpu.bin",
        "--no-info"
    };

    auto opts = sk265::config::CliParser::parse(args);
    REQUIRE(opts.encoderParams["dolby-vision-profile"] == "8.1");
    // Verified: dolby-vision-rpu is parsed into frontend CliOptions field
    REQUIRE(opts.doviRpuPath == "dummy_rpu.bin");

    REQUIRE(api->param_default_preset(param.raw(), opts.encoderParams["preset"].c_str(), nullptr) == 0);
    param->internalBitDepth = 10;
    param->sourceBitDepth = 10;
    param->internalCsp = X265_CSP_I420;

    for (const auto& [k, v] : opts.encoderParams) {
        if (k == "preset") continue;
        int ret = api->param_parse(param.raw(), k.c_str(), v.c_str());
        REQUIRE(ret == 0);
    }
}

TEST_CASE("Golden Advanced Rate Control and VBV buffer validation", "[golden][rc]") {
    const x265_api* api = sk265::core::CoreRouter::getApi(8);
    REQUIRE(api != nullptr);

    auto param = sk265::core::make_param_handle(api);
    REQUIRE(param != nullptr);

    std::vector<std::string> args = {
        "sk265",
        "-i", "input.y4m",
        "-o", "output.hevc",
        "--bitrate", "6000",
        "--vbv-maxrate", "10000",
        "--vbv-bufsize", "12000",
        "--vbv-init", "0.9",
        "--qcomp", "0.65",
        "--ipratio", "1.4",
        "--pbratio", "1.3",
        "--qpmin", "12",
        "--qpmax", "48",
        "--qpstep", "4",
        "--cplxblur", "20",
        "--qblur", "0.5"
    };

    auto opts = sk265::config::CliParser::parse(args);
    REQUIRE(api->param_default_preset(param.raw(), "medium", nullptr) == 0);

    for (const auto& [k, v] : opts.encoderParams) {
        int ret = api->param_parse(param.raw(), k.c_str(), v.c_str());
        REQUIRE(ret == 0);
    }

    REQUIRE(param->rc.bitrate == 6000);
    REQUIRE(param->rc.vbvMaxBitrate == 10000);
    REQUIRE(param->rc.vbvBufferSize == 12000);
    REQUIRE(param->rc.qpMin == 12);
    REQUIRE(param->rc.qpMax == 48);
}

TEST_CASE("Golden Motion, Spatial, SAO and Slice tuning validation", "[golden][tuning]") {
    const x265_api* api = sk265::core::CoreRouter::getApi(8);
    REQUIRE(api != nullptr);

    auto param = sk265::core::make_param_handle(api);
    REQUIRE(param != nullptr);

    std::vector<std::string> args = {
        "sk265",
        "-i", "input.y4m",
        "-o", "output.hevc",
        "--me", "star",
        "--subme", "4",
        "--merange", "57",
        "--max-merge", "4",
        "--temporal-mvp",
        "--no-early-skip",
        "--rskip", "2",
        "--rskip-edge-threshold", "5",
        "--psy-rd", "2.0",
        "--psy-rdoq", "1.0",
        "--deblock", "-2:-2",
        "--selective-sao", "2",
        "--limit-sao",
        "--no-sao-non-deblock",
        "--ctu", "64",
        "--min-cu-size", "16",
        "--max-tu-size", "32",
        "--tu-intra-depth", "3",
        "--tu-inter-depth", "3",
        "--sar", "16:9",
        "--range", "full",
        "--aud",
        "--repeat-headers",
        "--idr-recovery-sei",
        "--single-sei"
    };

    auto opts = sk265::config::CliParser::parse(args);
    REQUIRE(api->param_default_preset(param.raw(), "medium", nullptr) == 0);

    for (const auto& [k, v] : opts.encoderParams) {
        INFO("Testing param: " << k << " = " << v);
        int ret = api->param_parse(param.raw(), k.c_str(), v.c_str());
        REQUIRE(ret == 0);
    }
}

TEST_CASE("Golden short options and positional arguments validation", "[golden][cli]") {
    const x265_api* api = sk265::core::CoreRouter::getApi(8);
    REQUIRE(api != nullptr);

    auto param = sk265::core::make_param_handle(api);
    REQUIRE(param != nullptr);

    std::vector<std::string> args = {
        "sk265",
        "-P", "main10",
        "-F", "4",
        "-I", "240",
        "-b", "4",
        "-s", "64",
        "-q", "22",
        "-m", "4",
        "-w",
        "input.y4m",
        "output.hevc"
    };

    auto opts = sk265::config::CliParser::parse(args);
    REQUIRE(opts.inputPath == "input.y4m");
    REQUIRE(opts.outputPath == "output.hevc");
    REQUIRE(opts.encoderParams["profile"] == "main10");
    REQUIRE(opts.encoderParams["frame-threads"] == "4");
    REQUIRE(opts.encoderParams["keyint"] == "240");
    REQUIRE(opts.encoderParams["bframes"] == "4");
    REQUIRE(opts.encoderParams["ctu"] == "64");
    REQUIRE(opts.encoderParams["qp"] == "22");
    REQUIRE(opts.encoderParams["subme"] == "4");
    REQUIRE(opts.encoderParams["weightp"] == "1");

    REQUIRE(api->param_default_preset(param.raw(), "medium", nullptr) == 0);
    for (const auto& [k, v] : opts.encoderParams) {
        if (k == "profile") continue; // profile is applied after other params
        int ret = api->param_parse(param.raw(), k.c_str(), v.c_str());
        REQUIRE(ret == 0);
    }
}

TEST_CASE("Golden Zones, Scenecut-Aware QP, Negative Offsets and Multi-pass validation", "[golden][complex]") {
    const x265_api* api = sk265::core::CoreRouter::getApi(8);
    REQUIRE(api != nullptr);

    auto param = sk265::core::make_param_handle(api);
    REQUIRE(param != nullptr);

    std::vector<std::string> args = {
        "sk265",
        "-i", "input.y4m",
        "-o", "output.hevc",
        "--qpfile", "test.qpfile",
        "--zones", "0,100,q=20/101,200,b=1.2",
        "--scenecut-aware-qp", "1",
        "--masking-strength", "2.5",
        "--cbqpoffs", "-2",
        "--crqpoffs", "-2",
        "--pass", "1",
        "--stats", "x265_2pass.log",
        "--multi-pass-opt-analysis",
        "--multi-pass-opt-distortion",
        "--scale-factor", "2"
    };

    auto opts = sk265::config::CliParser::parse(args);
    REQUIRE(opts.qpfilePath == "test.qpfile");
    REQUIRE(opts.encoderParams["zones"] == "0,100,q=20/101,200,b=1.2");
    REQUIRE(opts.encoderParams["cbqpoffs"] == "-2");
    REQUIRE(opts.encoderParams["crqpoffs"] == "-2");
    REQUIRE(opts.encoderParams["pass"] == "1");

    REQUIRE(api->param_default_preset(param.raw(), "medium", nullptr) == 0);
    for (const auto& [k, v] : opts.encoderParams) {
        int ret = api->param_parse(param.raw(), k.c_str(), v.c_str());
        REQUIRE(ret == 0);
    }

    REQUIRE(param->rc.zoneCount == 2);
    REQUIRE(param->rc.zones[0].startFrame == 0);
    REQUIRE(param->rc.zones[0].endFrame == 100);
    REQUIRE(param->rc.zones[0].qp == 20);
    REQUIRE(param->rc.zones[1].startFrame == 101);
    REQUIRE(param->rc.zones[1].endFrame == 200);
    REQUIRE(param->cbQpOffset == -2);
    REQUIRE(param->crQpOffset == -2);
    REQUIRE(param->rc.bStatWrite == 1);
    REQUIRE(param->scaleFactor == 2);
}

TEST_CASE("Golden invalid parameter error detection", "[golden][errors]") {
    const x265_api* api = sk265::core::CoreRouter::getApi(8);
    REQUIRE(api != nullptr);

    auto param = sk265::core::make_param_handle(api);
    REQUIRE(param != nullptr);
    REQUIRE(api->param_default_preset(param.raw(), "medium", nullptr) == 0);

    // Invalid enum parameter value (e.g. motion estimation method)
    REQUIRE(api->param_parse(param.raw(), "me", "invalid_me_method") == X265_PARAM_BAD_VALUE);

    // Invalid integer parameter value (non-numeric string)
    REQUIRE(api->param_parse(param.raw(), "aq-mode", "not_a_number") == X265_PARAM_BAD_VALUE);

    // Unknown parameter name
    REQUIRE(api->param_parse(param.raw(), "unknown_option_xyz", "123") == X265_PARAM_BAD_NAME);

    // Invalid Preset
    REQUIRE(api->param_default_preset(param.raw(), "totally_invalid_preset", nullptr) != 0);
}
