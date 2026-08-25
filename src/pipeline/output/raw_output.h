#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include "pipeline/output/output_interface.h"

namespace sk265::pipeline::output {

class RawOutput : public IOutput {
public:
    RawOutput();
    ~RawOutput() override;

    bool open(const OutputConfig& config) override;
    bool open(const std::string& path);
    bool writeHeaders(const x265_nal* nals, uint32_t nalCount) override;
    bool writeFrame(const x265_nal* nals, uint32_t nalCount, const x265_picture& pic) override;
    void close(int64_t largestPts = -1, int64_t secondLargestPts = -1) override;

    bool writeBytes(const uint8_t* data, size_t size);
    bool isOpen() const noexcept { return isStdOut_ || fileStream_.is_open(); }

private:
    std::ofstream fileStream_;
    bool isStdOut_{false};
};

} // namespace sk265::pipeline::output
