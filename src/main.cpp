#include <iostream>
#include <chrono>
#include <memory>
#include <cstring>
#include "core/x265_handle.h"
#include "core/x265_router.h"
#include "config/cli_parser.h"
#include "pipeline/input/y4m_input.h"
#include "pipeline/input/avisynth_input.h"
#include "pipeline/output/raw_output.h"

int main(int argc, char** argv) {
    auto opts = sk265::config::CliParser::parse(argc, argv);

    if (opts.showHelp || (opts.inputPath.empty() && opts.outputPath.empty() && !opts.showVersion)) {
        sk265::config::CliParser::printHelp();
        return 0;
    }

    if (opts.showVersion) {
        std::cout << "sk265 version 0.1.0 (Decoupled x265 C-API Frontend)\n";
        return 0;
    }

    if (opts.inputPath.empty() || opts.outputPath.empty()) {
        std::cerr << "sk265 [error]: Input (-i) and Output (-o) parameters are required.\n";
        return 1;
    }

    // 1. Select and open input source
    std::unique_ptr<sk265::pipeline::input::IInput> input;
    if (opts.inputPath.size() > 4 && opts.inputPath.substr(opts.inputPath.size() - 4) == ".avs") {
        input = std::make_unique<sk265::pipeline::input::AviSynthInput>();
    } else {
        input = std::make_unique<sk265::pipeline::input::Y4mInput>();
    }

    if (!input->open(opts.inputPath)) {
        std::cerr << "sk265 [error]: Failed to open input source: " << opts.inputPath << "\n";
        return 1;
    }

    auto info = input->getInfo();
    int bitDepth = opts.bitDepth > 0 ? opts.bitDepth : info.bitDepth;

    // 2. Dispatch to symmetric core router
    const x265_api* api = sk265::core::CoreRouter::getApi(bitDepth);
    if (!api) {
        std::cerr << "sk265 [error]: Unsupported target bit depth: " << bitDepth << "\n";
        return 1;
    }

    // 3. Initialize and configure parameters via RAII handle
    auto param = sk265::core::make_param_handle(api);
    if (!param) {
        std::cerr << "sk265 [error]: Failed to allocate x265_param\n";
        return 1;
    }

    std::string preset = opts.encoderParams.count("preset") ? opts.encoderParams["preset"] : "medium";
    std::string tune = opts.encoderParams.count("tune") ? opts.encoderParams["tune"] : "";
    if (api->param_default_preset(param.raw(), preset.c_str(), tune.empty() ? nullptr : tune.c_str()) != 0) {
        std::cerr << "sk265 [error]: Invalid preset (" << preset << ") or tune (" << tune << ")\n";
        return 1;
    }

    param->sourceWidth = info.width;
    param->sourceHeight = info.height;
    param->fpsNum = info.fpsNum;
    param->fpsDenom = info.fpsDen;
    param->internalBitDepth = bitDepth;
    param->sourceBitDepth = info.bitDepth;
    param->internalCsp = X265_CSP_I420;

    // Forward pass-through parameters directly to x265_param_parse
    for (const auto& [name, value] : opts.encoderParams) {
        if (name == "preset" || name == "tune") continue;
        int ret = api->param_parse(param.raw(), name.c_str(), value.c_str());
        if (ret != 0) {
            std::cerr << "sk265 [error]: x265_param_parse failed for --" << name << " " << value << " (code: " << ret << ")\n";
            return 1;
        }
    }

    // 4. Open encoder instance
    auto encoder = sk265::core::make_encoder_handle(api, param.raw());
    if (!encoder) {
        std::cerr << "sk265 [error]: Failed to open x265 encoder instance\n";
        return 1;
    }

    // 5. Open output destination
    sk265::pipeline::output::RawOutput output;
    sk265::pipeline::output::OutputConfig outCfg;
    outCfg.outputPath = opts.outputPath;
    outCfg.width = info.width;
    outCfg.height = info.height;
    outCfg.fpsNum = info.fpsNum;
    outCfg.fpsDen = info.fpsDen;
    outCfg.bitDepth = bitDepth;

    if (!output.open(outCfg)) {
        std::cerr << "sk265 [error]: Failed to open output destination: " << opts.outputPath << "\n";
        return 1;
    }

    // 6. Write stream headers
    x265_nal* nals = nullptr;
    uint32_t nalCount = 0;
    int headerBytes = api->encoder_headers(encoder.raw(), &nals, &nalCount);
    if (headerBytes > 0 && nalCount > 0) {
        if (!output.writeHeaders(nals, nalCount)) {
            std::cerr << "sk265 [error]: Failed to write headers to output\n";
            return 1;
        }
    }

    std::cout << "sk265 [info]: Encoding " << info.width << "x" << info.height
              << " (" << bitDepth << "-bit) -> " << opts.outputPath << "\n";

    // 7. Frame processing loop
    auto pic_in = sk265::core::make_picture_handle(api);
    auto pic_out = sk265::core::make_picture_handle(api);

    int framesEncoded = 0;
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        if (opts.frameCount > 0 && framesEncoded >= opts.frameCount) {
            break;
        }

        auto frameOpt = input->readFrame();
        if (!frameOpt.has_value()) {
            break; // EOF
        }

        auto& frame = *frameOpt;
        if (frame.pts < opts.seekFrame) {
            continue; // Skip frames before seek offset
        }

        api->picture_init(param.raw(), pic_in.raw());
        pic_in->planes[0] = const_cast<uint8_t*>(frame.planes[0].data());
        pic_in->stride[0] = static_cast<int>(frame.strides[0]);
        pic_in->planes[1] = const_cast<uint8_t*>(frame.planes[1].data());
        pic_in->stride[1] = static_cast<int>(frame.strides[1]);
        pic_in->planes[2] = const_cast<uint8_t*>(frame.planes[2].data());
        pic_in->stride[2] = static_cast<int>(frame.strides[2]);
        pic_in->pts = frame.pts;
        pic_in->bitDepth = bitDepth;

        nals = nullptr;
        nalCount = 0;
        int bytes = api->encoder_encode(encoder.raw(), &nals, &nalCount, pic_in.raw(), pic_out.raw());
        if (bytes < 0) {
            std::cerr << "sk265 [error]: Fatal error during frame encode\n";
            break;
        }

        if (bytes > 0 && nalCount > 0) {
            output.writeFrame(nals, nalCount, *pic_out.raw());
        }

        framesEncoded++;
    }

    // 8. Flush encoder buffer
    while (true) {
        nals = nullptr;
        nalCount = 0;
        int bytes = api->encoder_encode(encoder.raw(), &nals, &nalCount, nullptr, pic_out.raw());
        if (bytes <= 0) {
            break;
        }
        output.writeFrame(nals, nalCount, *pic_out.raw());
    }

    output.close();

    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime).count();
    double fps = elapsedMs > 0 ? (framesEncoded * 1000.0 / elapsedMs) : 0.0;

    std::cout << "sk265 [info]: Encoded " << framesEncoded << " frames in "
              << (elapsedMs / 1000.0) << "s (" << fps << " fps)\n";

    return 0;
}
