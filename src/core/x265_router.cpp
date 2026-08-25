#include "core/x265_router.h"
#include "core/x265_handle.h"
#include "core/x265_api.h"

namespace x265_8bit  { const x265_api* x265_api_get(int); }
namespace x265_10bit {
const x265_api* x265_api_get(int);
void x265_alloc_analysis_data(x265_param* param, x265_analysis_data* analysis);
void x265_free_analysis_data(x265_param* param, x265_analysis_data* analysis);
x265_picture* x265_picture_alloc();
void x265_picture_init(x265_param* param, x265_picture* pic);
void x265_picture_free(x265_picture* pic);
int x265_encoder_reconfig(x265_encoder* encoder, x265_param* param);
int x265_param_parse(x265_param* p, const char* name, const char* value);
x265_zone* x265_zone_alloc(int zoneCount, int isZoneFile);
void x265_zone_free(x265_param* param);
FILE* x265_csvlog_open(const x265_param* param);
void x265_csvlog_frame(const x265_param* param, const x265_picture* pic);
void x265_csvlog_encode(const x265_param* param, const x265_stats* stats, int padx, int pady, int argc, char** argv);
void x265_dither_image(x265_picture* pic, int picWidth, int picHeight, int16_t* errorBuf, int bitDepth);
}
namespace x265_12bit { const x265_api* x265_api_get(int); }

extern "C" {

void x265_alloc_analysis_data(x265_param* param, x265_analysis_data* analysis) {
    x265_10bit::x265_alloc_analysis_data(param, analysis);
}

void x265_free_analysis_data(x265_param* param, x265_analysis_data* analysis) {
    x265_10bit::x265_free_analysis_data(param, analysis);
}

x265_picture* x265_picture_alloc() {
    return x265_10bit::x265_picture_alloc();
}

void x265_picture_init(x265_param* param, x265_picture* pic) {
    x265_10bit::x265_picture_init(param, pic);
}

void x265_picture_free(x265_picture* pic) {
    x265_10bit::x265_picture_free(pic);
}

int x265_encoder_reconfig(x265_encoder* encoder, x265_param* param) {
    return x265_10bit::x265_encoder_reconfig(encoder, param);
}

int x265_param_parse(x265_param* p, const char* name, const char* value) {
    return x265_10bit::x265_param_parse(p, name, value);
}

x265_zone* x265_zone_alloc(int zoneCount, int isZoneFile) {
    return x265_10bit::x265_zone_alloc(zoneCount, isZoneFile);
}

void x265_zone_free(x265_param* param) {
    x265_10bit::x265_zone_free(param);
}

FILE* x265_csvlog_open(const x265_param* param) {
    return x265_10bit::x265_csvlog_open(param);
}

void x265_csvlog_frame(const x265_param* param, const x265_picture* pic) {
    x265_10bit::x265_csvlog_frame(param, pic);
}

void x265_csvlog_encode(const x265_param* param, const x265_stats* stats, int padx, int pady, int argc, char** argv) {
    x265_10bit::x265_csvlog_encode(param, stats, padx, pady, argc, argv);
}

void x265_dither_image(x265_picture* pic, int picWidth, int picHeight, int16_t* errorBuf, int bitDepth) {
    x265_10bit::x265_dither_image(pic, picWidth, picHeight, errorBuf, bitDepth);
}

} // extern "C"

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
