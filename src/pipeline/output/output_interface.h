#pragma once
#include <string>
#include <cstdint>

#ifndef X265_H
struct x265_nal {
    uint32_t type;
    uint32_t sizeBytes;
    uint8_t* payload;
};
struct x265_picture;
#endif

namespace sk265::pipeline::output {

struct OutputConfig {
    std::string outputPath;
    int width{0};
    int height{0};
    int fpsNum{25};
    int fpsDen{1};
    int bitDepth{8};
    int colorSpace{0};

    // HDR / Dolby Vision / Colorimetry metadata reservation
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
