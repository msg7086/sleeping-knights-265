#pragma once
#include <cstdint>
#include <cstddef>

#define VS_MAKE_VERSION(major, minor) (((major) << 16) | (minor))
#define VAPOURSYNTH_API_MAJOR 4
#define VAPOURSYNTH_API_MINOR 0
#define VAPOURSYNTH_API_VERSION VS_MAKE_VERSION(VAPOURSYNTH_API_MAJOR, VAPOURSYNTH_API_MINOR)

#define VSSCRIPT_API_MAJOR 4
#define VSSCRIPT_API_MINOR 1
#define VSSCRIPT_API_VERSION VS_MAKE_VERSION(VSSCRIPT_API_MAJOR, VSSCRIPT_API_MINOR)

#if defined(_WIN32) && !defined(_WIN64)
#define VS_CC __stdcall
#else
#define VS_CC
#endif

typedef struct VSFrame VSFrame;
typedef struct VSNode VSNode;
typedef struct VSCore VSCore;
typedef struct VSMap VSMap;
typedef struct VSScript VSScript;
typedef struct VSPlugin VSPlugin;
typedef struct VSPluginFunction VSPluginFunction;
typedef struct VSFunction VSFunction;
typedef struct VSLogHandle VSLogHandle;
typedef struct VSFrameContext VSFrameContext;
typedef struct VSAudioInfo VSAudioInfo;
typedef struct VSAudioFormat VSAudioFormat;
typedef struct VSFilterDependency VSFilterDependency;

typedef void (VS_CC *VSFilterGetFrame)(int n, int activationReason, void *instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const struct VSAPI *vsapi);
typedef void (VS_CC *VSFilterFree)(void *instanceData, VSCore *core, const struct VSAPI *vsapi);
typedef void (VS_CC *VSFrameDoneCallback)(void* userData, const VSFrame* f, int n, VSNode* node, const char* errorMsg);
typedef void (VS_CC *VSPublicFunction)(const VSMap *in, VSMap *out, void *userData, VSCore *core, const struct VSAPI *vsapi);
typedef void (VS_CC *VSFreeFunctionData)(void *userData);
typedef void (VS_CC *VSLogHandler)(int msgType, const char *msg, void *userData);
typedef void (VS_CC *VSLogHandlerFree)(void *userData);

typedef enum VSColorFamily {
    cfUndefined = 0,
    cfGray      = 1,
    cfRGB       = 2,
    cfYUV       = 3
} VSColorFamily;

typedef enum VSSampleType {
    stInteger = 0,
    stFloat   = 1
} VSSampleType;

typedef struct VSVideoFormat {
    int colorFamily;
    int sampleType;
    int bitsPerSample;
    int bytesPerSample;
    int subSamplingW;
    int subSamplingH;
    int numPlanes;
} VSVideoFormat;

typedef struct VSVideoInfo {
    VSVideoFormat format;
    int64_t fpsNum;
    int64_t fpsDen;
    int width;
    int height;
    int numFrames;
    int flags;
} VSVideoInfo;

typedef struct VSCoreInfo {
    const char* versionString;
    int core;
    int api;
    int numThreads;
    int64_t maxFramebufferSize;
    int64_t usedFramebufferSize;
} VSCoreInfo;

typedef struct VSAPI VSAPI;
struct VSAPI {
    /* Audio and video filter related including nodes */
    void (VS_CC *createVideoFilter)(VSMap *out, const char *name, const VSVideoInfo *vi, VSFilterGetFrame getFrame, VSFilterFree free, int filterMode, const VSFilterDependency *dependencies, int numDeps, void *instanceData, VSCore *core);
    VSNode *(VS_CC *createVideoFilter2)(const char *name, const VSVideoInfo *vi, VSFilterGetFrame getFrame, VSFilterFree free, int filterMode, const VSFilterDependency *dependencies, int numDeps, void *instanceData, VSCore *core);
    void (VS_CC *createAudioFilter)(VSMap *out, const char *name, const VSAudioInfo *ai, VSFilterGetFrame getFrame, VSFilterFree free, int filterMode, const VSFilterDependency *dependencies, int numDeps, void *instanceData, VSCore *core);
    VSNode *(VS_CC *createAudioFilter2)(const char *name, const VSAudioInfo *ai, VSFilterGetFrame getFrame, VSFilterFree free, int filterMode, const VSFilterDependency *dependencies, int numDeps, void *instanceData, VSCore *core);
    int (VS_CC *setLinearFilter)(VSNode *node);
    void (VS_CC *setCacheMode)(VSNode *node, int mode);
    void (VS_CC *setCacheOptions)(VSNode *node, int fixedSize, int maxSize, int maxHistorySize);

