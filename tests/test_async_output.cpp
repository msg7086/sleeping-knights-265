#include <catch2/catch_test_macros.hpp>
#include "pipeline/output/async_output.h"
#include "pipeline/output/raw_output.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

class MockSlowOutput : public sk265::pipeline::output::IOutput {
public:
    bool open(const sk265::pipeline::output::OutputConfig& /*config*/) override {
        opened = true;
        return true;
    }

    bool writeHeaders(const x265_nal* nals, uint32_t nalCount) override {
        headerCallCount++;
        for (uint32_t i = 0; i < nalCount; ++i) {
            headerBytes += nals[i].sizeBytes;
        }
        return true;
    }

    bool writeFrame(const x265_nal* nals, uint32_t nalCount, const x265_picture& pic) override {
        // Simulate slow disk / NFS latency (20ms per frame)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        frameCount++;
        ptsList.push_back(pic.pts);
        for (uint32_t i = 0; i < nalCount; ++i) {
            frameBytes += nals[i].sizeBytes;
        }
        return true;
    }

    void close(int64_t largestPts = -1, int64_t secondLargestPts = -1) override {
        closed = true;
        savedLargestPts = largestPts;
        savedSecondLargestPts = secondLargestPts;
    }

    bool opened{false};
    bool closed{false};
    int headerCallCount{0};
    size_t headerBytes{0};
    int frameCount{0};
    size_t frameBytes{0};
    std::vector<int64_t> ptsList;
    int64_t savedLargestPts{-1};
    int64_t savedSecondLargestPts{-1};
};

TEST_CASE("AsyncOutput absorbs I/O latency and writes all packets in order", "[output][async]") {
    auto mock = std::make_unique<MockSlowOutput>();
    auto* mockPtr = mock.get();

    sk265::pipeline::output::AsyncOutput asyncOut(std::move(mock), 64);

    sk265::pipeline::output::OutputConfig cfg;
    cfg.outputPath = "mock.hevc";
    REQUIRE(asyncOut.open(cfg));
    CHECK(mockPtr->opened);

    uint8_t mockHdrData[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    x265_nal hdrNal;
    hdrNal.payload = mockHdrData;
    hdrNal.sizeBytes = sizeof(mockHdrData);
    hdrNal.type = 32; // VPS
    REQUIRE(asyncOut.writeHeaders(&hdrNal, 1));

    // Submit 5 frames in rapid succession (should take <10ms to submit, while mock takes 5*20ms = 100ms)
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 5; ++i) {
        uint8_t frameData[8] = {static_cast<uint8_t>(i)};
        x265_nal frmNal;
        frmNal.payload = frameData;
        frmNal.sizeBytes = sizeof(frameData);
        frmNal.type = 1;

        x265_picture pic{};
        pic.pts = i * 100;
        pic.dts = i * 100;
        pic.sliceType = (i == 0) ? X265_TYPE_IDR : X265_TYPE_P;

        REQUIRE(asyncOut.writeFrame(&frmNal, 1, pic));
    }
    auto tSubmit = std::chrono::steady_clock::now();
    auto submitMs = std::chrono::duration_cast<std::chrono::milliseconds>(tSubmit - t0).count();
    // Submitting 5 frames to async queue should be near instant (< 50ms)
    CHECK(submitMs < 80);

    // Close and wait for drain
    asyncOut.close(400, 300);

    CHECK(mockPtr->closed);
    CHECK(mockPtr->headerCallCount == 1);
    CHECK(mockPtr->headerBytes == 16);
    CHECK(mockPtr->frameCount == 5);
    CHECK(mockPtr->frameBytes == 5 * 8);
    CHECK(mockPtr->savedLargestPts == 400);
    CHECK(mockPtr->savedSecondLargestPts == 300);

    std::vector<int64_t> expectedPts = {0, 100, 200, 300, 400};
    CHECK(mockPtr->ptsList == expectedPts);
}

TEST_CASE("AsyncOutput integration with RawOutput file writer", "[output][async]") {
    std::string testFile = "test_async_raw_out.hevc";
    fs::remove(testFile);

    auto raw = std::make_unique<sk265::pipeline::output::RawOutput>();
    sk265::pipeline::output::AsyncOutput asyncOut(std::move(raw), 32);

    sk265::pipeline::output::OutputConfig cfg;
    cfg.outputPath = testFile;
    REQUIRE(asyncOut.open(cfg));

    uint8_t data[] = {0x00, 0x00, 0x00, 0x01, 0x40, 0x01};
    x265_nal nal;
    nal.payload = data;
    nal.sizeBytes = sizeof(data);
    nal.type = 32;

    REQUIRE(asyncOut.writeHeaders(&nal, 1));

    x265_picture pic{};
    pic.pts = 0;
    pic.sliceType = X265_TYPE_IDR;
    REQUIRE(asyncOut.writeFrame(&nal, 1, pic));

    asyncOut.close();

    REQUIRE(fs::exists(testFile));
    CHECK(fs::file_size(testFile) == 2 * sizeof(data));
    fs::remove(testFile);
}
