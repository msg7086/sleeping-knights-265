#pragma once
#include <string>
#include <memory>
#include "pipeline/output/output_interface.h"

namespace sk265::pipeline::output {

struct MuxerInstance {
    std::unique_ptr<IOutput> output;
    std::string muxerName;
    bool bAnnexB{true};
    bool bRepeatHeaders{true};
};

struct OutputFactoryResult {
    bool success{false};
    std::string errorMessage;
    MuxerInstance instance;

    bool has_value() const noexcept { return success; }
    const std::string& error() const noexcept { return errorMessage; }
    MuxerInstance& value() noexcept { return instance; }
};

class OutputFactory {
public:
    static OutputFactoryResult create(const std::string& muxerParam, const std::string& outputPath);
};

} // namespace sk265::pipeline::output