    void (VS_CC *freeNode)(VSNode *node);
    VSNode *(VS_CC *addNodeRef)(VSNode *node);
    int (VS_CC *getNodeType)(VSNode *node);
    const VSVideoInfo *(VS_CC *getVideoInfo)(VSNode *node);
    const VSAudioInfo *(VS_CC *getAudioInfo)(VSNode *node);

    /* Frame related functions */
    VSFrame *(VS_CC *newVideoFrame)(const VSVideoFormat *format, int width, int height, const VSFrame *propSrc, VSCore *core);
    VSFrame *(VS_CC *newVideoFrame2)(const VSVideoFormat *format, int width, int height, const VSFrame **planeSrc, const int *planes, const VSFrame *propSrc, VSCore *core);
    VSFrame *(VS_CC *newAudioFrame)(const VSAudioFormat *format, int numSamples, const VSFrame *propSrc, VSCore *core);
    VSFrame *(VS_CC *newAudioFrame2)(const VSAudioFormat *format, int numSamples, const VSFrame **channelSrc, const int *channels, const VSFrame *propSrc, VSCore *core);
    void (VS_CC *freeFrame)(const VSFrame *f);
    const VSFrame *(VS_CC *addFrameRef)(const VSFrame *f);
    VSFrame *(VS_CC *copyFrame)(const VSFrame *f, VSCore *core);
    const VSMap *(VS_CC *getFramePropertiesRO)(const VSFrame *f);
    VSMap *(VS_CC *getFramePropertiesRW)(VSFrame *f);

    ptrdiff_t (VS_CC *getStride)(const VSFrame *f, int plane);
    const uint8_t *(VS_CC *getReadPtr)(const VSFrame *f, int plane);
    uint8_t *(VS_CC *getWritePtr)(VSFrame *f, int plane);

    const VSVideoFormat *(VS_CC *getVideoFrameFormat)(const VSFrame *f);
    const VSAudioFormat *(VS_CC *getAudioFrameFormat)(const VSFrame *f);
    int (VS_CC *getFrameType)(const VSFrame *f);
    int (VS_CC *getFrameWidth)(const VSFrame *f, int plane);
    int (VS_CC *getFrameHeight)(const VSFrame *f, int plane);
    int (VS_CC *getFrameLength)(const VSFrame *f);

    /* General format functions */
    int (VS_CC *getVideoFormatName)(const VSVideoFormat *format, char *buffer);
    int (VS_CC *getAudioFormatName)(const VSAudioFormat *format, char *buffer);
    int (VS_CC *queryVideoFormat)(VSVideoFormat *format, int colorFamily, int sampleType, int bitsPerSample, int subSamplingW, int subSamplingH, VSCore *core);
    int (VS_CC *queryAudioFormat)(VSAudioFormat *format, int sampleType, int bitsPerSample, uint64_t channelLayout, VSCore *core);
    uint32_t (VS_CC *queryVideoFormatID)(int colorFamily, int sampleType, int bitsPerSample, int subSamplingW, int subSamplingH, VSCore *core);
    int (VS_CC *getVideoFormatByID)(VSVideoFormat *format, uint32_t id, VSCore *core);

