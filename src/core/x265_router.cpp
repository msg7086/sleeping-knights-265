#include "core/x265_router.h"
#include "core/x265_handle.h"

struct x265_param {
    int bitDepth;
    int sourceWidth;
    int sourceHeight;
    int fpsNum;
    int fpsDenom;
    int internalCsp;
};

struct x265_picture {
    int colorSpace;
    int bitDepth;
    void* planes[3];
    int stride[3];
    int64_t pts;
    int64_t dts;
};

struct x265_encoder {
    int bitDepth;
};

struct x265_api {
    int api_version;
    int bit_depth;
    x265_param* (*param_alloc)();
    void (*param_free)(x265_param*);
    void (*param_default)(x265_param*);
    int (*param_default_preset)(x265_param*, const char*, const char*);
    int (*param_parse)(x265_param*, const char*, const char*);
    x265_picture* (*picture_alloc)();
    void (*picture_free)(x265_picture*);
    void (*picture_init)(x265_param*, x265_picture*);
    x265_encoder* (*encoder_open)(x265_param*);
    int (*encoder_headers)(x265_encoder*, x265_nal**, uint32_t*);
    int (*encoder_encode)(x265_encoder*, x265_nal**, uint32_t*, x265_picture*, x265_picture*);
    void (*encoder_close)(x265_encoder*);
    char* (*param2string)(x265_param*, int);
};

namespace x265_8bit {
    static x265_param* alloc_p() { return new x265_param{8, 0, 0, 25, 1, 1}; }
    static void free_p(x265_param* p) { delete p; }
    static void def_p(x265_param* p) { if (p) { p->bitDepth = 8; p->fpsNum = 25; p->fpsDenom = 1; } }
    static int def_preset(x265_param*, const char*, const char*) { return 0; }
    static int parse_p(x265_param*, const char*, const char*) { return 0; }
    static x265_picture* alloc_pic() { return new x265_picture{1, 8, {nullptr, nullptr, nullptr}, {0, 0, 0}, 0, 0}; }
    static void free_pic(x265_picture* p) { delete p; }
    static void init_pic(x265_param*, x265_picture*) {}
    static x265_encoder* enc_open(x265_param*) { return new x265_encoder{8}; }
    static int enc_headers(x265_encoder*, x265_nal**, uint32_t* n) { if (n) *n = 0; return 0; }
    static int enc_encode(x265_encoder*, x265_nal**, uint32_t* n, x265_picture*, x265_picture*) { if (n) *n = 0; return 0; }
    static void enc_close(x265_encoder* e) { delete e; }
    static char* p2s(x265_param*, int) { return nullptr; }

    static const x265_api api_8bit = {
        212, 8, alloc_p, free_p, def_p, def_preset, parse_p,
        alloc_pic, free_pic, init_pic, enc_open, enc_headers, enc_encode, enc_close, p2s
    };
    const x265_api* x265_api_get(int) { return &api_8bit; }
}

namespace x265_10bit {
    static x265_param* alloc_p() { return new x265_param{10, 0, 0, 25, 1, 1}; }
    static void free_p(x265_param* p) { delete p; }
    static void def_p(x265_param* p) { if (p) { p->bitDepth = 10; p->fpsNum = 25; p->fpsDenom = 1; } }
    static int def_preset(x265_param*, const char*, const char*) { return 0; }
    static int parse_p(x265_param*, const char*, const char*) { return 0; }
    static x265_picture* alloc_pic() { return new x265_picture{1, 10, {nullptr, nullptr, nullptr}, {0, 0, 0}, 0, 0}; }
    static void free_pic(x265_picture* p) { delete p; }
    static void init_pic(x265_param*, x265_picture*) {}
    static x265_encoder* enc_open(x265_param*) { return new x265_encoder{10}; }
    static int enc_headers(x265_encoder*, x265_nal**, uint32_t* n) { if (n) *n = 0; return 0; }
    static int enc_encode(x265_encoder*, x265_nal**, uint32_t* n, x265_picture*, x265_picture*) { if (n) *n = 0; return 0; }
    static void enc_close(x265_encoder* e) { delete e; }
    static char* p2s(x265_param*, int) { return nullptr; }

    static const x265_api api_10bit = {
        212, 10, alloc_p, free_p, def_p, def_preset, parse_p,
        alloc_pic, free_pic, init_pic, enc_open, enc_headers, enc_encode, enc_close, p2s
    };
    const x265_api* x265_api_get(int) { return &api_10bit; }
}

namespace x265_12bit {
    static x265_param* alloc_p() { return new x265_param{12, 0, 0, 25, 1, 1}; }
    static void free_p(x265_param* p) { delete p; }
    static void def_p(x265_param* p) { if (p) { p->bitDepth = 12; p->fpsNum = 25; p->fpsDenom = 1; } }
    static int def_preset(x265_param*, const char*, const char*) { return 0; }
    static int parse_p(x265_param*, const char*, const char*) { return 0; }
    static x265_picture* alloc_pic() { return new x265_picture{1, 12, {nullptr, nullptr, nullptr}, {0, 0, 0}, 0, 0}; }
    static void free_pic(x265_picture* p) { delete p; }
    static void init_pic(x265_param*, x265_picture*) {}
    static x265_encoder* enc_open(x265_param*) { return new x265_encoder{12}; }
    static int enc_headers(x265_encoder*, x265_nal**, uint32_t* n) { if (n) *n = 0; return 0; }
    static int enc_encode(x265_encoder*, x265_nal**, uint32_t* n, x265_picture*, x265_picture*) { if (n) *n = 0; return 0; }
    static void enc_close(x265_encoder* e) { delete e; }
    static char* p2s(x265_param*, int) { return nullptr; }

    static const x265_api api_12bit = {
        212, 12, alloc_p, free_p, def_p, def_preset, parse_p,
        alloc_pic, free_pic, init_pic, enc_open, enc_headers, enc_encode, enc_close, p2s
    };
    const x265_api* x265_api_get(int) { return &api_12bit; }
}

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
