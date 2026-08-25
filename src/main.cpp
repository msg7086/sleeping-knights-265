#include <iostream>
#include <chrono>
#include <memory>
#include <thread>
#include <cstring>
#include "core/x265_handle.h"
#include "core/x265_router.h"
#include "core/tuning_preset.h"
#include "config/cli_parser.h"
#include "config/cascading_config.h"
#include "pipeline/input/y4m_input.h"
#include "pipeline/input/avisynth_input.h"
#include "pipeline/input/vapoursynth_input.h"
#include "pipeline/input/lavf_input.h"
#include "pipeline/output/output_factory.h"
#include "pipeline/bounded_queue.h"
#include "utils/progress.h"
#include "utils/signal_handler.h"
#include "version.h"

static void printBanner() {
    std::cerr << "sk265[info]: version " << SK265_VERSION << "\n";
}

int main(int argc, char** argv) {
    sk265::utils::installSignalHandler();
    auto opts = sk265::config::CascadingConfig::resolve(argc, argv);

    if (opts.showHelp || (opts.inputPath.empty() && opts.outputPath.empty() && !opts.showVersion)) {
        sk265::config::CliParser::printHelp();
        return 0;
    }

    if (opts.showVersion) {
        std::cout << "sk265 version " << SK265_VERSION << "\n";
        return 0;
    }

    printBanner();

    if (opts.inputPath.empty() || opts.outputPath.empty()) {
        std::cerr << "sk265[error]: Input (-i) and Output (-o) parameters are required.\n";
        return 1;
    }

    // 1. Select and open input source
    std::unique_ptr<sk265::pipeline::input::IInput> input;
    auto hasExt = [](const std::string& path, const std::string& ext) {
        if (path.size() < ext.size()) return false;
        return std::equal(ext.rbegin(), ext.rend(), path.rbegin(),
                          [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); });
    };

    if (hasExt(opts.inputPath, ".avs")) {
        auto avs = std::make_unique<sk265::pipeline::input::AviSynthInput>();
        if (!opts.avsLibPath.empty()) {
            avs->setCustomLibraryPath(opts.avsLibPath);
        }
        if (opts.seekFrame > 0) {
            avs->setSeekFrame(opts.seekFrame);
        }
        input = std::move(avs);
    } else if (hasExt(opts.inputPath, ".vpy")) {
        auto vpy = std::make_unique<sk265::pipeline::input::VapourSynthInput>();
        if (!opts.vpyLibPath.empty()) {
            vpy->setCustomLibraryPath(opts.vpyLibPath);
        }
        if (opts.seekFrame > 0) {
            vpy->setSeekFrame(opts.seekFrame);
        }
        input = std::move(vpy);
    } else if (hasExt(opts.inputPath, ".y4m") || opts.inputPath == "-") {
        input = std::make_unique<sk265::pipeline::input::Y4mInput>();
    } else {
        auto lavf = std::make_unique<sk265::pipeline::input::LavfInput>();
        if (opts.seekFrame > 0) {
            lavf->setSeekFrame(opts.seekFrame);
        }
        input = std::move(lavf);
    }

    if (!input->open(opts.inputPath)) {
        std::cerr << "sk265[error]: Failed to open input source: " << opts.inputPath << "\n";
        return 1;
    }

    auto info = input->getInfo();
    int bitDepth = opts.bitDepth > 0 ? opts.bitDepth : info.bitDepth;

    // 2. Dispatch to symmetric core router
    const x265_api* api = sk265::core::CoreRouter::getApi(bitDepth);
    if (!api) {
        std::cerr << "sk265[error]: Unsupported target bit depth: " << bitDepth << "\n";
        return 1;
    }

    // 3. Initialize and configure parameters via RAII handle
    auto param = sk265::core::make_param_handle(api);
    if (!param) {
        std::cerr << "sk265[error]: Failed to allocate x265_param\n";
        return 1;
    }

    std::string preset = opts.encoderParams.count("preset") ? opts.encoderParams["preset"] : "medium";
    std::string tune = opts.encoderParams.count("tune") ? opts.encoderParams["tune"] : "";
    bool isCustomTune = sk265::core::TuningPreset::isCustomTune(tune);
    const char* x265Tune = isCustomTune ? nullptr : (tune.empty() ? nullptr : tune.c_str());

    if (api->param_default_preset(param.raw(), preset.c_str(), x265Tune) != 0) {
        std::cerr << "sk265[error]: Invalid preset (" << preset << ") or tune (" << tune << ")\n";
        return 1;
    }

    if (isCustomTune) {
        sk265::core::TuningPreset::apply(param.raw(), tune);
    }

    param->sourceWidth = info.width;
    param->sourceHeight = info.height;
    param->fpsNum = info.fpsNum;
    param->fpsDenom = info.fpsDen;
    param->internalBitDepth = bitDepth;
    param->sourceBitDepth = info.bitDepth;
    param->internalCsp = info.colorSpace;

    auto muxerResult = sk265::pipeline::output::OutputFactory::create(opts.muxer, opts.outputPath);
    if (!muxerResult.has_value()) {
        std::cerr << "sk265[error]: " << muxerResult.error() << "\n";
        return 1;
    }

    auto muxerInstance = std::move(muxerResult.value());
    param->bAnnexB = muxerInstance.bAnnexB;
    param->bRepeatHeaders = muxerInstance.bRepeatHeaders;

    // Forward pass-through parameters directly to x265_param_parse
    for (const auto& [name, value] : opts.encoderParams) {
        if (name == "preset" || name == "tune") continue;
        int ret = api->param_parse(param.raw(), name.c_str(), value.c_str());
        if (ret != 0) {
            std::cerr << "sk265[error]: x265_param_parse failed for --" << name << " " << value << " (code: " << ret << ")\n";
            return 1;
        }
    }

    // 4. Open encoder instance
    auto encoder = sk265::core::make_encoder_handle(api, param.raw());
    if (!encoder) {
        std::cerr << "sk265[error]: Failed to open x265 encoder instance\n";
        return 1;
    }

    // 5. Open output destination
    auto output = std::move(muxerInstance.output);

    sk265::pipeline::output::OutputConfig outCfg;
    outCfg.outputPath = opts.outputPath;
    outCfg.width = info.width;
    outCfg.height = info.height;
    outCfg.fpsNum = info.fpsNum;
    outCfg.fpsDen = info.fpsDen;
    outCfg.bitDepth = bitDepth;
    outCfg.sarWidth = param->vui.sarWidth;
    outCfg.sarHeight = param->vui.sarHeight;
    outCfg.colorPrimaries = param->vui.colorPrimaries;
    outCfg.transferCharacteristics = param->vui.transferCharacteristics;
    outCfg.matrixCoeffs = param->vui.matrixCoeffs;
    outCfg.fullRange = (param->vui.bEnableVideoFullRangeFlag == 1);

    if (!output->open(outCfg)) {
        std::cerr << "sk265[error]: Failed to open output destination: " << opts.outputPath << "\n";
        return 1;
    }

    // 6. Write stream headers
    x265_nal* nals = nullptr;
    uint32_t nalCount = 0;
    int headerBytes = api->encoder_headers(encoder.raw(), &nals, &nalCount);
    if (headerBytes > 0 && nalCount > 0) {
        if (!output->writeHeaders(nals, nalCount)) {
            std::cerr << "sk265[error]: Failed to write headers to output\n";
            return 1;
        }
    }

    size_t queueCapacity = opts.queueConfig.resolveCapacity(info.width, info.height, bitDepth, info.colorSpace);
    std::cerr << "sk265[info]: Encoding " << info.width << "x" << info.height
              << " (" << bitDepth << "-bit) -> " << opts.outputPath
              << " (Prefetch queue: " << queueCapacity << " frames)\n";

    // 7. Initialize asynchronous bounded prefetch queue and producer thread
    sk265::pipeline::BoundedQueue<sk265::pipeline::VideoFrame> queue(queueCapacity);

    std::jthread producer([&](std::stop_token st) {
        int64_t framesRead = 0;
        while (!st.stop_requested() && !sk265::utils::isInterrupted()) {
            if (opts.frameCount > 0 && framesRead >= opts.frameCount + opts.seekFrame) {
                break;
            }

            auto frameOpt = input->readFrame();
            if (!frameOpt.has_value()) {
                break; // EOF
            }

            if (frameOpt->pts < opts.seekFrame) {
                continue; // Skip frames before seek offset
            }

            framesRead++;
            if (!queue.push(std::move(*frameOpt), st)) {
                break;
            }
        }
        queue.close();
    });

    // 8. Main encoding consumer loop
    auto pic_in = sk265::core::make_picture_handle(api);
    auto pic_out = sk265::core::make_picture_handle(api);

    int64_t expectedFrames = opts.frameCount > 0 ? opts.frameCount : info.totalFrames;
    sk265::utils::ConsoleProgress progress(expectedFrames, info.fpsNum, info.fpsDen, opts.bProgress, opts.bStylish);

    int framesEncoded = 0;
    uint64_t totalBytesEncoded = 0;
    int64_t largestPts = -1;
    int64_t secondLargestPts = -1;
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        if (sk265::utils::isInterrupted()) {
            std::cerr << "\nsk265[info]: Interrupt received, flushing remaining frames...\n";
            sk265::utils::requestStop();
            queue.close();
            break;
        }

        if (opts.frameCount > 0 && framesEncoded >= opts.frameCount) {
            sk265::utils::requestStop();
            queue.close();
            break;
        }

        auto frameOpt = queue.pop();
        if (!frameOpt.has_value()) {
            break; // EOF or queue closed
        }

        auto& frame = *frameOpt;
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
            std::cerr << "sk265[error]: Fatal error during frame encode\n";
            break;
        }

        if (bytes > 0 && nalCount > 0) {
            if (pic_out->pts > largestPts) {
                secondLargestPts = largestPts;
                largestPts = pic_out->pts;
            }
            output->writeFrame(nals, nalCount, *pic_out.raw());
            totalBytesEncoded += bytes;
        }

        framesEncoded++;
        progress.update(framesEncoded, totalBytesEncoded);
    }

    // 9. Flush encoder buffer
    while (true) {
        nals = nullptr;
        nalCount = 0;
        int bytes = api->encoder_encode(encoder.raw(), &nals, &nalCount, nullptr, pic_out.raw());
        if (bytes <= 0) {
            break;
        }
        if (pic_out->pts > largestPts) {
            secondLargestPts = largestPts;
            largestPts = pic_out->pts;
        }
        output->writeFrame(nals, nalCount, *pic_out.raw());
        totalBytesEncoded += bytes;
        progress.update(framesEncoded, totalBytesEncoded);
    }

    progress.finish(framesEncoded, totalBytesEncoded);
    output->close(largestPts, secondLargestPts);

    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime).count();
    double fps = elapsedMs > 0 ? (framesEncoded * 1000.0 / elapsedMs) : 0.0;

    std::cerr << "sk265[info]: Encoded " << framesEncoded << " frames in "
              << (elapsedMs / 1000.0) << "s (" << fps << " fps)\n";

    return sk265::utils::isInterrupted() ? 2 : 0;
}
