#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include "config/config_file_parser.h"

TEST_CASE("ConfigFileParser parses text files with comments, quotes and flags", "[config]") {
    std::string tempConfig = "test_temp_config.txt";
    {
        std::ofstream out(tempConfig);
        out << "# This is a comment\n";
        out << "// Another comment format\n";
        out << "\n";
        out << "--preset slow\n";
        out << "--crf 18 # inline comment\n";
        out << "--aq-mode 3\n";
        out << "--tune \"film and grain\"\n";
        out << "--qg-size=16\n";
        out << "-D 10\n";
    }

    auto tokens = sk265::config::ConfigFileParser::parseFile(tempConfig);
    std::filesystem::remove(tempConfig);

    std::vector<std::string> expected = {
        "--preset", "slow",
        "--crf", "18",
        "--aq-mode", "3",
        "--tune", "film and grain",
        "--qg-size=16",
        "-D", "10"
    };

    REQUIRE(tokens == expected);
}

TEST_CASE("ConfigFileParser resolves profile paths vs identifiers strictly", "[config]") {
    std::string tempProfile = "test_custom_profile.txt";
    {
        std::ofstream out(tempProfile);
        out << "--crf 21\n";
    }

    // 1. Explicit path with extension matches the file
    auto resolvedWithExt = sk265::config::ConfigFileParser::resolveProfilePath("test_custom_profile.txt");
    REQUIRE(resolvedWithExt == "test_custom_profile.txt");

    // 2. Pure identifier without extension strictly looks in ~/.config/sk265/ and does NOT touch local test_custom_profile.txt
    auto resolvedIdentifier = sk265::config::ConfigFileParser::resolveProfilePath("test_custom_profile");
    REQUIRE(resolvedIdentifier.empty());

    std::filesystem::remove(tempProfile);
}

TEST_CASE("ConfigFileParser handles non-existent file gracefully", "[config]") {
    auto tokens = sk265::config::ConfigFileParser::parseFile("non_existent_file_xyz_123.txt");
    REQUIRE(tokens.empty());
}
