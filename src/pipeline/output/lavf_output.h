#pragma once
#include <string>
#include <vector>
#include <memory>
#include "pipeline/output/output_interface.h"

namespace sk265::pipeline::output {

class LavfOutput : public IOutput {
public:
    LavfOutput();
    ~LavfOutput() override;

    bool open(const OutputConfig& config) override;
    bool writeHeaders(const x265_nal* nals, uint32_t nalCount) override;
    bool writeFrame(const x265_nal* nals, uint32_t nalCount, const x265_picture& pic) override;
    void close(int64_t largestPts = -1, int64_t secondLargestPts = -1) override;

    bool isOpen() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sk265::pipeline::output
