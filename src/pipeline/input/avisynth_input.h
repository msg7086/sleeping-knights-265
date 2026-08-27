#pragma once
#include <string>
#include <memory>
#include "pipeline/input/input_interface.h"
#include "pipeline/input/avisynth_c.h"

namespace sk265::pipeline::input {

struct AvsFuncTable {
    const char* (AVS_CC *avs_clip_get_error)(AVS_Clip*);
    AVS_ScriptEnvironment* (AVS_CC *avs_create_script_environment)(int);
    void (AVS_CC *avs_delete_script_environment)(AVS_ScriptEnvironment*);
    AVS_VideoFrame* (AVS_CC *avs_get_frame)(AVS_Clip*, int);
    const AVS_VideoInfo* (AVS_CC *avs_get_video_info)(AVS_Clip*);
    int (AVS_CC *avs_function_exists)(AVS_ScriptEnvironment*, const char*);
    AVS_Value (AVS_CC *avs_invoke)(AVS_ScriptEnvironment*, const char*, AVS_Value, const char**);
    void (AVS_CC *avs_release_clip)(AVS_Clip*);
    void (AVS_CC *avs_release_value)(AVS_Value);
    void (AVS_CC *avs_release_video_frame)(AVS_VideoFrame*);
    AVS_Clip* (AVS_CC *avs_take_clip)(AVS_Value, AVS_ScriptEnvironment*);

    int (AVS_CC *avs_is_y8)(const AVS_VideoInfo*);
    int (AVS_CC *avs_is_420)(const AVS_VideoInfo*);
    int (AVS_CC *avs_is_422)(const AVS_VideoInfo*);
    int (AVS_CC *avs_is_444)(const AVS_VideoInfo*);
    int (AVS_CC *avs_bits_per_component)(const AVS_VideoInfo*);

    const uint8_t* (AVS_CC *avs_get_read_ptr)(AVS_VideoFrame*);
    const uint8_t* (AVS_CC *avs_get_read_ptr_p)(AVS_VideoFrame*, int);
    int (AVS_CC *avs_get_pitch)(AVS_VideoFrame*);
    int (AVS_CC *avs_get_pitch_p)(AVS_VideoFrame*, int);
    int (AVS_CC *avs_get_row_size)(AVS_VideoFrame*);
    int (AVS_CC *avs_get_height)(AVS_VideoFrame*);
};

class AviSynthInput : public IInput {
public:
    AviSynthInput();
    ~AviSynthInput() override;

    [[nodiscard]] std::string_view getTag() const noexcept override { return "avs+"; }
    void setCustomLibraryPath(const std::string& customLibPath) { customLibPath_ = customLibPath; }
    void setSeekFrame(int64_t seekFrame) { seekFrame_ = seekFrame; }

    bool open(const std::string& path) override;
    InputInfo getInfo() const override { return info_; }
    std::optional<VideoFrame> readFrame() override;
    bool isEof() const override { return eof_ || currentFrameIndex_ >= info_.totalFrames; }
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
    AvsFuncTable func_{};
    AVS_ScriptEnvironment* env_{nullptr};
    AVS_Clip* clip_{nullptr};
};

} // namespace sk265::pipeline::input
