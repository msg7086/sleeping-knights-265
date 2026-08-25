#pragma once
#include <string>
#include <unordered_map>

namespace sk265::config {

struct CliOptions {
    std::string inputPath;
    std::string outputPath;
    int bitDepth{8};
    int frameCount{0};
    int seekFrame{0};
    bool showHelp{false};
    bool showVersion{false};

    // Standardized encoder parameters for pass-through to x265_param_parse
    std::unordered_map<std::string, std::string> encoderParams;
};

} // namespace sk265::config
