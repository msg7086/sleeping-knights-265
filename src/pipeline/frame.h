#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include <array>
#include <any>

namespace sk265::pipeline {

struct VideoFrame {
    int64_t pts{-1};
    int64_t dts{-1};
    int width{0};
    int height{0};
    int bitDepth{8};
    int colorSpace{1}; // 0 = I400, 1 = I420, 2 = I422, 3 = I444

    std::array<std::span<const uint8_t>, 3> planes{};
    std::array<size_t, 3> strides{0, 0, 0};
    std::vector<uint8_t> buffer;
    std::any handle;

    VideoFrame() = default;

    VideoFrame(VideoFrame&& o) noexcept
        : pts(o.pts), dts(o.dts), width(o.width), height(o.height),
          bitDepth(o.bitDepth), colorSpace(o.colorSpace),
          strides(o.strides), buffer(std::move(o.buffer)), handle(std::move(o.handle)) {
        rebindPlanes();
    }

    VideoFrame& operator=(VideoFrame&& o) noexcept {
        if (this != &o) {
            pts = o.pts;
            dts = o.dts;
            width = o.width;
            height = o.height;
            bitDepth = o.bitDepth;
            colorSpace = o.colorSpace;
            strides = o.strides;
            buffer = std::move(o.buffer);
            handle = std::move(o.handle);
            rebindPlanes();
        }
        return *this;
    }

    VideoFrame(const VideoFrame&) = delete;
    VideoFrame& operator=(const VideoFrame&) = delete;

    void rebindPlanes() {
        if (buffer.empty()) {
            planes[0] = {};
            planes[1] = {};
            planes[2] = {};
            return;
        }
        int bytesPerSample = (bitDepth > 8) ? 2 : 1;
        size_t ySize = static_cast<size_t>(width) * height * bytesPerSample;
        size_t uvWidth = 0;
        size_t uvHeight = 0;
        if (colorSpace == 1) { // I420
            uvWidth = width / 2;
            uvHeight = height / 2;
        } else if (colorSpace == 2) { // I422
            uvWidth = width / 2;
            uvHeight = height;
        } else if (colorSpace == 3) { // I444
            uvWidth = width;
            uvHeight = height;
        }

        size_t uvSize = uvWidth * uvHeight * bytesPerSample;
        uint8_t* ptr = buffer.data();
        planes[0] = std::span<const uint8_t>(ptr, ySize);
        if (uvSize > 0) {
            planes[1] = std::span<const uint8_t>(ptr + ySize, uvSize);
            planes[2] = std::span<const uint8_t>(ptr + ySize + uvSize, uvSize);
        } else {
            planes[1] = {};
            planes[2] = {};
        }
    }

    void allocate(int w, int h, int depth, int cs = 1) {
        width = w;
        height = h;
        bitDepth = depth;
        colorSpace = cs;
        int bytesPerSample = (depth > 8) ? 2 : 1;

        size_t ySize = static_cast<size_t>(w) * h * bytesPerSample;
        size_t uvWidth = 0;
        size_t uvHeight = 0;
        if (cs == 1) { // I420
            uvWidth = w / 2;
            uvHeight = h / 2;
        } else if (cs == 2) { // I422
            uvWidth = w / 2;
            uvHeight = h;
        } else if (cs == 3) { // I444
            uvWidth = w;
            uvHeight = h;
        }

        size_t uvSize = uvWidth * uvHeight * bytesPerSample;

        buffer.resize(ySize + 2 * uvSize);
        strides[0] = static_cast<size_t>(w) * bytesPerSample;
        strides[1] = uvWidth * bytesPerSample;
        strides[2] = uvWidth * bytesPerSample;

        rebindPlanes();
    }
};

} // namespace sk265::pipeline
