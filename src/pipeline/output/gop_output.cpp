#include "pipeline/output/gop_output.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <cerrno>
#include <cstring>
#include <thread>
#include <chrono>

namespace sk265::pipeline::output {

GopOutput::GopOutput() = default;

GopOutput::~GopOutput() {
    close();
}

void GopOutput::parseOutputPath(const std::string& path) {
    auto qPos = path.find('?');
    std::string rawPath;
    if (qPos == std::string::npos) {
        rawPath = path;
    } else {
        rawPath = path.substr(0, qPos);
        std::string queryString = path.substr(qPos + 1);
        std::stringstream ss(queryString);
        std::string item;
        while (std::getline(ss, item, '&')) {
            if (item.empty()) continue;
            auto eqPos = item.find('=');
            std::string key = (eqPos == std::string::npos) ? item : item.substr(0, eqPos);
            std::string val = (eqPos == std::string::npos) ? "1" : item.substr(eqPos + 1);
            if (key == "start") {
                try {
                    m_frameOffset = std::stoll(val);
                } catch (...) {
                    m_frameOffset = 0;
                }
            }
        }
    }

    auto slashPos = rawPath.find_last_of("/\\");
    std::string filenameOnly;
    if (slashPos != std::string::npos) {
        m_dirPrefix = rawPath.substr(0, slashPos + 1);
        filenameOnly = rawPath.substr(slashPos + 1);
    } else {
        m_dirPrefix = "";
        filenameOnly = rawPath;
    }

    auto dotPos = filenameOnly.rfind('.');
    if (dotPos != std::string::npos) {
        m_filenamePrefix = filenameOnly.substr(0, dotPos);
    } else {
        m_filenamePrefix = filenameOnly;
    }
    m_gopPath = rawPath;
}

FILE* GopOutput::openFileForWrite(const std::string& filename, bool retry) {
    int attempts = retry ? 3 : 1;
    while (attempts-- > 0) {
#if defined(_WIN32)
        std::filesystem::path p(reinterpret_cast<const char8_t*>(filename.c_str()));
        FILE* fp = _wfopen(p.c_str(), L"wb");
#else
        FILE* fp = std::fopen(filename.c_str(), "wb");
#endif
        if (fp != nullptr) {
            return fp;
        }
        if (retry && attempts > 0) {
            std::cerr << "sk265[warning]: unable to open file " << filename
                      << " for writing, error " << errno << " " << std::strerror(errno)
                      << ", retrying...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    m_failed = true;
    std::cerr << "sk265[error]: unable to open file " << filename
              << " for writing, error " << errno << " " << std::strerror(errno) << "\n";
    return nullptr;
}

bool GopOutput::smartWrite(const void* data, size_t size, FILE* fp) {
    if (!fp || size == 0) return true;
    size_t written = std::fwrite(data, 1, size, fp);
    if (written == size) {
        return true;
    }
    m_failed = true;
    std::cerr << "sk265[error]: write error on output file (written: " << written
              << ", expected: " << size << ", error: " << std::strerror(errno) << ")\n";
    return false;
}

bool GopOutput::writeOptionsFile() {
    std::string optFilename = m_dirPrefix + m_filenamePrefix + ".options";
    FILE* optFp = openFileForWrite(optFilename, false);
    if (!optFp) return false;

    int matrixIdx = (m_config.matrixCoeffs >= 0) ? m_config.matrixCoeffs : 2;
    int fullRangeVal = m_config.fullRange ? 1 : 0;
    int tbNum = (m_config.timebaseNum > 0) ? m_config.timebaseNum : m_config.fpsDen;
    int tbDen = (m_config.timebaseDen > 0) ? m_config.timebaseDen : m_config.fpsNum;

    std::fprintf(optFp, "b-frames %d\n", m_config.bframes);
    std::fprintf(optFp, "b-pyramid %d\n", m_config.bBPyramid);
    std::fprintf(optFp, "input-timebase-num %d\n", tbNum);
    std::fprintf(optFp, "input-timebase-den %d\n", tbDen);
    std::fprintf(optFp, "output-fps-num %u\n", static_cast<unsigned int>(m_config.fpsNum));
    std::fprintf(optFp, "output-fps-den %u\n", static_cast<unsigned int>(m_config.fpsDen));
    std::fprintf(optFp, "source-width %d\n", m_config.width);
    std::fprintf(optFp, "source-height %d\n", m_config.height);
    std::fprintf(optFp, "sar-width %d\n", m_config.sarWidth);
    std::fprintf(optFp, "sar-height %d\n", m_config.sarHeight);
    std::fprintf(optFp, "primaries-index %d\n", m_config.colorPrimaries);
    std::fprintf(optFp, "transfer-index %d\n", m_config.transferCharacteristics);
    std::fprintf(optFp, "matrix-index %d\n", matrixIdx);
    std::fprintf(optFp, "full-range %d\n", fullRangeVal);

    std::fclose(optFp);
    return true;
}

bool GopOutput::open(const OutputConfig& config) {
    m_config = config;
    m_currentFrame = 0;
    m_failed = false;

    parseOutputPath(config.outputPath);

    m_gopFp = openFileForWrite(m_gopPath, false);
    if (!m_gopFp) return false;

    if (!writeOptionsFile()) {
        close();
        return false;
    }

    std::fprintf(m_gopFp, "#options %s.options\n", m_filenamePrefix.c_str());
    std::fflush(m_gopFp);
    return true;
}

bool GopOutput::writeHeaders(const x265_nal* nals, uint32_t nalCount) {
    if (!m_gopFp) return false;

    std::string hdrFilename = m_dirPrefix + m_filenamePrefix + ".headers";
    FILE* hdrFp = openFileForWrite(hdrFilename, false);
    if (!hdrFp) return false;

    std::fprintf(m_gopFp, "#headers %s.headers\n", m_filenamePrefix.c_str());
    std::fflush(m_gopFp);

    for (uint32_t i = 0; i < nalCount; ++i) {
        if (!smartWrite(nals[i].payload, nals[i].sizeBytes, hdrFp)) {
            std::fclose(hdrFp);
            return false;
        }
    }

    std::fclose(hdrFp);
    return true;
}

bool GopOutput::writeFrame(const x265_nal* nals, uint32_t nalCount, const x265_picture& pic) {
    if (!m_gopFp) return false;

    const bool isKeyframe = (pic.sliceType == X265_TYPE_IDR) || (m_currentFrame == 0 && !m_dataFp);

    if (isKeyframe) {
        if (m_dataFp) {
            std::fclose(m_dataFp);
            m_dataFp = nullptr;
        }

        std::ostringstream ss;
        ss << m_filenamePrefix << "-" << std::setfill('0') << std::setw(6)
           << (m_currentFrame + m_frameOffset) << ".hevc-gop-data";
        std::string chunkFilename = ss.str();
        std::string fullChunkPath = m_dirPrefix + chunkFilename;

        m_dataFp = openFileForWrite(fullChunkPath, m_currentFrame > 0);
        if (!m_dataFp) return false;

        std::fprintf(m_gopFp, "%s\n", chunkFilename.c_str());
        std::fflush(m_gopFp);
    }

    if (!m_dataFp) return false;

    // 1. Write timestamp header: 4-byte size (16) + PTS + DTS
    const uint8_t tsLenBytes[4] = {0x00, 0x00, 0x00, 0x10};
    if (!smartWrite(tsLenBytes, 4, m_dataFp)) return false;
    if (!smartWrite(&pic.pts, sizeof(int64_t), m_dataFp)) return false;
    if (!smartWrite(&pic.dts, sizeof(int64_t), m_dataFp)) return false;

    // 2. Write all NAL units in the frame
    for (uint32_t i = 0; i < nalCount; ++i) {
        if (!smartWrite(nals[i].payload, nals[i].sizeBytes, m_dataFp)) {
            return false;
        }
    }

    m_currentFrame++;
    return true;
}

void GopOutput::close(int64_t /*largestPts*/, int64_t /*secondLargestPts*/) {
    if (m_dataFp) {
        std::fclose(m_dataFp);
        m_dataFp = nullptr;
    }
    if (m_gopFp) {
        std::fprintf(m_gopFp, "# %lld frames written, last frame %lld\n",
                     static_cast<long long>(m_currentFrame),
                     static_cast<long long>(m_currentFrame > 0 ? (m_currentFrame + m_frameOffset) : m_frameOffset));
        std::fclose(m_gopFp);
        m_gopFp = nullptr;
    }
}

} // namespace sk265::pipeline::output
