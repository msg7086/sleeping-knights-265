#pragma once
#include <istream>
#include <fstream>
#include <memory>
#include "pipeline/input/input_interface.h"

namespace sk265::pipeline::input {

class Y4mInput : public IInput {
public:
    Y4mInput();
    ~Y4mInput() override;

    [[nodiscard]] std::string_view getTag() const noexcept override { return "y4m "; }
    bool open(const std::string& path) override;
    bool openFromStream(std::istream& stream);
    InputInfo getInfo() const override { return info_; }
    std::optional<VideoFrame> readFrame() override;
    bool isEof() const override { return eof_; }
    void close() override;

private:
    bool parseHeader(std::istream& stream);

    InputInfo info_{};
    bool eof_{false};
    int64_t currentFrameIndex_{0};
    std::unique_ptr<std::ifstream> fileStream_;
    std::istream* activeStream_{nullptr};
};

} // namespace sk265::pipeline::input
