#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace sk265::config {

class ConfigFileParser {
public:
    static std::vector<std::string> parseFile(const std::string& filePath);
    static std::vector<std::string> parseString(const std::string& content);

    static std::filesystem::path getGlobalConfigPath();
    static std::filesystem::path resolveProfilePath(const std::string& nameOrPath);
};

} // namespace sk265::config
