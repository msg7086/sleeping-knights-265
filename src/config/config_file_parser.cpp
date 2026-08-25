#include "config/config_file_parser.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstdlib>

namespace sk265::config {

std::vector<std::string> ConfigFileParser::parseFile(const std::string& filePath) {
    if (filePath.empty()) return {};
    std::ifstream in(filePath);
    if (!in.is_open()) return {};

    std::stringstream buffer;
    buffer << in.rdbuf();
    return parseString(buffer.str());
}

std::vector<std::string> ConfigFileParser::parseString(const std::string& content) {
    std::vector<std::string> tokens;
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        // Strip comments outside quotes
        std::string stripped;
        bool inQuote = false;
        char quoteChar = 0;

        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (!inQuote && (c == '"' || c == '\'')) {
                inQuote = true;
                quoteChar = c;
                stripped += c;
            } else if (inQuote && c == quoteChar) {
                inQuote = false;
                stripped += c;
            } else if (!inQuote && c == '#') {
                break; // comment till end of line
            } else if (!inQuote && c == '/' && i + 1 < line.size() && line[i + 1] == '/') {
                break; // comment till end of line
            } else {
                stripped += c;
            }
        }

        // Tokenize stripped line
        std::string currentToken;
        inQuote = false;
        quoteChar = 0;

        for (size_t i = 0; i < stripped.size(); ++i) {
            char c = stripped[i];
            if (!inQuote && (c == '"' || c == '\'')) {
                inQuote = true;
                quoteChar = c;
            } else if (inQuote && c == quoteChar) {
                inQuote = false;
            } else if (!inQuote && std::isspace(static_cast<unsigned char>(c))) {
                if (!currentToken.empty()) {
                    tokens.push_back(currentToken);
                    currentToken.clear();
                }
            } else {
                currentToken += c;
            }
        }

        if (!currentToken.empty()) {
            tokens.push_back(currentToken);
        }
    }

    return tokens;
}

static std::vector<std::filesystem::path> getConfigBaseDirs() {
    std::vector<std::filesystem::path> baseDirs;
#ifdef _WIN32
    const char* appData = std::getenv("APPDATA");
    if (appData && *appData) {
        baseDirs.push_back(std::filesystem::path(appData) / "sk265");
    }
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile && *userProfile) {
        baseDirs.push_back(std::filesystem::path(userProfile) / ".config" / "sk265");
    }
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        baseDirs.push_back(std::filesystem::path(xdg) / "sk265");
    }
    const char* home = std::getenv("HOME");
    if (home && *home) {
        baseDirs.push_back(std::filesystem::path(home) / ".config" / "sk265");
    }
#endif
    return baseDirs;
}

std::filesystem::path ConfigFileParser::getGlobalConfigPath() {
    for (const auto& dir : getConfigBaseDirs()) {
        auto p1 = dir / "default.txt";
        if (std::filesystem::exists(p1)) return p1;
        auto p2 = dir / "default.conf";
        if (std::filesystem::exists(p2)) return p2;
    }
    return {};
}

std::filesystem::path ConfigFileParser::resolveProfilePath(const std::string& nameOrPath) {
    if (nameOrPath.empty()) return {};

    std::filesystem::path p(nameOrPath);
    bool hasPathSeparator = (nameOrPath.find('/') != std::string::npos || nameOrPath.find('\\') != std::string::npos);
    bool hasExtension = p.has_extension();

    // 1. Explicit path (contains '/' or '\' or has file extension like .txt, .conf)
    if (hasPathSeparator || hasExtension) {
        return p;
    }

    // 2. Pure Identifier (e.g. "anime", "hdr10")
    // Strictly and only check ~/.config/sk265/<name>.txt (or %APPDATA%/sk265/<name>.txt on Windows)
    for (const auto& dir : getConfigBaseDirs()) {
        auto candidate = dir / (nameOrPath + ".txt");
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}

} // namespace sk265::config
