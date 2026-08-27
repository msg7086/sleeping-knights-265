#include "utils/log_mirror.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#endif

namespace sk265::utils {

LogMirror::LogMirror() = default;

LogMirror::~LogMirror() {
    stop();
}

bool LogMirror::start(const std::string& logFilePath) {
    if (logFilePath.empty() || isRunning_.load()) {
        return false;
    }
    logFilePath_ = logFilePath;

#if defined(_WIN32)
    std::filesystem::path p(reinterpret_cast<const char8_t*>(logFilePath_.c_str()));
    logFp_ = _wfopen(p.c_str(), L"ab");
#else
    logFp_ = std::fopen(logFilePath_.c_str(), "ab");
#endif

    if (!logFp_) {
        return false;
    }

#ifdef _WIN32
    int fds[2];
    if (_pipe(fds, 65536, _O_BINARY) != 0) {
        std::fclose(logFp_);
        logFp_ = nullptr;
        return false;
    }
    pipeReadFd_ = fds[0];
    pipeWriteFd_ = fds[1];

    origStderrFd_ = _dup(2);
    if (origStderrFd_ < 0) {
        _close(pipeReadFd_);
        _close(pipeWriteFd_);
        std::fclose(logFp_);
        logFp_ = nullptr;
        return false;
    }

    std::fflush(stderr);
    _dup2(pipeWriteFd_, 2);
#else
    int fds[2];
    if (pipe(fds) != 0) {
        std::fclose(logFp_);
        logFp_ = nullptr;
        return false;
    }
    pipeReadFd_ = fds[0];
    pipeWriteFd_ = fds[1];

    origStderrFd_ = dup(2);
    if (origStderrFd_ < 0) {
        close(pipeReadFd_);
        close(pipeWriteFd_);
        std::fclose(logFp_);
        logFp_ = nullptr;
        return false;
    }

    std::fflush(stderr);
    dup2(pipeWriteFd_, 2);
#endif

    isRunning_.store(true);
    stopRequested_.store(false);
    workerThread_ = std::jthread([this]() { workerLoop(); });
    return true;
}

void LogMirror::stop() {
    if (!isRunning_.exchange(false)) {
        return;
    }
    stopRequested_.store(true);

    std::fflush(stderr);
    if (origStderrFd_ >= 0) {
#ifdef _WIN32
        _dup2(origStderrFd_, 2);
        if (pipeWriteFd_ >= 0) {
            _close(pipeWriteFd_);
            pipeWriteFd_ = -1;
        }
#else
        dup2(origStderrFd_, 2);
        if (pipeWriteFd_ >= 0) {
            close(pipeWriteFd_);
            pipeWriteFd_ = -1;
        }
#endif
    }

    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

void LogMirror::workerLoop() {
    char buf[4096];
    std::string lineBuffer;
    bool hasUnflushedData = false;
    auto lastWriteTime = std::chrono::steady_clock::now();

    auto flushLogFile = [&]() {
        if (logFp_ && hasUnflushedData) {
            std::fflush(logFp_);
            hasUnflushedData = false;
        }
    };

    while (!stopRequested_.load()) {
#ifdef _WIN32
        HANDLE hRead = reinterpret_cast<HANDLE>(_get_osfhandle(pipeReadFd_));
        if (hRead == INVALID_HANDLE_VALUE) break;

        DWORD bytesAvail = 0;
        if (!PeekNamedPipe(hRead, nullptr, 0, nullptr, &bytesAvail, nullptr)) {
            break;
        }

        if (bytesAvail == 0) {
            if (hasUnflushedData) {
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastWriteTime).count() >= 200) {
                    flushLogFile();
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        int toRead = static_cast<int>(std::min<DWORD>(bytesAvail, sizeof(buf)));
        int n = _read(pipeReadFd_, buf, toRead);
        if (n <= 0) break;
#else
        struct pollfd pfd;
        pfd.fd = pipeReadFd_;
        pfd.events = POLLIN;
        int ret = poll(&pfd, 1, 50);
        if (ret < 0) break;
        if (ret == 0) {
            if (hasUnflushedData) {
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastWriteTime).count() >= 200) {
                    flushLogFile();
                }
            }
            continue;
        }
        int n = read(pipeReadFd_, buf, sizeof(buf));
        if (n <= 0) break;
#endif

        if (origStderrFd_ >= 0) {
#ifdef _WIN32
            _write(origStderrFd_, buf, n);
#else
            write(origStderrFd_, buf, n);
#endif
        }

        for (int i = 0; i < n; ++i) {
            char ch = buf[i];
            if (ch == '\r') {
                lineBuffer.clear();
            } else if (ch == '\n') {
                if (!lineBuffer.empty()) {
                    std::fwrite(lineBuffer.data(), 1, lineBuffer.size(), logFp_);
                    lineBuffer.clear();
                }
                std::fputc('\n', logFp_);
                hasUnflushedData = true;
                lastWriteTime = std::chrono::steady_clock::now();
            } else {
                lineBuffer.push_back(ch);
            }
        }
    }

#ifdef _WIN32
    while (true) {
        int n = _read(pipeReadFd_, buf, sizeof(buf));
        if (n <= 0) break;
        if (origStderrFd_ >= 0) _write(origStderrFd_, buf, n);
        for (int i = 0; i < n; ++i) {
            char ch = buf[i];
            if (ch == '\r') {
                lineBuffer.clear();
            } else if (ch == '\n') {
                if (!lineBuffer.empty()) {
                    std::fwrite(lineBuffer.data(), 1, lineBuffer.size(), logFp_);
                    lineBuffer.clear();
                }
                std::fputc('\n', logFp_);
                hasUnflushedData = true;
            } else {
                lineBuffer.push_back(ch);
            }
        }
    }
#else
    while (true) {
        int n = read(pipeReadFd_, buf, sizeof(buf));
        if (n <= 0) break;
        if (origStderrFd_ >= 0) write(origStderrFd_, buf, n);
        for (int i = 0; i < n; ++i) {
            char ch = buf[i];
            if (ch == '\r') {
                lineBuffer.clear();
            } else if (ch == '\n') {
                if (!lineBuffer.empty()) {
                    std::fwrite(lineBuffer.data(), 1, lineBuffer.size(), logFp_);
                    lineBuffer.clear();
                }
                std::fputc('\n', logFp_);
                hasUnflushedData = true;
            } else {
                lineBuffer.push_back(ch);
            }
        }
    }
#endif

    if (!lineBuffer.empty() && logFp_) {
        std::fwrite(lineBuffer.data(), 1, lineBuffer.size(), logFp_);
        std::fputc('\n', logFp_);
        hasUnflushedData = true;
    }

    flushLogFile();

    if (logFp_) {
        std::fclose(logFp_);
        logFp_ = nullptr;
    }

#ifdef _WIN32
    if (pipeReadFd_ >= 0) { _close(pipeReadFd_); pipeReadFd_ = -1; }
    if (origStderrFd_ >= 0) { _close(origStderrFd_); origStderrFd_ = -1; }
#else
    if (pipeReadFd_ >= 0) { close(pipeReadFd_); pipeReadFd_ = -1; }
    if (origStderrFd_ >= 0) { close(origStderrFd_); origStderrFd_ = -1; }
#endif
}

} // namespace sk265::utils
