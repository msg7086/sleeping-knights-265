#include "pipeline/output/async_output.h"
#include <algorithm>

namespace sk265::pipeline::output {

AsyncOutput::AsyncOutput(std::unique_ptr<IOutput> underlying, size_t queueCapacity)
    : underlying_(std::move(underlying)), queue_(queueCapacity > 0 ? queueCapacity : 128) {}

AsyncOutput::~AsyncOutput() {
    close();
}

bool AsyncOutput::open(const OutputConfig& config) {
    if (!underlying_ || !underlying_->open(config)) {
        return false;
    }

    failed_.store(false);
    closed_.store(false);
    underlyingClosed_.store(false);

    workerThread_ = std::jthread([this](std::stop_token st) {
        workerLoop(st);
    });

    return true;
}

bool AsyncOutput::writeHeaders(const x265_nal* nals, uint32_t nalCount) {
    if (failed_.load() || closed_.load() || !nals || nalCount == 0) {
        return false;
    }

    OutputPacket packet;
    packet.kind = OutputPacket::Kind::Headers;

    size_t totalBytes = 0;
    for (uint32_t i = 0; i < nalCount; ++i) {
        totalBytes += nals[i].sizeBytes;
    }
    packet.payloadData.reserve(totalBytes);
    packet.nalOffsets.reserve(nalCount);
    packet.nalSizes.reserve(nalCount);
    packet.nalTypes.reserve(nalCount);

    for (uint32_t i = 0; i < nalCount; ++i) {
        packet.nalOffsets.push_back(packet.payloadData.size());
        packet.nalSizes.push_back(nals[i].sizeBytes);
        packet.nalTypes.push_back(nals[i].type);
        packet.payloadData.insert(packet.payloadData.end(),
                                  nals[i].payload,
                                  nals[i].payload + nals[i].sizeBytes);
    }

    return queue_.push(std::move(packet));
}

bool AsyncOutput::writeFrame(const x265_nal* nals, uint32_t nalCount, const x265_picture& pic) {
    if (failed_.load() || closed_.load()) {
        return false;
    }

    OutputPacket packet;
    packet.kind = OutputPacket::Kind::Frame;
    packet.sliceType = pic.sliceType;
    packet.pts = pic.pts;
    packet.dts = pic.dts;
    packet.poc = pic.poc;
    packet.bitDepth = pic.bitDepth;
    packet.colorSpace = pic.colorSpace;

    if (nals && nalCount > 0) {
        size_t totalBytes = 0;
        for (uint32_t i = 0; i < nalCount; ++i) {
            totalBytes += nals[i].sizeBytes;
        }
        packet.payloadData.reserve(totalBytes);
        packet.nalOffsets.reserve(nalCount);
        packet.nalSizes.reserve(nalCount);
        packet.nalTypes.reserve(nalCount);

        for (uint32_t i = 0; i < nalCount; ++i) {
            packet.nalOffsets.push_back(packet.payloadData.size());
            packet.nalSizes.push_back(nals[i].sizeBytes);
            packet.nalTypes.push_back(nals[i].type);
            packet.payloadData.insert(packet.payloadData.end(),
                                      nals[i].payload,
                                      nals[i].payload + nals[i].sizeBytes);
        }
    }

    return queue_.push(std::move(packet));
}

void AsyncOutput::close(int64_t largestPts, int64_t secondLargestPts) {
    if (closed_.exchange(true)) {
        return;
    }

    OutputPacket packet;
    packet.kind = OutputPacket::Kind::Close;
    packet.largestPts = largestPts;
    packet.secondLargestPts = secondLargestPts;
    queue_.push(std::move(packet));
    queue_.close();

    if (workerThread_.joinable()) {
        workerThread_.join();
    }

    if (!underlyingClosed_.exchange(true) && underlying_) {
        underlying_->close(largestPts, secondLargestPts);
    }
}

void AsyncOutput::workerLoop(std::stop_token /*st*/) {
    while (true) {
        auto packetOpt = queue_.pop();
        if (!packetOpt.has_value()) {
            break;
        }

        auto& packet = *packetOpt;
        if (packet.kind == OutputPacket::Kind::Headers) {
            std::vector<x265_nal> nals(packet.nalSizes.size());
            for (size_t i = 0; i < nals.size(); ++i) {
                nals[i].payload = packet.payloadData.data() + packet.nalOffsets[i];
                nals[i].sizeBytes = packet.nalSizes[i];
                nals[i].type = packet.nalTypes[i];
            }
            if (!underlying_->writeHeaders(nals.data(), static_cast<uint32_t>(nals.size()))) {
                failed_.store(true);
            }
        } else if (packet.kind == OutputPacket::Kind::Frame) {
            std::vector<x265_nal> nals(packet.nalSizes.size());
            for (size_t i = 0; i < nals.size(); ++i) {
                nals[i].payload = packet.payloadData.data() + packet.nalOffsets[i];
                nals[i].sizeBytes = packet.nalSizes[i];
                nals[i].type = packet.nalTypes[i];
            }
            x265_picture pic{};
            pic.sliceType = packet.sliceType;
            pic.pts = packet.pts;
            pic.dts = packet.dts;
            pic.poc = packet.poc;
            pic.bitDepth = packet.bitDepth;
            pic.colorSpace = packet.colorSpace;

            if (!underlying_->writeFrame(nals.data(), static_cast<uint32_t>(nals.size()), pic)) {
                failed_.store(true);
            }
        } else if (packet.kind == OutputPacket::Kind::Close) {
            if (!underlyingClosed_.exchange(true) && underlying_) {
                underlying_->close(packet.largestPts, packet.secondLargestPts);
            }
            break;
        }
    }
}

} // namespace sk265::pipeline::output
