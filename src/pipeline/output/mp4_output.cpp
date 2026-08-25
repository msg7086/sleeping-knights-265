#include "pipeline/output/mp4_output.h"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace sk265::pipeline::output {

struct ParsedNalu {
    const uint8_t* data{nullptr};
    size_t size{0};
    uint8_t type{0};
};

static ParsedNalu parseNal(const x265_nal& nal) {
    ParsedNalu res{};
    if (nal.sizeBytes < 4 || !nal.payload) return res;

    // Check for Annex-B start code: 00 00 01 or 00 00 00 01
    if (nal.payload[0] == 0 && nal.payload[1] == 0) {
        if (nal.payload[2] == 1) {
            res.data = nal.payload + 3;
            res.size = nal.sizeBytes - 3;
        } else if (nal.payload[2] == 0 && nal.payload[3] == 1) {
            res.data = nal.payload + 4;
            res.size = nal.sizeBytes - 4;
        } else {
            res.data = nal.payload + 4;
            res.size = nal.sizeBytes - 4;
        }
    } else {
        res.data = nal.payload + 4;
        res.size = nal.sizeBytes - 4;
    }

    if (res.size > 0) {
        res.type = (res.data[0] >> 1) & 0x3F;
    }
    return res;
}

static void appendNalToSample(std::vector<uint8_t>& buf, const ParsedNalu& nalu) {
    if (nalu.size == 0 || !nalu.data) return;
    uint32_t len = static_cast<uint32_t>(nalu.size);
    buf.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));
    buf.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(len & 0xFF));
    buf.insert(buf.end(), nalu.data, nalu.data + nalu.size);
}

Mp4Output::Mp4Output() = default;

Mp4Output::~Mp4Output() {
    close();
}

void Mp4Output::cleanup() {
    if (summary_) {
        lsmash_cleanup_summary(reinterpret_cast<lsmash_summary_t*>(summary_));
        summary_ = nullptr;
    }
    if (root_) {
        lsmash_close_file(&fileParam_);
        lsmash_destroy_root(root_);
        root_ = nullptr;
    }
    track_ = 0;
    sampleEntry_ = 0;
    seiBuffer_.clear();
}

void Mp4Output::sign() {
    if (!root_) return;
    const char* string = "Multiplexed by L-SMASH (sk265)";
    int length = static_cast<int>(std::strlen(string));
    lsmash_box_type_t type = lsmash_form_iso_box_type(LSMASH_4CC('f', 'r', 'e', 'e'));
    lsmash_box_t* free_box = lsmash_create_box(type, reinterpret_cast<uint8_t*>(const_cast<char*>(string)), length, LSMASH_BOX_PRECEDENCE_N);
    if (!free_box) return;
    if (lsmash_add_box_ex(lsmash_root_as_box(root_), &free_box) < 0) {
        lsmash_destroy_box(free_box);
        return;
    }
    lsmash_write_top_level_box(free_box);
}

