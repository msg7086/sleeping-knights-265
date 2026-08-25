#pragma once
#include <chrono>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdint>
#include <cstdio>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace sk265::utils {

class ConsoleProgress {
public:
    ConsoleProgress(int64_t totalFrames = 0, int fpsNum = 25, int fpsDen = 1, bool enabled = true, bool stylish = false)
        : totalFrames_(totalFrames), fpsNum_(fpsNum > 0 ? fpsNum : 25), fpsDen_(fpsDen > 0 ? fpsDen : 1), enabled_(enabled), stylish_(stylish) {
        startTime_ = std::chrono::steady_clock::now();
        lastUpdateTime_ = startTime_;
    }

    void setEnabled(bool enabled) noexcept { enabled_ = enabled; }
    void setStylish(bool stylish) noexcept { stylish_ = stylish; }
    bool isStylish() const noexcept { return stylish_; }
    void setTotalFrames(int64_t totalFrames) noexcept { totalFrames_ = totalFrames; }

    void printHeader() {
        if (!enabled_ || !stylish_ || headerPrinted_) return;
        headerPrinted_ = true;
        if (totalFrames_ > 0) {
            std::cerr << "         frames        fps    kb/s      elapsed    remain       size   est.size\n";
        } else {
            std::cerr << " frames    fps    kb/s      elapsed       size\n";
        }
        std::cerr.flush();
    }

    void update(int64_t frameNum, uint64_t totalBytes, bool force = false) {
        if (!enabled_ || frameNum <= 0) return;

        if (stylish_ && !headerPrinted_) {
            printHeader();
        }

        auto now = std::chrono::steady_clock::now();
        auto msSinceLast = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime_).count();
        if (!force && msSinceLast < 150) {
            return;
        }
        lastUpdateTime_ = now;

        auto totalElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_).count();
        double elapsedSec = totalElapsedMs > 0 ? (totalElapsedMs / 1000.0) : 0.001;
        double fps = frameNum / elapsedSec;
        double bitrateKbps = (totalBytes * 8.0 / 1000.0) / (frameNum * (static_cast<double>(fpsDen_) / fpsNum_));

        int elapsedTotalSec = static_cast<int>(elapsedSec);
        int elH = elapsedTotalSec / 3600;
        int elM = (elapsedTotalSec % 3600) / 60;
        int elS = elapsedTotalSec % 60;

        int fps_prec = fps > 999.5 ? 0 : fps > 99.5 ? 1 : fps > 9.95 ? 2 : 3;
        int bitrate_prec = bitrateKbps > 9999.5 ? 0 : bitrateKbps > 999.5 ? 1 : 2;

        int file_prec = totalBytes < 1048576000 ? 2 : totalBytes < 10485760000ULL ? 1 : 0;
        double file_num = totalBytes < 1048576 ? (static_cast<double>(totalBytes) / 1024.0) : (static_cast<double>(totalBytes) / 1048576.0);
        const char* file_unit = totalBytes < 1048576 ? "KB" : "MB";
        if (totalBytes >= 1073741824ULL) {
            file_num = static_cast<double>(totalBytes) / 1073741824.0;
            file_unit = "GB";
        }

        char buffer[320];
        char titleBuf[320];

        if (totalFrames_ > 0) {
            double percent = (100.0 * frameNum) / totalFrames_;
            if (percent > 100.0) percent = 100.0;

            int64_t remainingFrames = totalFrames_ - frameNum;
            int etaSec = fps > 0 ? static_cast<int>(remainingFrames / fps) : 0;
            int etaH = etaSec / 3600;
            int etaM = (etaSec % 3600) / 60;
            int etaS = etaSec % 60;

            double estBytes = (static_cast<double>(totalBytes) * totalFrames_) / frameNum;
            int est_prec = estBytes < 1048576000 ? 2 : estBytes < 10485760000ULL ? 1 : 0;
            double est_num = estBytes < 1048576 ? (estBytes / 1024.0) : (estBytes / 1048576.0);
            const char* est_unit = estBytes < 1048576 ? "KB" : "MB";
            if (estBytes >= 1073741824.0) {
                est_num = estBytes / 1073741824.0;
                est_unit = "GB";
            }

            if (stylish_) {
                snprintf(buffer, sizeof(buffer), "[%5.1f%%] %6lld/%-6lld %5.*f  %6.*f   %02d:%02d:%02d   %02d:%02d:%02d  %6.*f %-2s %6.*f %-2s",
                         percent, static_cast<long long>(frameNum), static_cast<long long>(totalFrames_),
                         fps_prec, fps, bitrate_prec, bitrateKbps,
                         elH, elM, elS, etaH, etaM, etaS,
                         file_prec, file_num, file_unit,
                         est_prec, est_num, est_unit);
            } else {
                snprintf(buffer, sizeof(buffer), "[%.1f%%] %lld/%lld frames, %.2f fps, %.2f kb/s, eta %d:%02d:%02d",
                         percent, static_cast<long long>(frameNum), static_cast<long long>(totalFrames_),
                         fps, bitrateKbps, etaH, etaM, etaS);
            }

            snprintf(titleBuf, sizeof(titleBuf), "sk265 [%.1f%%] %lld/%lld frames, %.1f fps, %.1f kb/s, eta %d:%02d:%02d",
                     percent, static_cast<long long>(frameNum), static_cast<long long>(totalFrames_),
                     fps, bitrateKbps, etaH, etaM, etaS);
        } else {
            if (stylish_) {
                snprintf(buffer, sizeof(buffer), "%6lld  %5.*f  %6.*f   %02d:%02d:%02d  %6.*f %-2s",
                         static_cast<long long>(frameNum), fps_prec, fps, bitrate_prec, bitrateKbps,
                         elH, elM, elS, file_prec, file_num, file_unit);
            } else {
                snprintf(buffer, sizeof(buffer), "%lld frames: %.2f fps, %.2f kb/s",
                         static_cast<long long>(frameNum), fps, bitrateKbps);
            }
            snprintf(titleBuf, sizeof(titleBuf), "sk265 %lld frames, %.1f fps, %.1f kb/s",
                     static_cast<long long>(frameNum), fps, bitrateKbps);
        }

        std::cerr << buffer << "   \r";
        std::cerr.flush();

#ifdef _WIN32
        SetConsoleTitleA(titleBuf);
#endif
    }

    void finish(int64_t frameNum = 0, uint64_t totalBytes = 0) {
        if (!enabled_) return;
        if (stylish_) {
            if (frameNum > 0) {
                update(frameNum, totalBytes, true);
            }
            std::cerr << "\n";
        } else {
            std::cerr << "                                                                                \r";
        }
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
    bool stylish_{false};
    bool headerPrinted_{false};
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point lastUpdateTime_;
};

} // namespace sk265::utils
