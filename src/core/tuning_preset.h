#pragma once
#include <string>
#include "core/x265_api.h"

namespace sk265::core {

class TuningPreset {
public:
    static bool isCustomTune(const std::string& tuneName);
    static void apply(x265_param* param, const std::string& tuneName);
};

} // namespace sk265::core
