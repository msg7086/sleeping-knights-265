#include "pipeline/output/raw_output.h"
#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#endif

namespace sk265::pipeline::output {

RawOutput::RawOutput() = default;

RawOutput::~RawOutput() {
    close();
}

bool RawOutput::open(const std::string& path) {
    OutputConfig cfg;
    cfg.outputPath = path;
    return open(cfg);
}

bool RawOutput::open(const OutputConfig& config) {
    close();
    if (config.outputPath.empty()) return false;

    if (config.outputPath == "-") {
        isStdOut_ = true;
#ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
#endif
        return true;
    }

    fileStream_.open(config.outputPath, std::ios::binary);
    return fileStream_.is_open();
}

bool RawOutput::writeBytes(const uint8_t* data, size_t size) {
    if (!isOpen() || !data || size == 0) return false;

    if (isStdOut_) {
        std::cout.write(reinterpret_cast<const char*>(data), size);
        return std::cout.good();
    } else {
        fileStream_.write(reinterpret_cast<const char*>(data), size);
        return fileStream_.good();
    }
}

bool RawOutput::writeHeaders(const x265_nal* nals, uint32_t nalCount) {
    if (!isOpen() || !nals || nalCount == 0) return false;

    for (uint32_t i = 0; i < nalCount; ++i) {
        if (!writeBytes(nals[i].payload, nals[i].sizeBytes)) {
            return false;
        }
    }
    return true;
}

bool RawOutput::writeFrame(const x265_nal* nals, uint32_t nalCount, const x265_picture&) {
    if (!isOpen() || !nals || nalCount == 0) return false;

    for (uint32_t i = 0; i < nalCount; ++i) {
        if (!writeBytes(nals[i].payload, nals[i].sizeBytes)) {
            return false;
        }
    }
    return true;
}

void RawOutput::close(int64_t, int64_t) {
    if (isStdOut_) {
        std::cout.flush();
        isStdOut_ = false;
    } else if (fileStream_.is_open()) {
        fileStream_.flush();
        fileStream_.close();
    }
}

} // namespace sk265::pipeline::output
