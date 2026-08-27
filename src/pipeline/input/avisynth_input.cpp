#include "pipeline/input/avisynth_input.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace sk265::pipeline::input {

AviSynthInput::AviSynthInput() = default;

AviSynthInput::~AviSynthInput() {
    close();
}

void AviSynthInput::close() {
    if (clip_ && func_.avs_release_clip) {
        func_.avs_release_clip(clip_);
        clip_ = nullptr;
    }
    if (env_ && func_.avs_delete_script_environment) {
        func_.avs_delete_script_environment(env_);
        env_ = nullptr;
    }
    if (libHandle_) {
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(libHandle_));
#else
        dlclose(libHandle_);
#endif
        libHandle_ = nullptr;
    }
    eof_ = true;
}

bool AviSynthInput::loadLibrary(const std::string& customLibPath) {
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
        libHandle_ = LoadLibraryW(L"AviSynth.dll");
        if (!libHandle_) libHandle_ = LoadLibraryW(L"avisynth.dll");
    }
    auto getSym = [this](const char* name) -> void* {
        return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(libHandle_), name));
    };
#else
    if (!customLibPath.empty()) {
        libHandle_ = dlopen(customLibPath.c_str(), RTLD_NOW);
    } else {
#ifdef __APPLE__
        libHandle_ = dlopen("libavisynth.dylib", RTLD_NOW);
#else
        libHandle_ = dlopen("libavisynth.so", RTLD_NOW);
#endif
    }
    auto getSym = [this](const char* name) -> void* {
        return dlsym(libHandle_, name);
    };
#endif

    if (!libHandle_) return false;

#define LOAD_FUNC(name) \
    func_.name = reinterpret_cast<decltype(func_.name)>(getSym(#name)); \
    if (!func_.name) { close(); return false; }

    LOAD_FUNC(avs_clip_get_error);
    LOAD_FUNC(avs_create_script_environment);
    LOAD_FUNC(avs_delete_script_environment);
    LOAD_FUNC(avs_get_frame);
    LOAD_FUNC(avs_get_video_info);
    LOAD_FUNC(avs_function_exists);
    LOAD_FUNC(avs_invoke);
    LOAD_FUNC(avs_release_clip);
    LOAD_FUNC(avs_release_value);
    LOAD_FUNC(avs_release_video_frame);
    LOAD_FUNC(avs_take_clip);

    LOAD_FUNC(avs_is_y8);
    LOAD_FUNC(avs_is_420);
    LOAD_FUNC(avs_is_422);
    LOAD_FUNC(avs_is_444);
    LOAD_FUNC(avs_bits_per_component);

    // Modern AVS+ pointer and pitch extraction functions
    func_.avs_get_read_ptr_p = reinterpret_cast<decltype(func_.avs_get_read_ptr_p)>(getSym("avs_get_read_ptr_p"));
    func_.avs_get_pitch_p = reinterpret_cast<decltype(func_.avs_get_pitch_p)>(getSym("avs_get_pitch_p"));
    func_.avs_get_read_ptr = reinterpret_cast<decltype(func_.avs_get_read_ptr)>(getSym("avs_get_read_ptr"));
    func_.avs_get_pitch = reinterpret_cast<decltype(func_.avs_get_pitch)>(getSym("avs_get_pitch"));
    func_.avs_get_row_size = reinterpret_cast<decltype(func_.avs_get_row_size)>(getSym("avs_get_row_size"));
    func_.avs_get_height = reinterpret_cast<decltype(func_.avs_get_height)>(getSym("avs_get_height"));

#undef LOAD_FUNC

    return true;
}

bool AviSynthInput::open(const std::string& path) {
    if (path.empty()) return false;

    if (!loadLibrary(customLibPath_)) {
        return false;
    }

    env_ = func_.avs_create_script_environment(AVS_INTERFACE_26);
    if (!env_) {
        close();
        return false;
    }

    if (func_.avs_function_exists(env_, "VersionString")) {
        AVS_Value ver = func_.avs_invoke(env_, "VersionString", avs_new_value_array(nullptr, 0), nullptr);
        if (!avs_is_error(ver) && avs_is_string(ver)) {
            std::cerr << "sk265[info]: " << avs_as_string(ver) << "\n";
        }
        func_.avs_release_value(ver);
    }

    // 1. Try modern AviSynth+ native Unicode: Import(filename, utf8=true)
    AVS_Value u8Args[2];
    const char* u8ArgNames[2] = { nullptr, "utf8" };
    u8Args[0] = avs_new_value_string(path.c_str());
    u8Args[1] = avs_new_value_bool(true);
    AVS_Value res = func_.avs_invoke(env_, "Import", avs_new_value_array(u8Args, 2), u8ArgNames);

    // 2. Fallback to system ACP Import if utf8=true is not recognized or failed
    if (avs_is_error(res) || !avs_is_clip(res)) {
        func_.avs_release_value(res);
        std::string scriptPath = path;
#ifdef _WIN32
        int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
        if (wlen > 0) {
            std::vector<wchar_t> wbuf(wlen);
            MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wbuf.data(), wlen);
            int acpLen = WideCharToMultiByte(CP_THREAD_ACP, 0, wbuf.data(), -1, nullptr, 0, nullptr, nullptr);
            if (acpLen > 0) {
                std::vector<char> acpBuf(acpLen);
                WideCharToMultiByte(CP_THREAD_ACP, 0, wbuf.data(), -1, acpBuf.data(), acpLen, nullptr, nullptr);
                scriptPath = acpBuf.data();
            }
        }
#endif
        AVS_Value fallbackArgs = avs_new_value_string(scriptPath.c_str());
        res = func_.avs_invoke(env_, "Import", fallbackArgs, nullptr);
    }

    if (avs_is_error(res) || !avs_is_clip(res)) {
        if (avs_is_error(res)) {
            std::cerr << "sk265[error]: AviSynth import failed: " << avs_as_string(res) << "\n";
        }
        func_.avs_release_value(res);
        close();
        return false;
    }

    clip_ = func_.avs_take_clip(res, env_);
    func_.avs_release_value(res);

    if (!clip_) {
        close();
        return false;
    }

    const AVS_VideoInfo* vi = func_.avs_get_video_info(clip_);
    if (!vi) {
        close();
        return false;
    }

    info_.width = vi->width;
    info_.height = vi->height;
    info_.fpsNum = vi->fps_numerator > 0 ? vi->fps_numerator : 25;
    info_.fpsDen = vi->fps_denominator > 0 ? vi->fps_denominator : 1;
    info_.totalFrames = vi->num_frames;
    info_.bitDepth = func_.avs_bits_per_component(vi);
    if (info_.bitDepth <= 0) info_.bitDepth = 8;

    planeCount_ = 3;
    if (func_.avs_is_y8(vi)) {
        planeCount_ = 1;
        info_.colorSpace = 0; // X265_CSP_I400
    } else if (func_.avs_is_420(vi)) {
        planeCount_ = 3;
        info_.colorSpace = 1; // X265_CSP_I420
    } else if (func_.avs_is_422(vi)) {
        planeCount_ = 3;
        info_.colorSpace = 2; // X265_CSP_I422
    } else if (func_.avs_is_444(vi)) {
        planeCount_ = 3;
        info_.colorSpace = 3; // X265_CSP_I444
    }

    currentFrameIndex_ = std::max<int64_t>(0, seekFrame_);
    eof_ = (info_.totalFrames > 0 && currentFrameIndex_ >= info_.totalFrames);
    return true;
}

