#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <vector>
#include <chrono>
#include "pipeline/bounded_queue.h"
#include "config/cli_options.h"

TEST_CASE("BoundedQueue handles multi-threaded push/pop with backpressure", "[pipeline][queue]") {
    sk265::pipeline::BoundedQueue<int> queue(4);
    REQUIRE(queue.capacity() == 4);

    std::vector<int> received;
    std::jthread consumer([&](std::stop_token st) {
        while (!st.stop_requested()) {
            auto item = queue.pop(st);
            if (!item.has_value()) break;
            received.push_back(*item);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    });

    for (int i = 0; i < 20; ++i) {
        REQUIRE(queue.push(i));
    }
    queue.close();
    consumer.join();

    REQUIRE(received.size() == 20);
    for (int i = 0; i < 20; ++i) {
        REQUIRE(received[i] == i);
    }
}

TEST_CASE("BoundedQueue stops cleanly via stop_token", "[pipeline][queue]") {
    sk265::pipeline::BoundedQueue<int> queue(10);
    std::stop_source source;

    std::jthread worker([&](std::stop_token st) {
        auto val = queue.pop(st);
        REQUIRE_FALSE(val.has_value());
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    source.request_stop();
    queue.close();
    worker.join();
}

TEST_CASE("QueueSizeConfig resolution-adaptive default tiers", "[config][queue]") {
    sk265::config::QueueSizeConfig defaultCfg;
    REQUIRE(defaultCfg.mode == sk265::config::QueueSizeConfig::Mode::DefaultAuto);

    // 4K UHD -> 4 frames
    REQUIRE(defaultCfg.resolveCapacity(3840, 2160, 10) == 4);
    // 8K -> 4 frames
    REQUIRE(defaultCfg.resolveCapacity(7680, 4320, 10) == 4);

    // 1440p / 2K -> 8 frames
    REQUIRE(defaultCfg.resolveCapacity(2560, 1440, 8) == 8);

    // 1080p -> 16 frames
    REQUIRE(defaultCfg.resolveCapacity(1920, 1080, 8) == 16);

    // 720p -> 32 frames
    REQUIRE(defaultCfg.resolveCapacity(1280, 720, 8) == 32);

    // SD 480p -> 32 frames
    REQUIRE(defaultCfg.resolveCapacity(640, 480, 8) == 32);
}

TEST_CASE("QueueSizeConfig parses integers and memory units with [2, 64] clamp", "[config][queue]") {
    // 1. Pure frame integers
    auto cfg1 = sk265::config::QueueSizeConfig::parse("16");
    REQUIRE(cfg1.mode == sk265::config::QueueSizeConfig::Mode::Frames);
    REQUIRE(cfg1.resolveCapacity(1920, 1080, 8) == 16);

    // Clamped minimum (0 or 1 -> 2)
    auto cfg2 = sk265::config::QueueSizeConfig::parse("1");
    REQUIRE(cfg2.resolveCapacity(1920, 1080, 8) == 2);

    // Clamped maximum (100 -> 64)
    auto cfg3 = sk265::config::QueueSizeConfig::parse("100");
    REQUIRE(cfg3.resolveCapacity(1920, 1080, 8) == 64);

    // 2. Memory units (e.g. 512M)
    auto cfgMem = sk265::config::QueueSizeConfig::parse("512MB");
    REQUIRE(cfgMem.mode == sk265::config::QueueSizeConfig::Mode::MemoryBytes);
    // 1080p 8-bit YUV420 is ~3.1 MB/frame -> 512MB / 3.1MB = ~164 frames -> clamped to 64
    REQUIRE(cfgMem.resolveCapacity(1920, 1080, 8) == 64);

    // 4K 10-bit YUV420 is ~24.8 MB/frame -> 100MB / 24.8MB = 4 frames
    auto cfg4k = sk265::config::QueueSizeConfig::parse("100M");
    REQUIRE(cfg4k.resolveCapacity(3840, 2160, 10) == 4);
}
