#pragma once
#include <string>
#include <memory>
#include <cstdio>
#include <cstdint>
#include "pipeline/output/output_interface.h"

namespace sk265::pipeline::output {

class GopOutput : public IOutput {
public:
    GopOutput();
    ~GopOutput() override;

    bool open(const OutputConfig& config) override;
    bool writeHeaders(const x265_nal* nals, uint32_t nalCount) override;
    bool writeFrame(const x265_nal* nals, uint32_t nalCount, const x265_picture& pic) override;
    void close(int64_t largestPts = -1, int64_t secondLargestPts = -1) override;

    // Accessors for testing & verification
    [[nodiscard]] const std::string& getGopPath() const noexcept { return m_gopPath; }
    [[nodiscard]] const std::string& getDirPrefix() const noexcept { return m_dirPrefix; }
    [[nodiscard]] const std::string& getFilenamePrefix() const noexcept { return m_filenamePrefix; }
    [[nodiscard]] int64_t getFrameOffset() const noexcept { return m_frameOffset; }
    [[nodiscard]] int64_t getCurrentFrame() const noexcept { return m_currentFrame; }

private:
    void parseOutputPath(const std::string& path);
    bool writeOptionsFile();
    FILE* openFileForWrite(const std::string& filename, bool retry = false);
    bool smartWrite(const void* data, size_t size, FILE* fp);

    OutputConfig m_config;
    std::string m_gopPath;
    std::string m_dirPrefix;
    std::string m_filenamePrefix;
    int64_t m_frameOffset{0};
    int64_t m_currentFrame{0};

    FILE* m_gopFp{nullptr};
    FILE* m_dataFp{nullptr};
    bool m_failed{false};
};

} // namespace sk265::pipeline::output
