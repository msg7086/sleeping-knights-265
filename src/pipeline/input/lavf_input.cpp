#include "pipeline/input/lavf_input.h"
#include <iostream>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#include <time.h>
extern "C" {
int clock_gettime(clockid_t clk_id, struct timespec *tp);
int nanosleep(const struct timespec *req, struct timespec *rem);

int clock_gettime64(clockid_t clk_id, struct timespec *tp) {
    return clock_gettime(clk_id, tp);
}

int nanosleep64(const struct timespec *req, struct timespec *rem) {
    return nanosleep(req, rem);
}
}
#endif

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
#include <libavutil/dict.h>
#include <libswscale/swscale.h>
}

namespace sk265::pipeline::input {

struct LavfInput::Impl {
    AVFormatContext* formatCtx{nullptr};
    AVCodecContext* codecCtx{nullptr};
    AVFrame* avFrame{nullptr};
    AVPacket* avPacket{nullptr};
    int videoStreamIndex{-1};

    InputInfo info{};
    bool eof{false};
    bool flushing{false};
    int64_t currentFrameIndex{0};
    int64_t seekFrame{0};
    int planeCount{3};

    ~Impl() {
        cleanup();
    }

    void cleanup() {
        if (avFrame) {
            av_frame_free(&avFrame);
        }
        if (avPacket) {
            av_packet_free(&avPacket);
        }
        if (codecCtx) {
            avcodec_free_context(&codecCtx);
        }
        if (formatCtx) {
            avformat_close_input(&formatCtx);
        }
        videoStreamIndex = -1;
        eof = true;
        flushing = false;
    }
};

LavfInput::LavfInput() : impl_(std::make_unique<Impl>()) {}

LavfInput::~LavfInput() = default;

void LavfInput::setSeekFrame(int64_t seekFrame) {
    impl_->seekFrame = std::max<int64_t>(0, seekFrame);
}

bool LavfInput::isEof() const {
    return impl_->eof || (impl_->info.totalFrames > 0 && impl_->currentFrameIndex >= impl_->info.totalFrames);
}

InputInfo LavfInput::getInfo() const {
    return impl_->info;
}

void LavfInput::close() {
    impl_->cleanup();
}

bool LavfInput::open(const std::string& path) {
    if (path.empty()) {
        impl_->cleanup();
        return false;
    }
    impl_->cleanup();

    int ret = avformat_open_input(&impl_->formatCtx, path.c_str(), nullptr, nullptr);
    if (ret < 0) {
        impl_->cleanup();
        return false;
    }

    ret = avformat_find_stream_info(impl_->formatCtx, nullptr);
    if (ret < 0) {
        impl_->cleanup();
        return false;
    }

    impl_->videoStreamIndex = av_find_best_stream(impl_->formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (impl_->videoStreamIndex < 0) {
        impl_->cleanup();
        return false;
    }

    AVStream* videoStream = impl_->formatCtx->streams[impl_->videoStreamIndex];
    const AVCodec* decoder = avcodec_find_decoder(videoStream->codecpar->codec_id);
    if (!decoder) {
        std::cerr << "sk265[error]: No suitable FFmpeg decoder found for codec ID " << videoStream->codecpar->codec_id << "\n";
        impl_->cleanup();
        return false;
    }

    impl_->codecCtx = avcodec_alloc_context3(decoder);
    if (!impl_->codecCtx) {
        impl_->cleanup();
        return false;
    }

    if (avcodec_parameters_to_context(impl_->codecCtx, videoStream->codecpar) < 0) {
        impl_->cleanup();
        return false;
    }

    // Enable multi-threaded software decoding
    impl_->codecCtx->thread_count = 0;
    impl_->codecCtx->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

    if (avcodec_open2(impl_->codecCtx, decoder, nullptr) < 0) {
        impl_->cleanup();
        return false;
    }

    impl_->info.width = impl_->codecCtx->width;
    impl_->info.height = impl_->codecCtx->height;

    AVRational rate = av_guess_frame_rate(impl_->formatCtx, videoStream, nullptr);
    impl_->info.fpsNum = rate.num > 0 ? rate.num : 25;
    impl_->info.fpsDen = rate.den > 0 ? rate.den : 1;

    // Total frame count detection
    int64_t nb = videoStream->nb_frames;
    if (nb <= 0) {
        AVDictionaryEntry* tag = av_dict_get(videoStream->metadata, "NUMBER_OF_FRAMES", nullptr, 0);
        if (!tag) tag = av_dict_get(videoStream->metadata, "NUMBER_OF_FRAMES-eng", nullptr, 0);
        if (tag && tag->value) {
            nb = std::atoll(tag->value);
        }
    }
    if (nb <= 0 && videoStream->duration > 0 && videoStream->time_base.den > 0) {
        double durSec = videoStream->duration * av_q2d(videoStream->time_base);
        nb = static_cast<int64_t>(durSec * (static_cast<double>(impl_->info.fpsNum) / impl_->info.fpsDen));
    }
    impl_->info.totalFrames = nb > 0 ? nb : 0;

    // Bit depth & Pixel format descriptor
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(impl_->codecCtx->pix_fmt);
    impl_->info.bitDepth = desc ? desc->comp[0].depth : 8;

    impl_->planeCount = 3;
    if (desc && (desc->flags & AV_PIX_FMT_FLAG_PLANAR)) {
        if (desc->nb_components == 1 || (desc->log2_chroma_w == 0 && desc->log2_chroma_h == 0 && desc->nb_components == 1)) {
            impl_->planeCount = 1;
            impl_->info.colorSpace = 0; // X265_CSP_I400
        } else if (desc->log2_chroma_w == 1 && desc->log2_chroma_h == 1) {
            impl_->info.colorSpace = 1; // X265_CSP_I420
        } else if (desc->log2_chroma_w == 1 && desc->log2_chroma_h == 0) {
            impl_->info.colorSpace = 2; // X265_CSP_I422
        } else if (desc->log2_chroma_w == 0 && desc->log2_chroma_h == 0) {
            impl_->info.colorSpace = 3; // X265_CSP_I444
        } else {
            impl_->info.colorSpace = 1;
        }
    } else {
        impl_->info.colorSpace = 1; // Default I420
    }

    impl_->avFrame = av_frame_alloc();
    impl_->avPacket = av_packet_alloc();
    if (!impl_->avFrame || !impl_->avPacket) {
        impl_->cleanup();
        return false;
    }

    std::cerr << "lavf [info]: demuxer: " << impl_->formatCtx->iformat->name
              << " (" << decoder->name << " " << impl_->info.width << "x" << impl_->info.height << ")\n";

    impl_->currentFrameIndex = 0;
    impl_->eof = false;
    impl_->flushing = false;
    return true;
}

std::optional<VideoFrame> LavfInput::readFrame() {
    if (impl_->eof || !impl_->codecCtx || !impl_->formatCtx) {
        return std::nullopt;
    }

    while (true) {
        int ret = avcodec_receive_frame(impl_->codecCtx, impl_->avFrame);
        if (ret == 0) {
            // Frame successfully decoded
            if (impl_->seekFrame > 0 && impl_->currentFrameIndex < impl_->seekFrame) {
                impl_->currentFrameIndex++;
                av_frame_unref(impl_->avFrame);
                continue; // Drop frames until seek target reached
            }

            VideoFrame frame;
            frame.allocate(impl_->info.width, impl_->info.height, impl_->info.bitDepth, impl_->info.colorSpace);
            frame.pts = impl_->currentFrameIndex++;

            int bytesPerSample = (impl_->info.bitDepth > 8) ? 2 : 1;
            for (int p = 0; p < impl_->planeCount; ++p) {
                const uint8_t* src = impl_->avFrame->data[p];
                int srcStride = impl_->avFrame->linesize[p];
                if (!src) continue;

                int planeH = impl_->info.height;
                int planeW = impl_->info.width;
                if (p > 0) {
                    planeH = (impl_->info.colorSpace == 1) ? (impl_->info.height / 2) : impl_->info.height;
                    planeW = (impl_->info.colorSpace == 3) ? impl_->info.width : (impl_->info.width / 2);
                }
                int rowBytes = planeW * bytesPerSample;

                uint8_t* dst = const_cast<uint8_t*>(frame.planes[p].data());
                for (int y = 0; y < planeH; ++y) {
                    std::memcpy(dst + y * frame.strides[p], src + y * srcStride, rowBytes);
                }
            }

            av_frame_unref(impl_->avFrame);
            return frame;
        } else if (ret == AVERROR(EAGAIN)) {
            // Need more packets
            if (impl_->flushing) {
                impl_->eof = true;
                return std::nullopt;
            }

            int pkgRet = av_read_frame(impl_->formatCtx, impl_->avPacket);
            if (pkgRet < 0) {
                // Container EOF reached: send flush packet to decoder
                impl_->flushing = true;
                avcodec_send_packet(impl_->codecCtx, nullptr);
            } else {
                if (impl_->avPacket->stream_index == impl_->videoStreamIndex) {
                    avcodec_send_packet(impl_->codecCtx, impl_->avPacket);
                }
                av_packet_unref(impl_->avPacket);
            }
        } else if (ret == AVERROR_EOF) {
            impl_->eof = true;
            return std::nullopt;
        } else {
            impl_->eof = true;
            return std::nullopt;
        }
    }
}

} // namespace sk265::pipeline::input
