#pragma once
#include <cstdint>

struct x265_api;

namespace sk265::core {

class CoreRouter {
public:
    static const x265_api* getApi(int bitDepth);
};

} // namespace sk265::core
