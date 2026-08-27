#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include "pipeline/output/output_interface.h"
#include "pipeline/bounded_queue.h"

namespace sk265::pipeline::output {

struct OutputPacket {
    enum class Kind {
        Headers,
        Frame,
        Close
    };

    Kind kind{Kind::Frame};
    std::vector<uint8_t> payloadData;
    std::vector<size_t> nalOffsets;
    std::vector<uint32_t> nalSizes;
    std::vector<int> nalTypes;

    int sliceType{0};
    int64_t pts{0};
    int64_t dts{0};
    int poc{0};
    int bitDepth{8};
    int colorSpace{1};

    int64_t largestPts{-1};
    int64_t secondLargestPts{-1};
};

class AsyncOutput : public IOutput {
public:
    explicit AsyncOutput(std::unique_ptr<IOutput> underlying, size_t queueCapacity = 128);
    ~AsyncOutput() override;

    bool open(const OutputConfig& config) override;
    bool writeHeaders(const x265_nal* nals, uint32_t nalCount) override;
    bool writeFrame(const x265_nal* nals, uint32_t nalCount, const x265_picture& pic) override;
    void close(int64_t largestPts = -1, int64_t secondLargestPts = -1) override;

    [[nodiscard]] bool hasFailed() const noexcept { return failed_.load(); }
    [[nodiscard]] size_t queueSize() const noexcept { return queue_.size(); }
    [[nodiscard]] IOutput* getUnderlying() const noexcept { return underlying_.get(); }

private:
    void workerLoop(std::stop_token st);

    std::unique_ptr<IOutput> underlying_;
    BoundedQueue<OutputPacket> queue_;
    std::atomic<bool> failed_{false};
    std::atomic<bool> closed_{false};
    std::atomic<bool> underlyingClosed_{false};
    std::jthread workerThread_;
};

} // namespace sk265::pipeline::output
