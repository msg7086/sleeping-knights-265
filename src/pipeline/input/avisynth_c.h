#pragma once
#include <cstdint>

#ifdef _WIN32
#define AVS_CC __stdcall
#else
#define AVS_CC
#endif

#define AVS_INTERFACE_26 6

struct AVS_Clip;
struct AVS_ScriptEnvironment;
struct AVS_VideoFrame;

struct AVS_VideoInfo {
    int width, height;
    unsigned fps_numerator, fps_denominator;
    int num_frames;
    int pixel_type;
    int audio_samples_per_second;
    int sample_type;
    int64_t num_audio_samples;
    int nchannels;
    int image_type;
};

struct AVS_Value {
    short type;
    short array_size;
    union {
        int i;
        int64_t i64;
        int b;
        float f;
        double f64;
        const char* s;
        const AVS_Value* a;
        AVS_Clip* c;
    } d;
};

inline bool avs_is_error(const AVS_Value& v) { return v.type == 'e'; }
inline bool avs_is_clip(const AVS_Value& v) { return v.type == 'c'; }
inline bool avs_is_string(const AVS_Value& v) { return v.type == 's'; }
inline const char* avs_as_string(const AVS_Value& v) { return v.d.s; }
inline AVS_Value avs_new_value_string(const char* s) {
    AVS_Value v{};
    v.type = 's';
    v.d.s = s;
    return v;
}
inline AVS_Value avs_new_value_bool(bool b) {
    AVS_Value v{};
    v.type = 'b';
    v.d.b = b ? 1 : 0;
    return v;
}
inline AVS_Value avs_new_value_array(const AVS_Value* a, int size) {
    AVS_Value v{};
    v.type = 'a';
    v.d.a = a;
    v.array_size = static_cast<short>(size);
    return v;
}