std::optional<VideoFrame> AviSynthInput::readFrame() {
    if (!clip_ || eof_ || (info_.totalFrames > 0 && currentFrameIndex_ >= info_.totalFrames)) {
        eof_ = true;
        return std::nullopt;
    }

    AVS_VideoFrame* avsFrm = func_.avs_get_frame(clip_, static_cast<int>(currentFrameIndex_));
    const char* err = func_.avs_clip_get_error(clip_);
    if (err || !avsFrm) {
        if (err) {
            std::cerr << "sk265[error]: " << err << " occurred while reading frame " << currentFrameIndex_ << "\n";
        }
        eof_ = true;
        return std::nullopt;
    }

    VideoFrame frame;
    frame.allocate(info_.width, info_.height, info_.bitDepth, info_.colorSpace);
    frame.pts = currentFrameIndex_++;

    // Copy planar data safely
    int bytesPerSample = (info_.bitDepth > 8) ? 2 : 1;
    static const int avsPlanes[3] = { AVS_PLANAR_Y, AVS_PLANAR_U, AVS_PLANAR_V };
    for (int p = 0; p < planeCount_; ++p) {
        int avsPlane = avsPlanes[p];
        const uint8_t* src = func_.avs_get_read_ptr_p ? func_.avs_get_read_ptr_p(avsFrm, avsPlane) : (p == 0 ? func_.avs_get_read_ptr(avsFrm) : nullptr);
        int srcPitch = func_.avs_get_pitch_p ? func_.avs_get_pitch_p(avsFrm, avsPlane) : (p == 0 ? func_.avs_get_pitch(avsFrm) : 0);
        if (!src) continue;

        int h = info_.height;
        int planeW = info_.width;
        if (p > 0) {
            h = (info_.colorSpace == 1) ? (info_.height / 2) : info_.height;
            planeW = (info_.colorSpace == 3) ? info_.width : (info_.width / 2);
        }
        int rowBytes = planeW * bytesPerSample;

        uint8_t* dst = const_cast<uint8_t*>(frame.planes[p].data());
        for (int y = 0; y < h; ++y) {
            std::memcpy(dst + y * frame.strides[p], src + y * srcPitch, rowBytes);
        }
    }

    func_.avs_release_video_frame(avsFrm);
    return frame;
}

} // namespace sk265::pipeline::input
