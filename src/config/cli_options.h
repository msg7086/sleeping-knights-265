#pragma once
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <cstdint>

namespace sk265::config {

struct QueueSizeConfig {
    enum class Mode { DefaultAuto, Frames, MemoryBytes };
    Mode mode{Mode::DefaultAuto};
    int64_t rawValue{0};

    static QueueSizeConfig parse(const std::string& str) {
        QueueSizeConfig cfg;
        if (str.empty()) return cfg;

        std::string s = str;
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();

        std::string upper;
        for (char c : s) upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        try {
            if (upper.ends_with("GB") || upper.ends_with("G")) {
                cfg.mode = Mode::MemoryBytes;
                size_t numLen = upper.ends_with("GB") ? upper.size() - 2 : upper.size() - 1;
                double val = std::stod(upper.substr(0, numLen));
                cfg.rawValue = static_cast<int64_t>(val * 1024 * 1024 * 1024);
            } else if (upper.ends_with("MB") || upper.ends_with("M")) {
                cfg.mode = Mode::MemoryBytes;
                size_t numLen = upper.ends_with("MB") ? upper.size() - 2 : upper.size() - 1;
                double val = std::stod(upper.substr(0, numLen));
                cfg.rawValue = static_cast<int64_t>(val * 1024 * 1024);
            } else if (upper.ends_with("KB") || upper.ends_with("K")) {
                cfg.mode = Mode::MemoryBytes;
                size_t numLen = upper.ends_with("KB") ? upper.size() - 2 : upper.size() - 1;
                double val = std::stod(upper.substr(0, numLen));
                cfg.rawValue = static_cast<int64_t>(val * 1024);
            } else {
                cfg.mode = Mode::Frames;
                cfg.rawValue = std::stoll(s);
            }
        } catch (...) {
            cfg.mode = Mode::DefaultAuto;
            cfg.rawValue = 0;
        }
        return cfg;
    }

    size_t resolveCapacity(int width, int height, int bitDepth, int colorSpace = 0) const {
        if (mode == Mode::DefaultAuto) {
            uint64_t pixels = static_cast<uint64_t>(width) * height;
            if (pixels >= 3840ULL * 2160ULL) {
                return 4;  // 4K UHD / 8K -> 4 frames
            } else if (pixels >= 2560ULL * 1440ULL) {
                return 8;  // 1440p / 2K -> 8 frames
            } else if (pixels >= 1920ULL * 1080ULL) {
                return 16; // 1080p FHD -> 16 frames
            } else {
                return 32; // 720p / SD & Below -> 32 frames
            }
        }

        if (mode == Mode::Frames) {
            return std::clamp<size_t>(static_cast<size_t>(std::max<int64_t>(0, rawValue)), 2, 64);
        }

        int bytesPerSample = (bitDepth > 8) ? 2 : 1;
        double chromaMult = (colorSpace == 2) ? 3.0 : (colorSpace == 1 ? 2.0 : 1.5);
        size_t frameBytes = static_cast<size_t>(width * height * bytesPerSample * chromaMult);
        if (frameBytes == 0) return 16;

        size_t frames = static_cast<size_t>(rawValue / frameBytes);
        return std::clamp<size_t>(frames, 2, 64);
    }
};

struct CliOptions {
    std::string inputPath;
    std::string outputPath;
    int bitDepth{8};
    int frameCount{0};
    int seekFrame{0};
    bool showHelp{false};
    bool showVersion{false};
    bool bProgress{true};
    bool bStylish{false};

    std::string doviRpuPath;
    std::string qpfilePath;
    std::string avsLibPath;
    std::string vpyLibPath;
    std::string muxer{"auto"}; // "auto", "lsmash", "lavf", "raw"
    QueueSizeConfig queueConfig;

    // Standardized encoder parameters for pass-through to x265_param_parse
    std::unordered_map<std::string, std::string> encoderParams;
};

} // namespace sk265::config
