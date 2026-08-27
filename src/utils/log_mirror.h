#pragma once
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <cstdio>

namespace sk265::utils {

class LogMirror {
public:
    LogMirror();
    ~LogMirror();

    LogMirror(const LogMirror&) = delete;
    LogMirror& operator=(const LogMirror&) = delete;
    LogMirror(LogMirror&&) = delete;
    LogMirror& operator=(LogMirror&&) = delete;

    bool start(const std::string& logFilePath);
    void stop();

    [[nodiscard]] bool isRunning() const noexcept { return isRunning_.load(); }
    [[nodiscard]] const std::string& getLogFilePath() const noexcept { return logFilePath_; }

private:
    void workerLoop();

    std::string logFilePath_;
    FILE* logFp_{nullptr};
    int origStderrFd_{-1};
    int pipeReadFd_{-1};
    int pipeWriteFd_{-1};

    std::atomic<bool> isRunning_{false};
    std::atomic<bool> stopRequested_{false};
    std::jthread workerThread_;
};

} // namespace sk265::utils
