#include "config/cli_parser.h"
#include <iostream>

namespace sk265::config {

CliOptions CliParser::parse(const std::vector<std::string>& args) {
    CliOptions opts;

    auto isValue = [](const std::string& s) -> bool {
        if (s.empty()) return false;
        if (s[0] != '-') return true;
        // Negative number or offset (e.g. -2, -2:-2, -0.5) is a value, not a flag
        if (s.size() > 1 && (std::isdigit(static_cast<unsigned char>(s[1])) || s[1] == '.')) {
            return true;
        }
        return false;
    };

    auto getNext = [&](size_t& idx) -> std::string {
        if (idx + 1 < args.size() && isValue(args[idx + 1])) return args[++idx];
        return "";
    };

    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& arg = args[i];

        if (arg == "-h" || arg == "--help") {
            opts.showHelp = true;
        } else if (arg == "-V" || arg == "--version") {
            opts.showVersion = true;
        } else if (arg == "--no-progress") {
            opts.bProgress = false;
        } else if (arg == "--progress") {
            opts.bProgress = true;
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
        } else if (arg == "--avs-lib") {
            opts.avsLibPath = getNext(i);
        } else if (arg == "--vpy-lib") {
            opts.vpyLibPath = getNext(i);
        } else if (arg == "--queue-size") {
            opts.queueConfig = QueueSizeConfig::parse(getNext(i));
        } else if (arg == "--dolby-vision-rpu") {
            opts.doviRpuPath = getNext(i);
        } else if (arg == "--qpfile") {
            opts.qpfilePath = getNext(i);
        } else if (arg == "-p" || arg == "--preset") {
            opts.encoderParams["preset"] = getNext(i);
        } else if (arg == "-t" || arg == "--tune") {
            opts.encoderParams["tune"] = getNext(i);
        } else if (arg == "-P" || arg == "--profile") {
            opts.encoderParams["profile"] = getNext(i);
        } else if (arg == "-F" || arg == "--frame-threads") {
            opts.encoderParams["frame-threads"] = getNext(i);
        } else if (arg == "-r" || arg == "--recon") {
            opts.encoderParams["recon"] = getNext(i);
        } else if (arg == "-I" || arg == "--keyint") {
            opts.encoderParams["keyint"] = getNext(i);
        } else if (arg == "-b" || arg == "--bframes") {
            opts.encoderParams["bframes"] = getNext(i);
        } else if (arg == "-s" || arg == "--ctu") {
            opts.encoderParams["ctu"] = getNext(i);
        } else if (arg == "-q" || arg == "--qp") {
            opts.encoderParams["qp"] = getNext(i);
        } else if (arg == "-m" || arg == "--subme") {
            opts.encoderParams["subme"] = getNext(i);
        } else if (arg == "-w" || arg == "--weightp") {
            opts.encoderParams["weightp"] = "1";
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
                if (i + 1 < args.size() && isValue(args[i + 1])) {
                    val = args[++i];
                }
                opts.encoderParams[key] = val;
            }
        } else if (arg[0] != '-') {
            // Positional argument support: infile [outfile]
            if (opts.inputPath.empty()) {
                opts.inputPath = arg;
            } else if (opts.outputPath.empty()) {
                opts.outputPath = arg;
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
