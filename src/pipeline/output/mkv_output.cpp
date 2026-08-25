#include "pipeline/output/mkv_output.h"
#include <iostream>
#include <cstring>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

namespace sk265::pipeline::output {

struct MkvOutput::Impl {
    AVFormatContext* formatCtx{nullptr};
    AVStream* videoStream{nullptr};
    OutputConfig config{};
    bool headerWritten{false};
    int64_t startOffset{0};
    int64_t frameCount{0};

    ~Impl() {
        cleanup();
    }

    void cleanup() {
        if (formatCtx) {
            if (headerWritten && formatCtx->pb) {
                av_write_trailer(formatCtx);
            }
            if (formatCtx->pb) {
                avio_closep(&formatCtx->pb);
            }
            avformat_free_context(formatCtx);
            formatCtx = nullptr;
        }
        videoStream = nullptr;
        headerWritten = false;
        frameCount = 0;
    }
};

MkvOutput::MkvOutput() : impl_(std::make_unique<Impl>()) {}
MkvOutput::~MkvOutput() = default;

bool MkvOutput::isOpen() const noexcept {
    return impl_->formatCtx != nullptr;
}

bool MkvOutput::open(const OutputConfig& config) {
    if (config.outputPath.empty()) return false;
    impl_->cleanup();
    impl_->config = config;

    int ret = avformat_alloc_output_context2(&impl_->formatCtx, nullptr, "matroska", config.outputPath.c_str());
    if (ret < 0 || !impl_->formatCtx) {
        impl_->cleanup();
        return false;
    }

    impl_->videoStream = avformat_new_stream(impl_->formatCtx, nullptr);
    if (!impl_->videoStream) {
        impl_->cleanup();
        return false;
    }

    AVCodecParameters* par = impl_->videoStream->codecpar;
    par->codec_type = AVMEDIA_TYPE_VIDEO;
    par->codec_id = AV_CODEC_ID_HEVC;
    par->width = config.width;
    par->height = config.height;

    // Timebase matching framerate
    impl_->videoStream->time_base = AVRational{config.fpsDen > 0 ? config.fpsDen : 1, config.fpsNum > 0 ? config.fpsNum : 25};
    impl_->videoStream->r_frame_rate = AVRational{config.fpsNum > 0 ? config.fpsNum : 25, config.fpsDen > 0 ? config.fpsDen : 1};
    impl_->videoStream->avg_frame_rate = impl_->videoStream->r_frame_rate;

    if (config.sarWidth > 0 && config.sarHeight > 0) {
        impl_->videoStream->sample_aspect_ratio = AVRational{config.sarWidth, config.sarHeight};
        par->sample_aspect_ratio = impl_->videoStream->sample_aspect_ratio;
    }

    // Color metadata
    if (config.colorPrimaries >= 0) par->color_primaries = static_cast<AVColorPrimaries>(config.colorPrimaries);
    if (config.transferCharacteristics >= 0) par->color_trc = static_cast<AVColorTransferCharacteristic>(config.transferCharacteristics);
    if (config.matrixCoeffs >= 0) par->color_space = static_cast<AVColorSpace>(config.matrixCoeffs);
    if (config.fullRange) par->color_range = AVCOL_RANGE_JPEG;

    if (!(impl_->formatCtx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&impl_->formatCtx->pb, config.outputPath.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            impl_->cleanup();
            return false;
        }
    }

    return true;
}

bool MkvOutput::writeHeaders(const x265_nal* nals, uint32_t nalCount) {
    if (!impl_->formatCtx || !impl_->videoStream || nalCount == 0) return false;

    // Concatenate VPS, SPS, PPS extradata for HEVC in Matroska
    std::vector<uint8_t> extradata;
    for (uint32_t i = 0; i < nalCount; ++i) {
        extradata.insert(extradata.end(), nals[i].payload, nals[i].payload + nals[i].sizeBytes);
    }

    if (!extradata.empty()) {
        impl_->videoStream->codecpar->extradata = static_cast<uint8_t*>(av_mallocz(extradata.size() + AV_INPUT_BUFFER_PADDING_SIZE));
        impl_->videoStream->codecpar->extradata_size = static_cast<int>(extradata.size());
        std::memcpy(impl_->videoStream->codecpar->extradata, extradata.data(), extradata.size());
    }

    int ret = avformat_write_header(impl_->formatCtx, nullptr);
    if (ret < 0) {
        return false;
    }
    impl_->headerWritten = true;
    return true;
}

bool MkvOutput::writeFrame(const x265_nal* nals, uint32_t nalCount, const x265_picture& pic) {
    if (!impl_->formatCtx || !impl_->headerWritten || nalCount == 0) return false;

    size_t totalBytes = 0;
    for (uint32_t i = 0; i < nalCount; ++i) {
        totalBytes += nals[i].sizeBytes;
    }

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) return false;

    int ret = av_new_packet(pkt, static_cast<int>(totalBytes));
    if (ret < 0) {
        av_packet_free(&pkt);
        return false;
    }

    uint8_t* dst = pkt->data;
    for (uint32_t i = 0; i < nalCount; ++i) {
        std::memcpy(dst, nals[i].payload, nals[i].sizeBytes);
        dst += nals[i].sizeBytes;
    }

    if (impl_->frameCount == 0) {
        impl_->startOffset = -pic.dts;
    }

    pkt->pts = pic.pts + impl_->startOffset;
    pkt->dts = pic.dts + impl_->startOffset;
    pkt->duration = 1;
    pkt->stream_index = impl_->videoStream->index;
    pkt->flags = (pic.sliceType == X265_TYPE_IDR) ? AV_PKT_FLAG_KEY : 0;

    AVRational srcTb{impl_->config.fpsDen > 0 ? impl_->config.fpsDen : 1, impl_->config.fpsNum > 0 ? impl_->config.fpsNum : 25};
    av_packet_rescale_ts(pkt, srcTb, impl_->videoStream->time_base);

    ret = av_interleaved_write_frame(impl_->formatCtx, pkt);
    av_packet_free(&pkt);

    impl_->frameCount++;
    return ret >= 0;
}

void MkvOutput::close(int64_t /*largestPts*/, int64_t /*secondLargestPts*/) {
    impl_->cleanup();
}

} // namespace sk265::pipeline::output
