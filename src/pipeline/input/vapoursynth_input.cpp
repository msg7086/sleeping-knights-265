#include "pipeline/input/vapoursynth_input.h"
#include <iostream>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace sk265::pipeline::input {

VapourSynthInput::VapourSynthInput() = default;

VapourSynthInput::~VapourSynthInput() {
    close();
}

void VapourSynthInput::close() {
    if (node_ && vsApi_ && vsApi_->freeNode) {
        vsApi_->freeNode(node_);
        node_ = nullptr;
    }
    if (script_ && vssApi_ && vssApi_->freeScript) {
        vssApi_->freeScript(script_);
        script_ = nullptr;
    }
    if (libHandle_) {
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(libHandle_));
#else
        dlclose(libHandle_);
#endif
        libHandle_ = nullptr;
    }
    vssApi_ = nullptr;
    vsApi_ = nullptr;
    eof_ = true;
}

bool VapourSynthInput::loadLibrary(const std::string& customLibPath) {
    if (libHandle_) return true;

#ifdef _WIN32
    if (!customLibPath.empty()) {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, customLibPath.c_str(), -1, nullptr, 0);
        if (wlen > 0) {
            std::vector<wchar_t> wbuf(wlen);
            MultiByteToWideChar(CP_UTF8, 0, customLibPath.c_str(), -1, wbuf.data(), wlen);
            libHandle_ = LoadLibraryW(wbuf.data());
        }
    } else {
        libHandle_ = LoadLibraryW(L"vsscript.dll");
        if (!libHandle_) libHandle_ = LoadLibraryW(L"VSScript.dll");
    }
    auto getSym = [this](const char* name) -> void* {
        return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(libHandle_), name));
    };
#else
    if (!customLibPath.empty()) {
        libHandle_ = dlopen(customLibPath.c_str(), RTLD_NOW);
    } else {
#ifdef __APPLE__
        libHandle_ = dlopen("libvapoursynth-script.dylib", RTLD_NOW);
#else
        libHandle_ = dlopen("libvapoursynth-script.so", RTLD_NOW);
#endif
    }
    auto getSym = [this](const char* name) -> void* {
        return dlsym(libHandle_, name);
    };
#endif

    if (!libHandle_) return false;

    using GetVSScriptAPIFunc = const VSSCRIPTAPI* (VS_CC *)(int);
    auto getVSScriptAPI = reinterpret_cast<GetVSScriptAPIFunc>(getSym("getVSScriptAPI"));
    if (!getVSScriptAPI) {
        close();
        return false;
    }

    vssApi_ = getVSScriptAPI(VSSCRIPT_API_VERSION);
    if (!vssApi_ || !vssApi_->getVSAPI) {
        close();
        return false;
    }

    vsApi_ = vssApi_->getVSAPI(VAPOURSYNTH_API_VERSION);
    if (!vsApi_) {
        close();
        return false;
    }

    return true;
}

bool VapourSynthInput::open(const std::string& path) {
    if (path.empty()) {
        close();
        return false;
    }

    if (!loadLibrary(customLibPath_)) {
        close();
        return false;
    }

    script_ = vssApi_->createScript(nullptr);
    if (!script_) {
        close();
        return false;
    }

    VSCore* core = vssApi_->getCore(script_);
    if (core && vsApi_->getCoreInfo) {
        VSCoreInfo coreInfo{};
        vsApi_->getCoreInfo(core, &coreInfo);
        if (coreInfo.versionString) {
            std::cerr << "sk265[info]: " << coreInfo.versionString << "\n";
        }
    }

    if (vssApi_->evalSetWorkingDir) {
        vssApi_->evalSetWorkingDir(script_, 1);
    }

    int evalRet = vssApi_->evaluateFile(script_, path.c_str());
    if (evalRet != 0) {
        const char* err = vssApi_->getError(script_);
        std::cerr << "sk265[error]: VapourSynth script evaluation failed: " << (err ? err : "unknown error") << "\n";
        close();
        return false;
    }

    node_ = vssApi_->getOutputNode(script_, 0);
    if (!node_) {
        std::cerr << "sk265[error]: No output node set in VapourSynth script (did you call clip.set_output()?)\n";
        close();
        return false;
    }

    const VSVideoInfo* vi = vsApi_->getVideoInfo(node_);
    if (!vi) {
        close();
        return false;
    }

    info_.width = vi->width;
    info_.height = vi->height;
    info_.fpsNum = vi->fpsNum > 0 ? static_cast<int>(vi->fpsNum) : 25;
    info_.fpsDen = vi->fpsDen > 0 ? static_cast<int>(vi->fpsDen) : 1;
    info_.totalFrames = vi->numFrames;
    info_.bitDepth = vi->format.bitsPerSample > 0 ? vi->format.bitsPerSample : 8;

    planeCount_ = 3;
    if (vi->format.colorFamily == cfGray) {
        planeCount_ = 1;
        info_.colorSpace = 0; // X265_CSP_I400
    } else if (vi->format.colorFamily == cfYUV) {
        if (vi->format.subSamplingW == 1 && vi->format.subSamplingH == 1) {
            info_.colorSpace = 1; // X265_CSP_I420
        } else if (vi->format.subSamplingW == 1 && vi->format.subSamplingH == 0) {
            info_.colorSpace = 2; // X265_CSP_I422
        } else if (vi->format.subSamplingW == 0 && vi->format.subSamplingH == 0) {
            info_.colorSpace = 3; // X265_CSP_I444
        } else {
            info_.colorSpace = 1;
        }
    } else {
        info_.colorSpace = 1;
    }

    currentFrameIndex_ = std::max<int64_t>(0, seekFrame_);
    eof_ = (info_.totalFrames > 0 && currentFrameIndex_ >= info_.totalFrames);
    return true;
}

std::optional<VideoFrame> VapourSynthInput::readFrame() {
    if (!node_ || eof_ || (info_.totalFrames > 0 && currentFrameIndex_ >= info_.totalFrames)) {
        eof_ = true;
        return std::nullopt;
    }

    char errBuf[512] = {0};
    const VSFrame* vsFrm = vsApi_->getFrame(static_cast<int>(currentFrameIndex_), node_, errBuf, sizeof(errBuf));
    if (!vsFrm) {
        if (errBuf[0] != '\0') {
            std::cerr << "sk265[error]: " << errBuf << " occurred while reading frame " << currentFrameIndex_ << "\n";
        }
        eof_ = true;
        return std::nullopt;
    }

    VideoFrame frame;
    frame.allocate(info_.width, info_.height, info_.bitDepth, info_.colorSpace);
    frame.pts = currentFrameIndex_++;

    int bytesPerSample = (info_.bitDepth > 8) ? 2 : 1;
    for (int p = 0; p < planeCount_; ++p) {
        const uint8_t* src = vsApi_->getReadPtr(vsFrm, p);
        ptrdiff_t srcStride = vsApi_->getStride(vsFrm, p);
        int planeW = vsApi_->getFrameWidth(vsFrm, p);
        int planeH = vsApi_->getFrameHeight(vsFrm, p);
        if (!src) continue;

        int rowBytes = planeW * bytesPerSample;
        uint8_t* dst = const_cast<uint8_t*>(frame.planes[p].data());
        for (int y = 0; y < planeH; ++y) {
            std::memcpy(dst + y * frame.strides[p], src + y * srcStride, rowBytes);
        }
    }

    vsApi_->freeFrame(vsFrm);
    return frame;
}

} // namespace sk265::pipeline::input
