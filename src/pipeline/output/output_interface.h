#pragma once
#include <string>
#include <cstdint>
#include "core/x265_api.h"

namespace sk265::pipeline::output {

struct OutputConfig {
    std::string outputPath;
    int width{0};
    int height{0};
    int fpsNum{25};
    int fpsDen{1};
    int bitDepth{8};
    int colorSpace{0};

    // Aspect ratio & HDR / Dolby Vision / Colorimetry metadata reservation
    int sarWidth{0};
    int sarHeight{0};
    std::string doviRpuPath;
    std::string hdr10plusPath;
    int colorPrimaries{-1};
    int transferCharacteristics{-1};
    int matrixCoeffs{-1};
    bool fullRange{false};
};

class IOutput {
public:
    virtual ~IOutput() = default;
    virtual bool open(const OutputConfig& config) = 0;
    virtual bool writeHeaders(const x265_nal* nals, uint32_t nalCount) = 0;
    virtual bool writeFrame(const x265_nal* nals, uint32_t nalCount, const x265_picture& pic) = 0;
    virtual void close(int64_t largestPts = -1, int64_t secondLargestPts = -1) = 0;
};

} // namespace sk265::pipeline::output
