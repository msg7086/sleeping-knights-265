#pragma once
#include <string>
#include <memory>
#include <optional>
#include "pipeline/input/input_interface.h"
#include "pipeline/input/vapoursynth_c.h"

namespace sk265::pipeline::input {

class VapourSynthInput : public IInput {
public:
    VapourSynthInput();
    ~VapourSynthInput() override;

    void setCustomLibraryPath(const std::string& customLibPath) { customLibPath_ = customLibPath; }
    void setSeekFrame(int64_t seekFrame) { seekFrame_ = seekFrame; }

    bool open(const std::string& path) override;
    InputInfo getInfo() const override { return info_; }
    std::optional<VideoFrame> readFrame() override;
    bool isEof() const override { return eof_ || (info_.totalFrames > 0 && currentFrameIndex_ >= info_.totalFrames); }
    void close() override;

private:
    bool loadLibrary(const std::string& customLibPath = "");

    InputInfo info_{};
    bool eof_{false};
    int64_t currentFrameIndex_{0};
    int64_t seekFrame_{0};
    int planeCount_{3};
    std::string customLibPath_;

    void* libHandle_{nullptr};
    const VSSCRIPTAPI* vssApi_{nullptr};
    const VSAPI* vsApi_{nullptr};
    VSScript* script_{nullptr};
    VSNode* node_{nullptr};
};

} // namespace sk265::pipeline::input
