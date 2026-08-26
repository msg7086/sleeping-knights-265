#include "core/x265_router.h"
#include "core/x265_handle.h"
#include "core/x265_api.h"

namespace x265_8bit  { const x265_api* x265_api_get(int); }
namespace x265_10bit { const x265_api* x265_api_get(int); }
namespace x265_12bit { const x265_api* x265_api_get(int); }

namespace sk265::core {

void ParamDeleter::operator()(x265_param* p) const {
    if (p && api && api->param_free) api->param_free(p);
}

void PictureDeleter::operator()(x265_picture* p) const {
    if (p && api && api->picture_free) api->picture_free(p);
}

void EncoderDeleter::operator()(x265_encoder* e) const {
    if (e && api && api->encoder_close) api->encoder_close(e);
}

ParamHandle::ParamHandle(const x265_api* a, x265_param* p)
    : api(a), ptr_(p, ParamDeleter{a}) {}

PictureHandle::PictureHandle(const x265_api* a, x265_picture* p)
    : api(a), ptr_(p, PictureDeleter{a}) {}

EncoderHandle::EncoderHandle(const x265_api* a, x265_encoder* e)
    : api(a), ptr_(e, EncoderDeleter{a}) {}

ParamHandle make_param_handle(const x265_api* api) {
    if (!api || !api->param_alloc) return ParamHandle(nullptr, nullptr);
    x265_param* p = api->param_alloc();
    if (p && api->param_default) api->param_default(p);
    return ParamHandle(api, p);
}

PictureHandle make_picture_handle(const x265_api* api) {
    if (!api || !api->picture_alloc) return PictureHandle(nullptr, nullptr);
    return PictureHandle(api, api->picture_alloc());
}

EncoderHandle make_encoder_handle(const x265_api* api, x265_param* param) {
    if (!api || !api->encoder_open || !param) return EncoderHandle(nullptr, nullptr);
    return EncoderHandle(api, api->encoder_open(param));
}

const x265_api* CoreRouter::getApi(int bitDepth) {
    switch (bitDepth) {
        case 8:  return x265_8bit::x265_api_get(0);
        case 10: return x265_10bit::x265_api_get(0);
        case 12: return x265_12bit::x265_api_get(0);
        default: return nullptr;
    }
}

} // namespace sk265::core
