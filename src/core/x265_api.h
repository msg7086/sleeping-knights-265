#pragma once
#include <cstdint>

#ifndef X265_H
#define X265_H

struct x265_param {
    int bitDepth;
    int sourceWidth;
    int sourceHeight;
    int fpsNum;
    int fpsDenom;
    int internalCsp;
    int frameNumThreads;
    int logLvl;
};

struct x265_picture {
    int colorSpace;
    int bitDepth;
    void* planes[3];
    int stride[3];
    int64_t pts;
    int64_t dts;
    int sliceType;
    int poc;
};

struct x265_nal {
    uint32_t type;
    uint32_t sizeBytes;
    uint8_t* payload;
};

struct x265_encoder;

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

#endif // X265_H