    /* Frame request and filter getframe functions */
    const VSFrame *(VS_CC *getFrame)(int n, VSNode *node, char *errorMsg, int bufSize);
    void (VS_CC *getFrameAsync)(int n, VSNode *node, VSFrameDoneCallback callback, void *userData);
    const VSFrame *(VS_CC *getFrameFilter)(int n, VSNode *node, VSFrameContext *frameCtx);
    void (VS_CC *requestFrameFilter)(int n, VSNode *node, VSFrameContext *frameCtx);
    void (VS_CC *releaseFrameEarly)(VSNode *node, int n, VSFrameContext *frameCtx);
    void (VS_CC *cacheFrame)(const VSFrame *frame, int n, VSFrameContext *frameCtx);
    void (VS_CC *setFilterError)(const char *errorMessage, VSFrameContext *frameCtx);

    /* External functions */
    VSFunction *(VS_CC *createFunction)(VSPublicFunction func, void *userData, VSFreeFunctionData free, VSCore *core);
    void (VS_CC *freeFunction)(VSFunction *f);
    VSFunction *(VS_CC *addFunctionRef)(VSFunction *f);
    void (VS_CC *callFunction)(VSFunction *func, const VSMap *in, VSMap *out);

    /* Map and property access functions */
    VSMap *(VS_CC *createMap)(void);
    void (VS_CC *freeMap)(VSMap *map);
    void (VS_CC *clearMap)(VSMap *map);
    void (VS_CC *copyMap)(const VSMap *src, VSMap *dst);

    void (VS_CC *mapSetError)(VSMap *map, const char *errorMessage);
    const char *(VS_CC *mapGetError)(const VSMap *map);

    int (VS_CC *mapNumKeys)(const VSMap *map);
    const char *(VS_CC *mapGetKey)(const VSMap *map, int index);
    int (VS_CC *mapDeleteKey)(VSMap *map, const char *key);
    int (VS_CC *mapNumElements)(const VSMap *map, const char *key);
    int (VS_CC *mapGetType)(const VSMap *map, const char *key);
    int (VS_CC *mapSetEmpty)(VSMap *map, const char *key, int type);

    int64_t (VS_CC *mapGetInt)(const VSMap *map, const char *key, int index, int *error);
    int (VS_CC *mapGetIntSaturated)(const VSMap *map, const char *key, int index, int *error);
    const int64_t *(VS_CC *mapGetIntArray)(const VSMap *map, const char *key, int *error);
    int (VS_CC *mapSetInt)(VSMap *map, const char *key, int64_t i, int append);
    int (VS_CC *mapSetIntArray)(VSMap *map, const char *key, const int64_t *i, int size);

    double (VS_CC *mapGetFloat)(const VSMap *map, const char *key, int index, int *error);
    float (VS_CC *mapGetFloatSaturated)(const VSMap *map, const char *key, int index, int *error);
    const double *(VS_CC *mapGetFloatArray)(const VSMap *map, const char *key, int *error);
    int (VS_CC *mapSetFloat)(VSMap *map, const char *key, double d, int append);
    int (VS_CC *mapSetFloatArray)(VSMap *map, const char *key, const double *d, int size);

    const char *(VS_CC *mapGetData)(const VSMap *map, const char *key, int index, int *error);
    int (VS_CC *mapGetDataSize)(const VSMap *map, const char *key, int index, int *error);
    int (VS_CC *mapGetDataTypeHint)(const VSMap *map, const char *key, int index, int *error);
    int (VS_CC *mapSetData)(VSMap *map, const char *key, const char *data, int size, int type, int append);

    VSNode *(VS_CC *mapGetNode)(const VSMap *map, const char *key, int index, int *error);
    int (VS_CC *mapSetNode)(VSMap *map, const char *key, VSNode *node, int append);
    int (VS_CC *mapConsumeNode)(VSMap *map, const char *key, VSNode *node, int append);

    const VSFrame *(VS_CC *mapGetFrame)(const VSMap *map, const char *key, int index, int *error);
    int (VS_CC *mapSetFrame)(VSMap *map, const char *key, const VSFrame *f, int append);
    int (VS_CC *mapConsumeFrame)(VSMap *map, const char *key, const VSFrame *f, int append);

