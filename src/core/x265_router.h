#pragma once
#include <cstdint>
#include "core/x265_api.h"

namespace sk265::core {

class CoreRouter {
public:
    static const x265_api* getApi(int bitDepth);
};

} // namespace sk265::core
