#include "pipeline/output/output_factory.h"
#include "pipeline/output/raw_output.h"
#include "pipeline/output/mp4_output.h"
#include "pipeline/output/lavf_output.h"
#include "pipeline/output/gop_output.h"
#include <algorithm>
#include <cctype>

namespace sk265::pipeline::output {

static bool hasExtension(const std::string& path, const std::string& ext) {
    if (path.size() < ext.size()) return false;
    return std::equal(ext.rbegin(), ext.rend(), path.rbegin(),
                      [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); });
}

static bool isKnownContainer(const std::string& path) {
    std::vector<std::string> containerExts = {
        ".mp4", ".m4v", ".mov", ".mkv", ".webm", ".ts", ".m2ts", ".mts", ".avi", ".flv", ".ogv", ".gop"
    };
    for (const auto& ext : containerExts) {
        if (hasExtension(path, ext)) return true;
    }
    return false;
}

OutputFactoryResult OutputFactory::create(const std::string& muxerParam, const std::string& outputPath) {
    OutputFactoryResult res;

    std::string muxer = muxerParam.empty() ? "auto" : muxerParam;
    for (char& c : muxer) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    // 1. Validate muxer parameter name
    if (muxer != "auto" && muxer != "lsmash" && muxer != "lavf" && muxer != "ffmpeg" && muxer != "raw" && muxer != "gop") {
        res.success = false;
        res.errorMessage = "Unknown muxer '" + muxerParam + "' (supported: auto, lsmash, lavf/ffmpeg, raw, gop)";
        return res;
    }

    // Strip query parameters for extension checks
    std::string cleanPath = outputPath;
    auto qPos = cleanPath.find('?');
    if (qPos != std::string::npos) {
        cleanPath = cleanPath.substr(0, qPos);
    }

    // 2. Resolve target muxer engine
    std::string resolvedMuxer = muxer;
    if (muxer == "auto") {
        if (hasExtension(cleanPath, ".mp4") || hasExtension(cleanPath, ".m4v") || hasExtension(cleanPath, ".mov")) {
            resolvedMuxer = "lsmash";
        } else if (hasExtension(cleanPath, ".mkv")) {
            resolvedMuxer = "lavf";
        } else if (hasExtension(cleanPath, ".gop")) {
            resolvedMuxer = "gop";
        } else {
            resolvedMuxer = "raw"; // Defaults to raw for .hevc, .h265, .265, .bin, .raw, .bit, -, etc.
        }
    }

    // 3. Verify format and container compatibility
    if (resolvedMuxer == "gop") {
        if (!hasExtension(cleanPath, ".gop")) {
            res.success = false;
            res.errorMessage = "Muxer 'gop' only supports GOP manifest files (.gop), got: " + outputPath;
            return res;
        }
        res.instance.output = std::make_unique<GopOutput>();
        res.instance.muxerName = "gop";
        res.instance.bAnnexB = false;
        res.instance.bRepeatHeaders = false;
        res.success = true;
        return res;
    }

    if (resolvedMuxer == "lsmash") {
        if (!hasExtension(cleanPath, ".mp4") && !hasExtension(cleanPath, ".m4v") && !hasExtension(cleanPath, ".mov")) {
            res.success = false;
            res.errorMessage = "Muxer 'lsmash' only supports MP4/MOV container files (.mp4, .m4v, .mov), got: " + outputPath;
            return res;
        }
        res.instance.output = std::make_unique<Mp4Output>();
        res.instance.muxerName = "lsmash";
        res.instance.bAnnexB = false;
        res.instance.bRepeatHeaders = false;
        res.success = true;
        return res;
    }

    if (resolvedMuxer == "lavf" || resolvedMuxer == "ffmpeg") {
        if (cleanPath == "-" || !isKnownContainer(cleanPath)) {
            res.success = false;
            res.errorMessage = "Muxer 'lavf' is a container multiplexer and cannot output raw bitstream (" + outputPath + "), use '--muxer raw' instead";
            return res;
        }
        res.instance.output = std::make_unique<LavfOutput>();
        res.instance.muxerName = "lavf";
        res.instance.bAnnexB = true;
        res.instance.bRepeatHeaders = true;
        res.success = true;
        return res;
    }

    if (resolvedMuxer == "raw") {
        if (isKnownContainer(cleanPath)) {
            res.success = false;
            res.errorMessage = "Muxer 'raw' is for Annex-B bitstreams (.hevc, .h265, .265, .bin, .raw), cannot output to container file: " + outputPath;
            return res;
        }
        res.instance.output = std::make_unique<RawOutput>();
        res.instance.muxerName = "raw";
        res.instance.bAnnexB = true;
        res.instance.bRepeatHeaders = true;
        res.success = true;
        return res;
    }

    res.success = false;
    res.errorMessage = "Unexpected resolution state for muxer: " + muxerParam;
    return res;
}

} // namespace sk265::pipeline::output
