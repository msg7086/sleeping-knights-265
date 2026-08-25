#include "core/tuning_preset.h"
#include <algorithm>
#include <cctype>

namespace sk265::core {

static std::string normalize(const std::string& str) {
    std::string s = str;
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool TuningPreset::isCustomTune(const std::string& tuneName) {
    std::string s = normalize(tuneName);
    return (s == "vcb-s" || s == "vcbs" || s == "vcb-s++" || s == "vcbs++" ||
            s == "lp" || s == "littlepox" || s == "lp++" || s == "littlepox++");
}

void TuningPreset::apply(x265_param* param, const std::string& tuneName) {
    if (!param) return;
    std::string tune = normalize(tuneName);

    // Common baseline for VCB-S / LP animation & film tuning
    param->searchRange = 25; // down from 57
    param->bEnableAMP = 0;
    param->bEnableRectInter = 0;
    param->rc.aqStrength = 0.8; // down from 1.0
    if (param->rdLevel < 4) param->rdLevel = 4;
    param->rdoqLevel = 2; // force rdoq to be effective
    param->bEnableSAO = 0;
    param->bEnableStrongIntraSmoothing = 0;
    if (param->bframes + 1 < param->lookaheadDepth) param->bframes++;
    if (param->bframes + 1 < param->lookaheadDepth) param->bframes++;
    if (param->tuQTMaxInterDepth > 3) param->tuQTMaxInterDepth--;
    if (param->tuQTMaxIntraDepth > 3) param->tuQTMaxIntraDepth--;
    if (param->maxNumMergeCand > 3) param->maxNumMergeCand--;
    if (param->subpelRefine < 3) param->subpelRefine = 3;
    param->keyframeMin = 1;
    param->keyframeMax = 360;
    param->bOpenGOP = 0;
    param->deblockingFilterBetaOffset = -1;
    param->deblockingFilterTCOffset = -1;
    param->maxCUSize = 32;
    param->maxTUSize = 32;
    param->rc.qgSize = 8;
    param->cbQpOffset = -2; // better chroma quality to compensate 420 subsampling
    param->crQpOffset = -2;
    param->rc.pbFactor = 1.2; // down from 1.3
    param->bEnableWeightedBiPred = 1;

    bool isLp = (tune.rfind("lp", 0) == 0 || tune.rfind("littlepox", 0) == 0);
    bool isPlusPlus = (tune.find("++") != std::string::npos);

    if (isLp) {
        // Mid bitrate anime
        param->rc.rfConstant = 20.0;
        param->psyRd = 1.5;
        param->psyRdoq = 0.8;

        if (isPlusPlus) {
            if (param->maxNumReferences < 2) param->maxNumReferences = 2;
            if (param->subpelRefine < 3) param->subpelRefine = 3;
            if (param->lookaheadDepth < 60) param->lookaheadDepth = 60;
            param->searchRange = 38;
        }
    } else {
        // High bitrate anime BD / film (VCB-S)
        param->rc.rfConstant = 18.0;
        param->psyRd = 1.8;
        param->psyRdoq = 1.0;

        if (isPlusPlus) {
            if (param->maxNumReferences < 3) param->maxNumReferences = 3;
            if (param->subpelRefine < 3) param->subpelRefine = 3;
            param->bIntraInBFrames = 1;
            param->bEnableRectInter = 1;
            param->limitTU = 4;
            if (param->lookaheadDepth < 60) param->lookaheadDepth = 60;
            param->searchRange = 38;
        }
    }
}

} // namespace sk265::core
