#include <catch2/catch_test_macros.hpp>
#include "pipeline/output/gop_output.h"
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

TEST_CASE("GopOutput path and query string parsing", "[output][gop]") {
    sk265::pipeline::output::GopOutput gop;

    sk265::pipeline::output::OutputConfig cfg1;
    cfg1.outputPath = "video.gop";
    gop.open(cfg1);
    CHECK(gop.getGopPath() == "video.gop");
    CHECK(gop.getDirPrefix().empty());
    CHECK(gop.getFilenamePrefix() == "video");
    CHECK(gop.getFrameOffset() == 0);
    gop.close();
    fs::remove("video.gop");
    fs::remove("video.options");

    sk265::pipeline::output::OutputConfig cfg2;
    cfg2.outputPath = "dir/sub/sample.gop?start=240&foo=bar";
    fs::create_directories("dir/sub");
    gop.open(cfg2);
    CHECK(gop.getGopPath() == "dir/sub/sample.gop");
    CHECK((gop.getDirPrefix() == "dir/sub/" || gop.getDirPrefix() == "dir/sub\\"));
    CHECK(gop.getFilenamePrefix() == "sample");
    CHECK(gop.getFrameOffset() == 240);
    gop.close();
    fs::remove_all("dir");
}

TEST_CASE("GopOutput full output generation and protocol verification", "[output][gop]") {
    std::string testDir = "test_gop_run_temp";
    fs::remove_all(testDir);
    fs::create_directories(testDir);

    std::string gopFile = testDir + "/clip.gop?start=50";
    sk265::pipeline::output::GopOutput gop;

    sk265::pipeline::output::OutputConfig cfg;
    cfg.outputPath = gopFile;
    cfg.width = 1920;
    cfg.height = 1080;
    cfg.fpsNum = 24;
    cfg.fpsDen = 1;
    cfg.sarWidth = 1;
    cfg.sarHeight = 1;
    cfg.colorPrimaries = 1;
    cfg.transferCharacteristics = 1;
    cfg.matrixCoeffs = 1;
    cfg.fullRange = false;
    cfg.bframes = 4;
    cfg.bBPyramid = 1;
    cfg.timebaseNum = 1;
    cfg.timebaseDen = 24;

    REQUIRE(gop.open(cfg));

    // 1. Verify .options file content
    std::string optPath = testDir + "/clip.options";
    REQUIRE(fs::exists(optPath));
    {
        std::ifstream optStream(optPath);
        std::string optContent((std::istreambuf_iterator<char>(optStream)),
                                std::istreambuf_iterator<char>());
        CHECK(optContent.find("b-frames 4") != std::string::npos);
        CHECK(optContent.find("b-pyramid 1") != std::string::npos);
        CHECK(optContent.find("input-timebase-num 1") != std::string::npos);
        CHECK(optContent.find("input-timebase-den 24") != std::string::npos);
        CHECK(optContent.find("output-fps-num 24") != std::string::npos);
        CHECK(optContent.find("output-fps-den 1") != std::string::npos);
        CHECK(optContent.find("source-width 1920") != std::string::npos);
        CHECK(optContent.find("source-height 1080") != std::string::npos);
        CHECK(optContent.find("sar-width 1") != std::string::npos);
        CHECK(optContent.find("sar-height 1") != std::string::npos);
        CHECK(optContent.find("primaries-index 1") != std::string::npos);
        CHECK(optContent.find("transfer-index 1") != std::string::npos);
        CHECK(optContent.find("matrix-index 1") != std::string::npos);
        CHECK(optContent.find("full-range 0") != std::string::npos);
    }

    // 2. Write headers (VPS, SPS, PPS mock NAL units)
    uint8_t mockHdrData[] = {0x00, 0x00, 0x00, 0x04, 0x40, 0x01, 0x0c, 0x01,
                             0x00, 0x00, 0x00, 0x04, 0x42, 0x01, 0x01, 0x01};
    x265_nal nalsHdr[2];
    nalsHdr[0].payload = mockHdrData;
    nalsHdr[0].sizeBytes = 8;
    nalsHdr[1].payload = mockHdrData + 8;
    nalsHdr[1].sizeBytes = 8;

    REQUIRE(gop.writeHeaders(nalsHdr, 2));

    std::string hdrPath = testDir + "/clip.headers";
    REQUIRE(fs::exists(hdrPath));
    CHECK(fs::file_size(hdrPath) == 16);

    // 3. Write Frame 0: IDR (start of first GOP chunk)
    uint8_t mockFrameNal[] = {0x00, 0x00, 0x00, 0x02, 0x26, 0x01};
    x265_nal nalFrame;
    nalFrame.payload = mockFrameNal;
    nalFrame.sizeBytes = sizeof(mockFrameNal);

    x265_picture pic0{};
    pic0.sliceType = X265_TYPE_IDR;
    pic0.pts = 1000;
    pic0.dts = 1000;
    REQUIRE(gop.writeFrame(&nalFrame, 1, pic0));

    std::string chunk0 = testDir + "/clip-000050.hevc-gop-data";
    REQUIRE(fs::exists(chunk0));

    // 4. Write Frame 1: Non-IDR (P/B slice in same chunk)
    x265_picture pic1{};
    pic1.sliceType = X265_TYPE_P;
    pic1.pts = 1041;
    pic1.dts = 1041;
    REQUIRE(gop.writeFrame(&nalFrame, 1, pic1));

    // 5. Write Frame 2: IDR (start of second GOP chunk)
    x265_picture pic2{};
    pic2.sliceType = X265_TYPE_IDR;
    pic2.pts = 1083;
    pic2.dts = 1083;
    REQUIRE(gop.writeFrame(&nalFrame, 1, pic2));

    std::string chunk1 = testDir + "/clip-000052.hevc-gop-data";
    REQUIRE(fs::exists(chunk1));

    // 6. Close and verify manifest trailer
    gop.close();

    std::string gopManifestPath = testDir + "/clip.gop";
    REQUIRE(fs::exists(gopManifestPath));
    {
        std::ifstream gopStream(gopManifestPath);
        std::string gopContent((std::istreambuf_iterator<char>(gopStream)),
                                std::istreambuf_iterator<char>());

        CHECK(gopContent.find("#options clip.options") != std::string::npos);
        CHECK(gopContent.find("#headers clip.headers") != std::string::npos);
        CHECK(gopContent.find("clip-000050.hevc-gop-data") != std::string::npos);
        CHECK(gopContent.find("clip-000052.hevc-gop-data") != std::string::npos);
        CHECK(gopContent.find("# 3 frames written, last frame 53") != std::string::npos);
    }

    // 7. Verify binary chunk frame header format: 4-byte size (16) + PTS + DTS + NAL
    std::ifstream chunkStream(chunk0, std::ios::binary);
    uint8_t tsLen[4] = {0};
    int64_t pts = 0;
    int64_t dts = 0;
    chunkStream.read(reinterpret_cast<char*>(tsLen), 4);
    chunkStream.read(reinterpret_cast<char*>(&pts), sizeof(int64_t));
    chunkStream.read(reinterpret_cast<char*>(&dts), sizeof(int64_t));

    CHECK(tsLen[0] == 0x00);
    CHECK(tsLen[1] == 0x00);
    CHECK(tsLen[2] == 0x00);
    CHECK(tsLen[3] == 0x10);
    CHECK(pts == 1000);
    CHECK(dts == 1000);

    chunkStream.close();
    fs::remove_all(testDir);
}
