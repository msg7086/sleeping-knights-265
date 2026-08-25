#include "config/cli_parser.h"
#include <iostream>

namespace sk265::config {

CliOptions CliParser::parse(const std::vector<std::string>& args) {
    CliOptions opts;
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& arg = args[i];

        auto getNext = [&](size_t& idx) -> std::string {
            if (idx + 1 < args.size()) return args[++idx];
            return "";
        };

        if (arg == "-h" || arg == "--help") {
            opts.showHelp = true;
        } else if (arg == "-V" || arg == "--version") {
            opts.showVersion = true;
        } else if (arg == "-i" || arg == "--input") {
            opts.inputPath = getNext(i);
        } else if (arg == "-o" || arg == "--output") {
            opts.outputPath = getNext(i);
        } else if (arg == "-D" || arg == "--output-depth") {
            std::string val = getNext(i);
            if (!val.empty()) opts.bitDepth = std::stoi(val);
        } else if (arg == "-f" || arg == "--frames") {
            std::string val = getNext(i);
            if (!val.empty()) opts.frameCount = std::stoi(val);
        } else if (arg == "--seek") {
            std::string val = getNext(i);
            if (!val.empty()) opts.seekFrame = std::stoi(val);
        } else if (arg == "-p" || arg == "--preset") {
            opts.encoderParams["preset"] = getNext(i);
        } else if (arg == "-t" || arg == "--tune") {
            opts.encoderParams["tune"] = getNext(i);
        } else if (arg == "--crf") {
            opts.encoderParams["crf"] = getNext(i);
        } else if (arg.rfind("--", 0) == 0) {
            std::string raw = arg.substr(2);
            size_t eqPos = raw.find('=');
            if (eqPos != std::string::npos) {
                std::string key = raw.substr(0, eqPos);
                std::string val = raw.substr(eqPos + 1);
                opts.encoderParams[key] = val;
            } else {
                std::string key = raw;
                std::string val = "true";
                if (i + 1 < args.size() && args[i + 1].rfind("-", 0) != 0) {
                    val = args[++i];
                }
                opts.encoderParams[key] = val;
            }
        }
    }
    return opts;
}

CliOptions CliParser::parse(int argc, char** argv) {
    std::vector<std::string> args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    return parse(args);
}

void CliParser::printHelp() {
    std::cout << "sk265 CLI - Modern Decoupled x265 Standalone Frontend (MVP)\n\n"
              << "Usage: sk265 [options] -i <input.y4m|script.avs> -o <output.hevc>\n\n"
              << "Frontend Options:\n"
              << "  -i, --input <file>        Input Y4M video stream or AviSynth+ script (.avs)\n"
              << "  -o, --output <file>       Output Annex-B HEVC bitstream\n"
              << "  -D, --output-depth <int>  Encoding bit depth (8, 10, 12) [default: 8]\n"
              << "  -f, --frames <int>        Max frames to encode [default: all]\n"
              << "      --seek <int>          First frame to encode [default: 0]\n"
              << "  -h, --help                Show this help message\n"
              << "  -V, --version             Show version information\n\n"
              << "Encoder Parameters (forwarded directly to x265_param_parse):\n"
              << "  -p, --preset <string>     Encoder preset (ultrafast..placebo)\n"
              << "  -t, --tune <string>       Encoder tune (film, animation, grain, ...)\n"
              << "      --crf <float>         Constant Rate Factor\n"
              << "      --<param> <value>     Any valid x265 parameter (e.g. --aq-mode 3)\n";
}

} // namespace sk265::config
