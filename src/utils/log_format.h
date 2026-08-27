#pragma once
#include <string>
#include <string_view>
#include <cstdint>
#include "pipeline/input/input_interface.h"

namespace sk265::utils {

class LogFormat {
public:
    static std::string formatCspName(int csp, int bitDepth);
    static std::string formatFrameRange(int64_t totalFrames, int seekFrame, int frameCount);
    static std::string formatInputBanner(
        std::string_view tag,
        const pipeline::input::InputInfo& info,
        int seekFrame,
        int frameCount,
        int sarWidth = 0,
        int sarHeight = 0
    );
    static std::string formatPresetTuneBanner(const std::string& preset, const std::string& tune);
    static std::string formatOutputBanner(std::string_view tag, const std::string& outputPath);
};

} // namespace sk265::utils
