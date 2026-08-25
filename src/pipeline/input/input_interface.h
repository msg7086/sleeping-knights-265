#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include "pipeline/frame.h"

namespace sk265::pipeline::input {

struct InputInfo {
    int width{0};
    int height{0};
    int bitDepth{8};
    int colorSpace{0};
    int fpsNum{25};
    int fpsDen{1};
    int64_t totalFrames{0};
};

class IInput {
public:
    virtual ~IInput() = default;
    virtual bool open(const std::string& path) = 0;
    virtual InputInfo getInfo() const = 0;
    virtual std::optional<VideoFrame> readFrame() = 0;
    virtual bool isEof() const = 0;
    virtual void close() = 0;
};

} // namespace sk265::pipeline::input