bool Mp4Output::open(const OutputConfig& config) {
    if (config.outputPath.empty()) return false;
    cleanup();
    config_ = config;

    root_ = lsmash_create_root();
    if (!root_) return false;

    if (lsmash_open_file(config.outputPath.c_str(), 0, &fileParam_) < 0) {
        cleanup();
        return false;
    }

    summary_ = reinterpret_cast<lsmash_video_summary_t*>(lsmash_create_summary(LSMASH_SUMMARY_TYPE_VIDEO));
    if (!summary_) {
        cleanup();
        return false;
    }
    summary_->sample_type = ISOM_CODEC_TYPE_HVC1_VIDEO;
    summary_->width = config.width;
    summary_->height = config.height;

    uint32_t i_display_width = config.width << 16;
    uint32_t i_display_height = config.height << 16;
    if (config.sarWidth > 0 && config.sarHeight > 0) {
        double sar = static_cast<double>(config.sarWidth) / config.sarHeight;
        if (sar > 1.0) {
            i_display_width = static_cast<uint32_t>(i_display_width * sar);
        } else if (sar > 0.0) {
            i_display_height = static_cast<uint32_t>(i_display_height / sar);
        }
        summary_->par_h = config.sarWidth;
        summary_->par_v = config.sarHeight;
    }

    summary_->color.primaries_index = (config.colorPrimaries >= 0) ? config.colorPrimaries : ISOM_PRIMARIES_INDEX_UNSPECIFIED;
    summary_->color.transfer_index = (config.transferCharacteristics >= 0) ? config.transferCharacteristics : ISOM_TRANSFER_INDEX_UNSPECIFIED;
    summary_->color.matrix_index = (config.matrixCoeffs >= 0) ? config.matrixCoeffs : ISOM_MATRIX_INDEX_UNSPECIFIED;
    summary_->color.full_range = config.fullRange ? 1 : 0;

    lsmash_brand_type brands[3];
    brands[0] = ISOM_BRAND_TYPE_MP42;
    brands[1] = ISOM_BRAND_TYPE_MP41;
    brands[2] = ISOM_BRAND_TYPE_ISOM;

    fileParam_.major_brand = brands[0];
    fileParam_.brands = brands;
    fileParam_.brand_count = 3;
    fileParam_.minor_version = 0;
    if (!lsmash_set_file(root_, &fileParam_)) {
        cleanup();
        return false;
    }

    lsmash_movie_parameters_t movie_param;
    lsmash_initialize_movie_parameters(&movie_param);
    if (lsmash_set_movie_parameters(root_, &movie_param) != 0) {
        cleanup();
        return false;
    }
    movieTimescale_ = lsmash_get_movie_timescale(root_);
    if (!movieTimescale_) {
        cleanup();
        return false;
    }

    track_ = lsmash_create_track(root_, ISOM_MEDIA_HANDLER_TYPE_VIDEO_TRACK);
    if (!track_) {
        cleanup();
        return false;
    }

    lsmash_track_parameters_t track_param;
    lsmash_initialize_track_parameters(&track_param);
    track_param.mode = static_cast<lsmash_track_mode>(ISOM_TRACK_ENABLED | ISOM_TRACK_IN_MOVIE | ISOM_TRACK_IN_PREVIEW);
    track_param.display_width = i_display_width;
    track_param.display_height = i_display_height;
    if (lsmash_set_track_parameters(root_, track_, &track_param) != 0) {
        cleanup();
        return false;
    }

    mediaTimescale_ = config.fpsNum > 0 ? static_cast<uint32_t>(config.fpsNum) : 25;
    timeInc_ = config.fpsDen > 0 ? static_cast<uint64_t>(config.fpsDen) : 1;

    lsmash_media_parameters_t media_param;
    lsmash_initialize_media_parameters(&media_param);
    media_param.timescale = mediaTimescale_;
    media_param.media_handler_name = const_cast<char*>("L-SMASH Video Media Handler");
    if (lsmash_set_media_parameters(root_, track_, &media_param) != 0) {
        cleanup();
        return false;
    }
    mediaTimescale_ = lsmash_get_media_timescale(root_, track_);
    if (!mediaTimescale_) {
        cleanup();
        return false;
    }

    numFrames_ = 0;
    startOffset_ = 0;
    firstCts_ = 0;
    largestPts_ = 0;
    secondLargestPts_ = 0;
    return true;
}

bool Mp4Output::writeHeaders(const x265_nal* nals, uint32_t nalCount) {
    if (!root_ || !track_ || !summary_ || nalCount < 3) return false;

    constexpr uint32_t NALU_LENGTH_SIZE = 4;
    ParsedNalu vpsNal{};
    ParsedNalu spsNal{};
    ParsedNalu ppsNal{};

    for (uint32_t i = 0; i < nalCount; ++i) {
        auto parsed = parseNal(nals[i]);
        if (parsed.type == 32 && !vpsNal.data) {
            vpsNal = parsed;
        } else if (parsed.type == 33 && !spsNal.data) {
            spsNal = parsed;
        } else if (parsed.type == 34 && !ppsNal.data) {
            ppsNal = parsed;
        } else if (parsed.type != 32 && parsed.type != 33 && parsed.type != 34) {
            appendNalToSample(seiBuffer_, parsed);
        }
    }

    if (!vpsNal.data || !spsNal.data || !ppsNal.data) return false;

    lsmash_codec_specific_t* cs = lsmash_create_codec_specific_data(
        LSMASH_CODEC_SPECIFIC_DATA_TYPE_ISOM_VIDEO_HEVC,
        LSMASH_CODEC_SPECIFIC_FORMAT_STRUCTURED
    );
    if (!cs) return false;

    auto* param = reinterpret_cast<lsmash_hevc_specific_parameters_t*>(cs->data.structured);
    param->lengthSizeMinusOne = NALU_LENGTH_SIZE - 1;

    if (lsmash_append_hevc_dcr_nalu(param, HEVC_DCR_NALU_TYPE_VPS, const_cast<uint8_t*>(vpsNal.data), static_cast<uint32_t>(vpsNal.size)) != 0 ||
        lsmash_append_hevc_dcr_nalu(param, HEVC_DCR_NALU_TYPE_SPS, const_cast<uint8_t*>(spsNal.data), static_cast<uint32_t>(spsNal.size)) != 0 ||
        lsmash_append_hevc_dcr_nalu(param, HEVC_DCR_NALU_TYPE_PPS, const_cast<uint8_t*>(ppsNal.data), static_cast<uint32_t>(ppsNal.size)) != 0) {
        lsmash_destroy_codec_specific_data(cs);
        return false;
    }

    if (lsmash_add_codec_specific_data(reinterpret_cast<lsmash_summary_t*>(summary_), cs) != 0) {
        lsmash_destroy_codec_specific_data(cs);
        return false;
    }
    lsmash_destroy_codec_specific_data(cs);

    // Inject Dolby Vision configuration box (dvcC / dvvC) if enabled
    if (config_.doviProfile > 0) {
        uint8_t dv_profile = 8;
        uint8_t compatibility_id = 1; // default HDR10 cross-compatibility
        if (config_.doviProfile == 50 || config_.doviProfile == 5) {
            dv_profile = 5;
            compatibility_id = 0;
        } else if (config_.doviProfile == 81 || config_.doviProfile == 8) {
            dv_profile = 8;
            compatibility_id = 1;
        } else if (config_.doviProfile == 82) {
            dv_profile = 8;
            compatibility_id = 2;
        } else if (config_.doviProfile == 84) {
            dv_profile = 8;
            compatibility_id = 4;
        }

        lsmash_codec_specific_t* dovi = lsmash_create_codec_specific_data(
            LSMASH_CODEC_SPECIFIC_DATA_TYPE_ISOM_VIDEO_HEVC_DOVI,
            LSMASH_CODEC_SPECIFIC_FORMAT_STRUCTURED
        );
        if (dovi) {
            lsmash_dovi_set_config(
                reinterpret_cast<lsmash_hevc_dovi_t*>(dovi->data.structured),
                dv_profile,
                compatibility_id,
                mediaTimescale_,
                static_cast<uint32_t>(timeInc_),
                config_.width,
                config_.height
            );
            lsmash_add_codec_specific_data(reinterpret_cast<lsmash_summary_t*>(summary_), dovi);
            lsmash_destroy_codec_specific_data(dovi);
        }
    }

    sampleEntry_ = lsmash_add_sample_entry(root_, track_, summary_);
    if (!sampleEntry_) return false;

    return true;
}

