#include "config/cascading_config.h"
#include "config/config_file_parser.h"
#include "utils/cpu_features.h"
#include <iostream>

namespace sk265::config {

CliOptions CascadingConfig::resolve(
    const std::vector<std::string>& cliArgs,
    const std::string& customGlobalPath
) {
    bool noGlobalConfig = false;
    std::vector<std::string> profileConfigs;

    for (size_t i = 1; i < cliArgs.size(); ++i) {
        const auto& arg = cliArgs[i];
        if (arg == "--no-global-config") {
            noGlobalConfig = true;
        } else if (arg == "--config" || arg == "--profile-config") {
            if (i + 1 < cliArgs.size()) {
                profileConfigs.push_back(cliArgs[++i]);
            }
        } else if (arg.rfind("--config=", 0) == 0) {
            profileConfigs.push_back(arg.substr(9));
        } else if (arg.rfind("--profile-config=", 0) == 0) {
            profileConfigs.push_back(arg.substr(17));
        }
    }

    std::vector<std::string> combinedArgs;
    combinedArgs.push_back(!cliArgs.empty() ? cliArgs[0] : "sk265");

    // Tier 1: Global default configuration (~/.config/sk265/default.txt)
    if (!noGlobalConfig) {
        std::filesystem::path globalPath = !customGlobalPath.empty()
            ? std::filesystem::path(customGlobalPath)
            : ConfigFileParser::getGlobalConfigPath();

        if (!globalPath.empty() && std::filesystem::exists(globalPath)) {
            auto globalTokens = ConfigFileParser::parseFile(globalPath.string());
            if (!globalTokens.empty()) {
                std::cerr << "sk265[info]: Loaded global config from " << globalPath.string() << ":\n"
                          << "sk265[info]:  ";
                for (const auto& tok : globalTokens) {
                    std::cerr << " " << tok;
                }
                std::cerr << "\n";
            }
            combinedArgs.insert(combinedArgs.end(), globalTokens.begin(), globalTokens.end());
        }
    }

    // Tier 2: Profile configuration files (--config profile.txt)
    for (const auto& profile : profileConfigs) {
        auto profilePath = ConfigFileParser::resolveProfilePath(profile);
        if (!profilePath.empty() && std::filesystem::exists(profilePath)) {
            auto profileTokens = ConfigFileParser::parseFile(profilePath.string());
            if (!profileTokens.empty()) {
                std::cerr << "sk265[info]: Loaded profile config from " << profilePath.string() << ":\n"
                          << "sk265[info]:  ";
                for (const auto& tok : profileTokens) {
                    std::cerr << " " << tok;
                }
                std::cerr << "\n";
            }
            combinedArgs.insert(combinedArgs.end(), profileTokens.begin(), profileTokens.end());
        } else {
            std::cerr << "sk265[warning]: Unable to find profile config file: " << profile << "\n";
        }
    }

    // Tier 3: Command-line explicit arguments (highest priority)
    for (size_t i = 1; i < cliArgs.size(); ++i) {
        const auto& arg = cliArgs[i];
        if (arg == "--no-global-config") continue;
        if (arg == "--config" || arg == "--profile-config") {
            ++i; // skip value
            continue;
        }
        if (arg.rfind("--config=", 0) == 0 || arg.rfind("--profile-config=", 0) == 0) {
            continue;
        }
        combinedArgs.push_back(arg);
    }

    auto opts = CliParser::parse(combinedArgs);

    // Hardware-aware default: enable avx512 on AVX512_BF16 capable CPUs if not explicitly specified
    if (opts.encoderParams.find("asm") == opts.encoderParams.end()) {
        if (utils::CpuFeatures::detect().shouldDefaultEnableAvx512()) {
            opts.encoderParams["asm"] = "avx512";
        }
    }

    return opts;
}

CliOptions CascadingConfig::resolve(
    int argc,
    char** argv,
    const std::string& customGlobalPath
) {
    std::vector<std::string> args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    return resolve(args, customGlobalPath);
}

} // namespace sk265::config
