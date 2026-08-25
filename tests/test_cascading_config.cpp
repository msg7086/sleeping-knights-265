#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include "config/cascading_config.h"

TEST_CASE("CascadingConfig verifies priority: default.txt < profile.txt < CLI arguments", "[config][cascade]") {
    std::string globalConfig = "test_global_default.txt";
    std::string profileA = "test_profile_a.txt";

    // 1. Create global default config: preset=fast, crf=24, aq-mode=1, qg-size=32
    {
        std::ofstream out(globalConfig);
        out << "# Global base defaults\n";
        out << "--preset fast\n";
        out << "--crf 24\n";
        out << "--aq-mode 1\n";
        out << "--qg-size 32\n";
    }

    // 2. Create profile A: crf=20, aq-mode=2
    {
        std::ofstream out(profileA);
        out << "# Profile A overrides\n";
        out << "--crf 20\n";
        out << "--aq-mode 2\n";
    }

    // Test Case 1: Only Global Config applied
    {
        std::vector<std::string> args = {"sk265", "-i", "in.y4m", "-o", "out.hevc"};
        auto opts = sk265::config::CascadingConfig::resolve(args, globalConfig);

        REQUIRE(opts.encoderParams["preset"] == "fast");
        REQUIRE(opts.encoderParams["crf"] == "24");
        REQUIRE(opts.encoderParams["aq-mode"] == "1");
        REQUIRE(opts.encoderParams["qg-size"] == "32");
    }

    // Test Case 2: Global Config + Profile A applied (profile overrides crf & aq-mode)
    {
        std::vector<std::string> args = {
            "sk265",
            "--config", profileA,
            "-i", "in.y4m",
            "-o", "out.hevc"
        };
        auto opts = sk265::config::CascadingConfig::resolve(args, globalConfig);

        REQUIRE(opts.encoderParams["preset"] == "fast");  // From global
        REQUIRE(opts.encoderParams["qg-size"] == "32");   // From global
        REQUIRE(opts.encoderParams["crf"] == "20");       // Overridden by Profile A
        REQUIRE(opts.encoderParams["aq-mode"] == "2");    // Overridden by Profile A
    }

    // Test Case 3: Global Config + Profile A + CLI explicit arguments (CLI overrides crf)
    {
        std::vector<std::string> args = {
            "sk265",
            "--config", profileA,
            "-i", "in.y4m",
            "-o", "out.hevc",
            "--crf", "16",
            "--preset", "slow"
        };
        auto opts = sk265::config::CascadingConfig::resolve(args, globalConfig);

        REQUIRE(opts.encoderParams["preset"] == "slow");  // Overridden by CLI
        REQUIRE(opts.encoderParams["crf"] == "16");       // Overridden by CLI
        REQUIRE(opts.encoderParams["aq-mode"] == "2");    // From Profile A
        REQUIRE(opts.encoderParams["qg-size"] == "32");   // From global
    }

    // Test Case 4: --no-global-config skips global config
    {
        std::vector<std::string> args = {
            "sk265",
            "--no-global-config",
            "-i", "in.y4m",
            "-o", "out.hevc",
            "--crf", "18"
        };
        auto opts = sk265::config::CascadingConfig::resolve(args, globalConfig);

        REQUIRE(opts.encoderParams.count("preset") == 0); // Not loaded from global
        REQUIRE(opts.encoderParams.count("qg-size") == 0);
        REQUIRE(opts.encoderParams["crf"] == "18");
    }

    std::filesystem::remove(globalConfig);
    std::filesystem::remove(profileA);
}
