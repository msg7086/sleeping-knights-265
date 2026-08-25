#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "config/cli_options.h"
#include "config/cli_parser.h"

namespace sk265::config {

class CascadingConfig {
public:
    static CliOptions resolve(
        const std::vector<std::string>& cliArgs,
        const std::string& customGlobalPath = ""
    );

    static CliOptions resolve(
        int argc,
        char** argv,
        const std::string& customGlobalPath = ""
    );
};

} // namespace sk265::config
