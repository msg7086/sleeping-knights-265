#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <vector>
#include "pipeline/output/mp4_output.h"
#include "core/x265_handle.h"
#include "core/x265_router.h"

TEST_CASE("Mp4Output handles invalid destination gracefully", "[output][mp4]") {
    sk265::pipeline::output::Mp4Output output;
    sk265::pipeline::output::OutputConfig cfg;
    cfg.outputPath = ""; // empty path
    cfg.width = 1280;
    cfg.height = 720;
    cfg.fpsNum = 25;
    cfg.fpsDen = 1;

    REQUIRE_FALSE(output.open(cfg));
    REQUIRE_FALSE(output.isOpen());
}

TEST_CASE("Mp4Output initializes MP4 container, writes NALs and finishes cleanly", "[output][mp4]") {
    std::string testPath = "test_output_mux.mp4";
    if (std::filesystem::exists(testPath)) {
        std::filesystem::remove(testPath);
    }

    const x265_api* api = sk265::core::CoreRouter::getApi(8);
    REQUIRE(api != nullptr);

    auto param = sk265::core::make_param_handle(api);
    api->param_default_preset(param.raw(), "ultrafast", nullptr);
    param->sourceWidth = 1280;
    param->sourceHeight = 720;
    param->fpsNum = 25;
    param->fpsDenom = 1;
    param->internalBitDepth = 8;
    param->internalCsp = X265_CSP_I420;
    param->bAnnexB = false;
    param->bRepeatHeaders = false;

    auto encoder = sk265::core::make_encoder_handle(api, param.raw());
    REQUIRE(encoder != nullptr);

    sk265::pipeline::output::Mp4Output output;
    sk265::pipeline::output::OutputConfig cfg;
    cfg.outputPath = testPath;
    cfg.width = 1280;
    cfg.height = 720;
    cfg.fpsNum = 25;
    cfg.fpsDen = 1;
    cfg.bitDepth = 8;
    cfg.colorPrimaries = 1;
    cfg.transferCharacteristics = 1;
    cfg.matrixCoeffs = 1;

    REQUIRE(output.open(cfg));
    REQUIRE(output.isOpen());

    x265_nal* nals = nullptr;
    uint32_t nalCount = 0;
    int hBytes = api->encoder_headers(encoder.raw(), &nals, &nalCount);
    REQUIRE(hBytes > 0);
    REQUIRE(nalCount >= 3);

    REQUIRE(output.writeHeaders(nals, nalCount));

    // Encode 1 real frame to obtain real NALs
    auto pic_in = sk265::core::make_picture_handle(api);
    auto pic_out = sk265::core::make_picture_handle(api);
    api->picture_init(param.raw(), pic_in.raw());

    std::vector<uint8_t> yPlane(1280 * 720, 128);
    std::vector<uint8_t> uPlane(640 * 360, 128);
    std::vector<uint8_t> vPlane(640 * 360, 128);
    pic_in->planes[0] = yPlane.data();
    pic_in->stride[0] = 1280;
    pic_in->planes[1] = uPlane.data();
    pic_in->stride[1] = 640;
    pic_in->planes[2] = vPlane.data();
    pic_in->stride[2] = 640;
    pic_in->pts = 0;
    pic_in->bitDepth = 8;

    nals = nullptr;
    nalCount = 0;
    int fBytes = api->encoder_encode(encoder.raw(), &nals, &nalCount, pic_in.raw(), pic_out.raw());
    if (fBytes > 0 && nalCount > 0) {
        REQUIRE(output.writeFrame(nals, nalCount, *pic_out.raw()));
    }

    // Flush frame
    while (true) {
        nals = nullptr;
        nalCount = 0;
        int bytes = api->encoder_encode(encoder.raw(), &nals, &nalCount, nullptr, pic_out.raw());
        if (bytes <= 0) break;
        REQUIRE(output.writeFrame(nals, nalCount, *pic_out.raw()));
    }

    output.close(0, 0);
    REQUIRE_FALSE(output.isOpen());

    REQUIRE(std::filesystem::exists(testPath));
    REQUIRE(std::filesystem::file_size(testPath) > 0);

    // Clean up
    std::filesystem::remove(testPath);
}
