#include "pipeline/input/y4m_input.h"
#include <sstream>
#include <iostream>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace sk265::pipeline::input {

Y4mInput::Y4mInput() = default;
Y4mInput::~Y4mInput() { close(); }

void Y4mInput::close() {
    if (fileStream_ && fileStream_->is_open()) {
        fileStream_->close();
    }
    activeStream_ = nullptr;
    eof_ = true;
}

bool Y4mInput::open(const std::string& path) {
    if (path == "-") {
#ifdef _WIN32
        _setmode(_fileno(stdin), _O_BINARY);
#endif
        return openFromStream(std::cin);
    }

    fileStream_ = std::make_unique<std::ifstream>(path, std::ios::binary);
    if (!fileStream_->is_open()) return false;
    return openFromStream(*fileStream_);
}

bool Y4mInput::openFromStream(std::istream& stream) {
    activeStream_ = &stream;
    eof_ = false;
    currentFrameIndex_ = 0;
    return parseHeader(stream);
}

bool Y4mInput::parseHeader(std::istream& stream) {
    std::string magic;
    stream >> magic;
    if (magic != "YUV4MPEG2") return false;

    info_.fpsNum = 25;
    info_.fpsDen = 1;
    info_.bitDepth = 8;
    info_.colorSpace = 1; // Default: I420

    std::string token;
    while (stream >> token) {
        char tag = token[0];
        std::string val = token.substr(1);
        if (tag == 'W') {
            info_.width = std::stoi(val);
        } else if (tag == 'H') {
            info_.height = std::stoi(val);
        } else if (tag == 'F') {
            size_t colon = val.find(':');
            if (colon != std::string::npos) {
                info_.fpsNum = std::stoi(val.substr(0, colon));
                info_.fpsDen = std::stoi(val.substr(colon + 1));
            }
        } else if (tag == 'C') {
            if (val.find("420p10") != std::string::npos) {
                info_.bitDepth = 10;
                info_.colorSpace = 1;
            } else if (val.find("420p12") != std::string::npos) {
                info_.bitDepth = 12;
                info_.colorSpace = 1;
            } else if (val.find("422p10") != std::string::npos) {
                info_.bitDepth = 10;
                info_.colorSpace = 2;
            } else if (val.find("422p12") != std::string::npos) {
                info_.bitDepth = 12;
                info_.colorSpace = 2;
            } else if (val.find("422") != std::string::npos) {
                info_.bitDepth = 8;
                info_.colorSpace = 2;
            } else if (val.find("444p10") != std::string::npos) {
                info_.bitDepth = 10;
                info_.colorSpace = 3;
            } else if (val.find("444p12") != std::string::npos) {
                info_.bitDepth = 12;
                info_.colorSpace = 3;
            } else if (val.find("444") != std::string::npos) {
                info_.bitDepth = 8;
                info_.colorSpace = 3;
            } else if (val.find("mono") != std::string::npos) {
                info_.bitDepth = 8;
                info_.colorSpace = 0;
            } else {
                info_.bitDepth = 8;
                info_.colorSpace = 1;
            }
        }
        if (stream.peek() == '\n') {
            stream.get();
            break;
        }
    }
    return info_.width > 0 && info_.height > 0;
}

std::optional<VideoFrame> Y4mInput::readFrame() {
    if (!activeStream_ || eof_ || activeStream_->eof()) return std::nullopt;

    std::string frameMagic;
    *activeStream_ >> frameMagic;
    if (frameMagic.rfind("FRAME", 0) != 0) {
        eof_ = true;
        return std::nullopt;
    }

    while (activeStream_->peek() != '\n' && activeStream_->good()) {
        activeStream_->get();
    }
    if (activeStream_->peek() == '\n') activeStream_->get();

    VideoFrame frame;
    frame.allocate(info_.width, info_.height, info_.bitDepth, info_.colorSpace);
    frame.pts = currentFrameIndex_++;

    activeStream_->read(reinterpret_cast<char*>(frame.buffer.data()), frame.buffer.size());
    if (activeStream_->gcount() < static_cast<std::streamsize>(frame.buffer.size())) {
        eof_ = true;
        return std::nullopt;
    }

    return frame;
}

} // namespace sk265::pipeline::input
