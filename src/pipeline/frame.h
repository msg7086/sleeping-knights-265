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
    int colorSpace{0}; // 0 = YUV420

    std::array<std::span<const uint8_t>, 3> planes{};
    std::array<size_t, 3> strides{0, 0, 0};
    std::vector<uint8_t> buffer;
    std::any handle;

    void allocate(int w, int h, int depth, int cs = 0) {
        width = w;
        height = h;
        bitDepth = depth;
        colorSpace = cs;
        int bytesPerSample = (depth > 8) ? 2 : 1;

        size_t ySize = static_cast<size_t>(w) * h * bytesPerSample;
        size_t uvWidth = (cs == 0 || cs == 1) ? (w / 2) : w;
        size_t uvHeight = (cs == 0) ? (h / 2) : h;
        size_t uvSize = uvWidth * uvHeight * bytesPerSample;

        buffer.resize(ySize + 2 * uvSize);
        strides[0] = static_cast<size_t>(w) * bytesPerSample;
        strides[1] = uvWidth * bytesPerSample;
        strides[2] = uvWidth * bytesPerSample;

        uint8_t* ptr = buffer.data();
        planes[0] = std::span<const uint8_t>(ptr, ySize);
        planes[1] = std::span<const uint8_t>(ptr + ySize, uvSize);
        planes[2] = std::span<const uint8_t>(ptr + ySize + uvSize, uvSize);
    }
};

} // namespace sk265::pipeline
