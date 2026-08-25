#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include "pipeline/output/raw_output.h"

TEST_CASE("RawOutput writes bytes cleanly to output file", "[output]") {
    std::string testPath = "test_temp_output.hevc";
    sk265::pipeline::output::RawOutput out;
    REQUIRE(out.open(testPath));

    std::vector<uint8_t> testBytes = {0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0c, 0x01};
    REQUIRE(out.writeBytes(testBytes.data(), testBytes.size()));
    out.close();

    REQUIRE(std::filesystem::exists(testPath));
    REQUIRE(std::filesystem::file_size(testPath) == testBytes.size());
    std::filesystem::remove(testPath);
}

TEST_CASE("RawOutput writes NAL headers and frame packets", "[output]") {
    std::string testPath = "test_temp_nals.hevc";
    sk265::pipeline::output::RawOutput out;
    REQUIRE(out.open(testPath));

    uint8_t nalPayload[] = {0x00, 0x00, 0x00, 0x01, 0x42, 0x01};
    x265_nal nal{};
    nal.type = 33; // SPS
    nal.sizeBytes = sizeof(nalPayload);
    nal.payload = nalPayload;

    REQUIRE(out.writeHeaders(&nal, 1));
    out.close();

    REQUIRE(std::filesystem::exists(testPath));
    REQUIRE(std::filesystem::file_size(testPath) == sizeof(nalPayload));
    std::filesystem::remove(testPath);
}
