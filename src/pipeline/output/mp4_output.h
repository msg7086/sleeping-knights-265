#pragma once
#include <string>
#include <vector>
#include <memory>
#include "pipeline/output/output_interface.h"
#include "core/x265_api.h"
#include "lsmash.h"

namespace sk265::pipeline::output {

class Mp4Output : public IOutput {
public:
    Mp4Output();
    ~Mp4Output() override;

    [[nodiscard]] std::string_view getTag() const noexcept override { return "mp4 "; }
    bool open(const OutputConfig& config) override;
    bool writeHeaders(const x265_nal* nals, uint32_t nalCount) override;
    bool writeFrame(const x265_nal* nals, uint32_t nalCount, const x265_picture& pic) override;
    void close(int64_t largestPts = -1, int64_t secondLargestPts = -1) override;

    bool isOpen() const noexcept { return root_ != nullptr && track_ != 0; }

private:
    void cleanup();
    void sign();

    OutputConfig config_{};
    lsmash_root_t* root_{nullptr};
    lsmash_file_parameters_t fileParam_{};
    lsmash_video_summary_t* summary_{nullptr};

    uint32_t movieTimescale_{0};
    uint32_t mediaTimescale_{0};
    uint32_t track_{0};
    uint32_t sampleEntry_{0};
    uint64_t timeInc_{1};

    int64_t startOffset_{0};
    uint64_t firstCts_{0};
    int64_t numFrames_{0};
    int64_t largestPts_{0};
    int64_t secondLargestPts_{0};

    std::vector<uint8_t> seiBuffer_;
};

} // namespace sk265::pipeline::output
