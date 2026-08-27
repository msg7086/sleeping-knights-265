#include <catch2/catch_test_macros.hpp>
#include "utils/log_mirror.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

TEST_CASE("LogMirror captures stderr and std::cerr cleanly", "[utils][log_mirror]") {
    std::string logFile = "test_log_mirror_capture.log";
    fs::remove(logFile);

    {
        sk265::utils::LogMirror mirror;
        REQUIRE(mirror.start(logFile));
        CHECK(mirror.isRunning());

        std::cerr << "sk265[info]: test message from std::cerr\n";
        std::fprintf(stderr, "x265 [info]: test message from C fprintf stderr\n");
        std::fflush(stderr);

        mirror.stop();
        CHECK_FALSE(mirror.isRunning());
    }

    REQUIRE(fs::exists(logFile));
    {
        std::ifstream ifs(logFile);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
        CHECK(content.find("sk265[info]: test message from std::cerr") != std::string::npos);
        CHECK(content.find("x265 [info]: test message from C fprintf stderr") != std::string::npos);
    }
    fs::remove(logFile);
}

TEST_CASE("LogMirror filters carriage-return progress lines from log file", "[utils][log_mirror]") {
    std::string logFile = "test_log_mirror_progress.log";
    fs::remove(logFile);

    {
        sk265::utils::LogMirror mirror;
        REQUIRE(mirror.start(logFile));

        std::cerr << "sk265[info]: starting test encode\n";
        std::cerr << "[ 10.0%] 10/100 frames, 25.00 fps\r";
        std::cerr << "[ 20.0%] 20/100 frames, 25.00 fps\r";
        std::cerr << "sk265[info]: finished test encode\n";
        std::fflush(stderr);

        mirror.stop();
    }

    REQUIRE(fs::exists(logFile));
    {
        std::ifstream ifs(logFile);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
        CHECK(content.find("sk265[info]: starting test encode") != std::string::npos);
        CHECK(content.find("sk265[info]: finished test encode") != std::string::npos);
        CHECK(content.find("[ 10.0%]") == std::string::npos);
        CHECK(content.find("[ 20.0%]") == std::string::npos);
    }
    fs::remove(logFile);
}