bool Mp4Output::writeFrame(const x265_nal* nals, uint32_t nalCount, const x265_picture& pic) {
    if (!root_ || !track_ || !sampleEntry_) return false;

    int64_t pts = pic.pts;
    int64_t dts = pic.dts;

    if (numFrames_ == 0) {
        startOffset_ = -dts;
        firstCts_ = static_cast<uint64_t>((pts + startOffset_) * timeInc_);
    }

    std::vector<uint8_t> samplePayload;
    if (!seiBuffer_.empty()) {
        samplePayload.insert(samplePayload.end(), seiBuffer_.begin(), seiBuffer_.end());
        seiBuffer_.clear();
    }

    for (uint32_t i = 0; i < nalCount; ++i) {
        auto parsed = parseNal(nals[i]);
        if (parsed.type == 32 || parsed.type == 33 || parsed.type == 34) {
            // Parameter sets are in hvcC box; skip in-band repeats for strict hvc1 compliance
            continue;
        }
        appendNalToSample(samplePayload, parsed);
    }

    if (samplePayload.empty()) {
        return true;
    }

    lsmash_sample_t* sample = lsmash_create_sample(static_cast<uint32_t>(samplePayload.size()));
    if (!sample) return false;

    std::memcpy(sample->data, samplePayload.data(), samplePayload.size());

    sample->dts = static_cast<uint64_t>((dts + startOffset_) * timeInc_);
    sample->cts = static_cast<uint64_t>((pts + startOffset_) * timeInc_);
    sample->index = sampleEntry_;
    sample->prop.ra_flags = (pic.sliceType == X265_TYPE_IDR)
        ? ISOM_SAMPLE_RANDOM_ACCESS_FLAG_SYNC
        : ISOM_SAMPLE_RANDOM_ACCESS_FLAG_NONE;

    if (lsmash_append_sample(root_, track_, sample) != 0) {
        return false;
    }

    if (pts > largestPts_) {
        secondLargestPts_ = largestPts_;
        largestPts_ = pts;
    } else if (pts > secondLargestPts_ && pts < largestPts_) {
        secondLargestPts_ = pts;
    }
    numFrames_++;
    return true;
}

void Mp4Output::close(int64_t largestPts, int64_t secondLargestPts) {
    if (root_ && track_) {
        int64_t lPts = (largestPts >= 0) ? largestPts : largestPts_;
        int64_t sPts = (secondLargestPts >= 0) ? secondLargestPts : secondLargestPts_;
        uint32_t lastDelta = static_cast<uint32_t>((lPts > sPts) ? (lPts - sPts) : 1);

        lsmash_flush_pooled_samples(root_, track_, lastDelta * timeInc_);

        double actual_duration = 0.0;
        if (movieTimescale_ != 0 && mediaTimescale_ != 0) {
            actual_duration = ((static_cast<double>((lPts + lastDelta) * timeInc_)) / mediaTimescale_) * movieTimescale_;
        }

        lsmash_edit_t edit;
        edit.duration = actual_duration;
        edit.start_time = firstCts_;
        edit.rate = ISOM_EDIT_MODE_NORMAL;
        lsmash_create_explicit_timeline_map(root_, track_, edit);
        lsmash_modify_explicit_timeline_map(root_, track_, 1, edit);

        lsmash_finish_movie(root_, nullptr);
        sign();
    }
    cleanup();
}

} // namespace sk265::pipeline::output
