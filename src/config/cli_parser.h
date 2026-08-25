#pragma once
#include <vector>
#include <string>
#include "config/cli_options.h"

namespace sk265::config {

class CliParser {
public:
    static CliOptions parse(const std::vector<std::string>& args);
    static CliOptions parse(int argc, char** argv);
    static void printHelp();
};

} // namespace sk265::config
