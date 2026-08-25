#pragma once
#include <string>
#include <memory>
#include <optional>
#include "pipeline/input/input_interface.h"

namespace sk265::pipeline::input {

class LavfInput : public IInput {
public:
    LavfInput();
    ~LavfInput() override;

    void setSeekFrame(int64_t seekFrame);

    bool open(const std::string& path) override;
    InputInfo getInfo() const override;
    std::optional<VideoFrame> readFrame() override;
    bool isEof() const override;
    void close() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sk265::pipeline::input
