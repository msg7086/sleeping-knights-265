#pragma once
#include <atomic>
#include <csignal>

#ifdef _WIN32
#include <windows.h>
#endif

namespace sk265::utils {

inline std::atomic<bool> g_interrupted{false};

#ifdef _WIN32
inline BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
    switch (ctrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
            g_interrupted.store(true);
            return TRUE;
        default:
            return FALSE;
    }
}
#else
inline void posixSignalHandler(int) {
    g_interrupted.store(true);
}
#endif

inline void installSignalHandler() {
    g_interrupted.store(false);
#ifdef _WIN32
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#else
    std::signal(SIGINT, posixSignalHandler);
    std::signal(SIGTERM, posixSignalHandler);
#endif
}

inline bool isInterrupted() noexcept {
    return g_interrupted.load();
}

inline void requestStop() noexcept {
    g_interrupted.store(true);
}

} // namespace sk265::utils