    VSFunction *(VS_CC *mapGetFunction)(const VSMap *map, const char *key, int index, int *error);
    int (VS_CC *mapSetFunction)(VSMap *map, const char *key, VSFunction *func, int append);
    int (VS_CC *mapConsumeFunction)(VSMap *map, const char *key, VSFunction *func, int append);

    /* Plugin and plugin function related */
    int (VS_CC *registerFunction)(const char *name, const char *args, const char *returnType, VSPublicFunction argsFunc, void *functionData, VSPlugin *plugin);
    VSPlugin *(VS_CC *getPluginByID)(const char *identifier, VSCore *core);
    VSPlugin *(VS_CC *getPluginByNamespace)(const char *ns, VSCore *core);
    VSPlugin *(VS_CC *getNextPlugin)(VSPlugin *plugin, VSCore *core);
    const char *(VS_CC *getPluginName)(VSPlugin *plugin);
    const char *(VS_CC *getPluginID)(VSPlugin *plugin);
    const char *(VS_CC *getPluginNamespace)(VSPlugin *plugin);
    VSPluginFunction *(VS_CC *getNextPluginFunction)(VSPluginFunction *func, VSPlugin *plugin);
    VSPluginFunction *(VS_CC *getPluginFunctionByName)(const char *name, VSPlugin *plugin);
    const char *(VS_CC *getPluginFunctionName)(VSPluginFunction *func);
    const char *(VS_CC *getPluginFunctionArguments)(VSPluginFunction *func);
    const char *(VS_CC *getPluginFunctionReturnType)(VSPluginFunction *func);
    const char *(VS_CC *getPluginPath)(const VSPlugin *plugin);
    int (VS_CC *getPluginVersion)(const VSPlugin *plugin);
    VSMap *(VS_CC *invoke)(VSPlugin *plugin, const char *name, const VSMap *args);

    /* Core and information */
    VSCore *(VS_CC *createCore)(int flags);
    void (VS_CC *freeCore)(VSCore *core);
    int64_t (VS_CC *setMaxCacheSize)(int64_t bytes, VSCore *core);
    int (VS_CC *setThreadCount)(int threads, VSCore *core);
    void (VS_CC *getCoreInfo)(VSCore *core, VSCoreInfo *info);
    int (VS_CC *getAPIVersion)(void);

    /* Message handler */
    void (VS_CC *logMessage)(int msgType, const char *msg, VSCore *core);
    VSLogHandle *(VS_CC *addLogHandler)(VSLogHandler handler, VSLogHandlerFree free, void *userData, VSCore *core);
    int (VS_CC *removeLogHandler)(VSLogHandle *handle, VSCore *core);
};

typedef struct VSSCRIPTAPI VSSCRIPTAPI;
struct VSSCRIPTAPI {
    int (VS_CC *getAPIVersion)(void);
    const VSAPI* (VS_CC *getVSAPI)(int version);
    VSScript* (VS_CC *createScript)(VSCore* core);
    VSCore* (VS_CC *getCore)(VSScript* handle);
    int (VS_CC *evaluateBuffer)(VSScript* handle, const char* buffer, const char* scriptFilename);
    int (VS_CC *evaluateFile)(VSScript* handle, const char* scriptFilename);
    const char* (VS_CC *getError)(VSScript* handle);
    int (VS_CC *getExitCode)(VSScript* handle);
    int (VS_CC *getVariable)(VSScript* handle, const char* name, VSMap* dst);
    int (VS_CC *setVariables)(VSScript* handle, const VSMap* vars);
    VSNode* (VS_CC *getOutputNode)(VSScript* handle, int index);
    VSNode* (VS_CC *getOutputAlphaNode)(VSScript* handle, int index);
    int (VS_CC *getAltOutputMode)(VSScript* handle, int index);
    void (VS_CC *freeScript)(VSScript* handle);
    void (VS_CC *evalSetWorkingDir)(VSScript* handle, int setCWD);
};
