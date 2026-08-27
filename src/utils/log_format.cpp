#include "utils/log_format.h"
#include <sstream>

namespace sk265::utils {

std::string LogFormat::formatCspName(int csp, int bitDepth) {
    std::string cspStr;
    switch (csp) {
        case 0: cspStr = "i400"; break;
        case 1: cspStr = "i420"; break;
        case 2: cspStr = "i422"; break;
        case 3: cspStr = "i444"; break;
        default: cspStr = "i420"; break;
    }
    return cspStr + "p" + std::to_string(bitDepth);
}

std::string LogFormat::formatFrameRange(int64_t totalFrames, int seekFrame, int frameCount) {
    if (totalFrames > 0) {
        int64_t actualEnd = (frameCount > 0) ? (seekFrame + frameCount - 1) : (totalFrames - 1);
        if (actualEnd >= totalFrames) {
            actualEnd = totalFrames - 1;
        }
        return "frames " + std::to_string(seekFrame) + " - " + std::to_string(actualEnd) + " of " + std::to_string(totalFrames);
    }

    if (frameCount > 0) {
        int64_t actualEnd = seekFrame + frameCount - 1;
        return "frames " + std::to_string(seekFrame) + " - " + std::to_string(actualEnd);
    }

    return "unknown frame count";
}

std::string LogFormat::formatInputBanner(
    std::string_view tag,
    const pipeline::input::InputInfo& info,
    int seekFrame,
    int frameCount,
    int sarWidth,
    int sarHeight
) {
    std::ostringstream oss;
    oss << tag << " [info]: "
        << info.width << "x" << info.height << " "
        << "fps " << info.fpsNum << "/" << info.fpsDen << " "
        << formatCspName(info.colorSpace, info.bitDepth);

    if (sarWidth > 0 && sarHeight > 0) {
        oss << " sar " << sarWidth << ":" << sarHeight;
    }

    oss << " " << formatFrameRange(info.totalFrames, seekFrame, frameCount);
    return oss.str();
}

std::string LogFormat::formatPresetTuneBanner(const std::string& preset, const std::string& tune) {
    std::ostringstream oss;
    oss << "x265 [info]: Using preset " << preset;
    if (!tune.empty()) {
        oss << " & tune " << tune;
    }
    return oss.str();
}

std::string LogFormat::formatOutputBanner(std::string_view tag, const std::string& outputPath) {
    return std::string(tag) + " [info]: output file: " + outputPath;
}

} // namespace sk265::utils
