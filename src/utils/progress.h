#pragma once
#include <chrono>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#endif

namespace sk265::utils {

class ConsoleProgress {
public:
    ConsoleProgress(int64_t totalFrames = 0, int fpsNum = 25, int fpsDen = 1, bool enabled = true)
        : totalFrames_(totalFrames), fpsNum_(fpsNum > 0 ? fpsNum : 25), fpsDen_(fpsDen > 0 ? fpsDen : 1), enabled_(enabled) {
        startTime_ = std::chrono::steady_clock::now();
        lastUpdateTime_ = startTime_;
    }

    void setEnabled(bool enabled) noexcept { enabled_ = enabled; }
    void setTotalFrames(int64_t totalFrames) noexcept { totalFrames_ = totalFrames; }

    void update(int64_t frameNum, uint64_t totalBytes, bool force = false) {
        if (!enabled_ || frameNum <= 0) return;

        auto now = std::chrono::steady_clock::now();
        auto msSinceLast = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime_).count();
        if (!force && msSinceLast < 150) {
            return; // Throttle terminal updates to ~150ms
        }
        lastUpdateTime_ = now;

        auto totalElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_).count();
        double elapsedSec = totalElapsedMs > 0 ? (totalElapsedMs / 1000.0) : 0.001;
        double fps = frameNum / elapsedSec;
        double bitrateKbps = (totalBytes * 8.0 / 1000.0) / (frameNum * (static_cast<double>(fpsDen_) / fpsNum_));

        char buffer[256];
        if (totalFrames_ > 0) {
            double percent = (100.0 * frameNum) / totalFrames_;
            if (percent > 100.0) percent = 100.0;

            int64_t remainingFrames = totalFrames_ - frameNum;
            int etaSec = fps > 0 ? static_cast<int>(remainingFrames / fps) : 0;
            int etaH = etaSec / 3600;
            int etaM = (etaSec % 3600) / 60;
            int etaS = etaSec % 60;

            snprintf(buffer, sizeof(buffer), "[%.1f%%] %lld/%lld frames, %.2f fps, %.2f kb/s, eta %d:%02d:%02d",
                     percent, static_cast<long long>(frameNum), static_cast<long long>(totalFrames_),
                     fps, bitrateKbps, etaH, etaM, etaS);
        } else {
            snprintf(buffer, sizeof(buffer), "%lld frames: %.2f fps, %.2f kb/s",
                     static_cast<long long>(frameNum), fps, bitrateKbps);
        }

        std::cerr << buffer << "   \r";
        std::cerr.flush();

#ifdef _WIN32
        std::string title = std::string("sk265 - ") + buffer;
        SetConsoleTitleA(title.c_str());
#endif
    }

    void finish(int64_t /*frameNum*/ = 0) {
        if (!enabled_) return;
        // Overwrite progress line with spaces
        std::cerr << "                                                                                \r";
        std::cerr.flush();
#ifdef _WIN32
        SetConsoleTitleA("sk265");
#endif
    }

    static std::string formatEta(int seconds) {
        int h = seconds / 3600;
        int m = (seconds % 3600) / 60;
        int s = seconds % 60;
        char buf[64];
        snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
        return std::string(buf);
    }

private:
    int64_t totalFrames_{0};
    int fpsNum_{25};
    int fpsDen_{1};
    bool enabled_{true};
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point lastUpdateTime_;
};

} // namespace sk265::utils
