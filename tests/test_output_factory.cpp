#include <catch2/catch_test_macros.hpp>
#include "pipeline/output/output_factory.h"

TEST_CASE("OutputFactory rejects unknown muxer names", "[output][factory]") {
    auto result = sk265::pipeline::output::OutputFactory::create("unknown_muxer", "out.mp4");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().find("Unknown muxer") != std::string::npos);
}

TEST_CASE("OutputFactory rejects incompatible muxer and container combinations", "[output][factory]") {
    // 1. L-SMASH cannot output MKV
    auto lsmashMkv = sk265::pipeline::output::OutputFactory::create("lsmash", "out.mkv");
    REQUIRE_FALSE(lsmashMkv.has_value());
    REQUIRE(lsmashMkv.error().find("lsmash") != std::string::npos);

    // 2. L-SMASH cannot output raw HEVC
    auto lsmashHevc = sk265::pipeline::output::OutputFactory::create("lsmash", "out.hevc");
    REQUIRE_FALSE(lsmashHevc.has_value());

    // 3. LAVF container muxer cannot output raw annex-B stream
    auto lavfHevc = sk265::pipeline::output::OutputFactory::create("lavf", "out.hevc");
    REQUIRE_FALSE(lavfHevc.has_value());

    // 4. Raw muxer cannot output MP4 or MKV container
    auto rawMp4 = sk265::pipeline::output::OutputFactory::create("raw", "out.mp4");
    REQUIRE_FALSE(rawMp4.has_value());
    auto rawMkv = sk265::pipeline::output::OutputFactory::create("raw", "out.mkv");
    REQUIRE_FALSE(rawMkv.has_value());

    // 5. GOP muxer cannot output MP4
    auto gopMp4 = sk265::pipeline::output::OutputFactory::create("gop", "out.mp4");
    REQUIRE_FALSE(gopMp4.has_value());
}

TEST_CASE("OutputFactory accepts valid combinations", "[output][factory]") {
    REQUIRE(sk265::pipeline::output::OutputFactory::create("auto", "out.mp4").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("auto", "out.mkv").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("auto", "out.hevc").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("auto", "out.h265").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("auto", "out.265").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("auto", "out.bin").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("auto", "out.raw").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("auto", "-").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("auto", "out.gop").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("auto", "out.gop?start=100").has_value());

    REQUIRE(sk265::pipeline::output::OutputFactory::create("lsmash", "out.mp4").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("lsmash", "out.m4v").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("lsmash", "out.mov").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("lavf", "out.mp4").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("ffmpeg", "out.mkv").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("raw", "out.hevc").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("raw", "out.265").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("raw", "-").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("gop", "out.gop").has_value());
    REQUIRE(sk265::pipeline::output::OutputFactory::create("gop", "out.gop?start=200").has_value());
}
