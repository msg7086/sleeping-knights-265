cmake_minimum_required(VERSION 3.20)
project(sk265 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Persistent backend build directory (protected from clean/rm -rf build/)
set(SK265_BACKEND_CACHE_DIR "${CMAKE_SOURCE_DIR}/.build_x265" CACHE PATH "Directory for cached x265 multi-depth libraries")

include(FetchContent)

# Catch2 v3.5.2 for testing
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

include(cmake/version.cmake)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/generated
    ${SK265_BACKEND_CACHE_DIR}/8bit
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/x265/source
)

# Imported multi-depth x265 static libraries from persistent cache
add_library(x265_8bit STATIC IMPORTED)
set_target_properties(x265_8bit PROPERTIES IMPORTED_LOCATION "${SK265_BACKEND_CACHE_DIR}/8bit/libx265.a")

add_library(x265_10bit STATIC IMPORTED)
set_target_properties(x265_10bit PROPERTIES IMPORTED_LOCATION "${SK265_BACKEND_CACHE_DIR}/10bit/libx265.a")

add_library(x265_12bit STATIC IMPORTED)
set_target_properties(x265_12bit PROPERTIES IMPORTED_LOCATION "${SK265_BACKEND_CACHE_DIR}/12bit/libx265.a")

# Configure L-SMASH static library
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated/lsmash_config")
file(WRITE "${CMAKE_BINARY_DIR}/generated/lsmash_config/config.h"
"#define LSMASH_REV \"0\"\n#define LSMASH_GIT_HASH \"unknown\"\n"
)

set(LSMASH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/l-smash")
set(LSMASH_SOURCES
    ${LSMASH_DIR}/common/alloc.c
    ${LSMASH_DIR}/common/bits.c
    ${LSMASH_DIR}/common/bytes.c
    ${LSMASH_DIR}/common/list.c
    ${LSMASH_DIR}/common/multibuf.c
    ${LSMASH_DIR}/common/osdep.c
    ${LSMASH_DIR}/common/utils.c
    ${LSMASH_DIR}/codecs/a52.c
    ${LSMASH_DIR}/codecs/alac.c
    ${LSMASH_DIR}/codecs/av1.c
    ${LSMASH_DIR}/codecs/description.c
    ${LSMASH_DIR}/codecs/dts.c
    ${LSMASH_DIR}/codecs/h264.c
    ${LSMASH_DIR}/codecs/hevc.c
    ${LSMASH_DIR}/codecs/id.c
    ${LSMASH_DIR}/codecs/mp4a.c
    ${LSMASH_DIR}/codecs/mp4sys.c
    ${LSMASH_DIR}/codecs/mp4v.c
    ${LSMASH_DIR}/codecs/nalu.c
    ${LSMASH_DIR}/codecs/opus.c
    ${LSMASH_DIR}/codecs/qt_wfex.c
    ${LSMASH_DIR}/codecs/vc1.c
    ${LSMASH_DIR}/codecs/wma.c
    ${LSMASH_DIR}/importer/a52_imp.c
    ${LSMASH_DIR}/importer/adts_imp.c
    ${LSMASH_DIR}/importer/als_imp.c
    ${LSMASH_DIR}/importer/amr_imp.c
    ${LSMASH_DIR}/importer/dts_imp.c
    ${LSMASH_DIR}/importer/importer.c
    ${LSMASH_DIR}/importer/isobm_imp.c
    ${LSMASH_DIR}/importer/mp3_imp.c
    ${LSMASH_DIR}/importer/nalu_imp.c
    ${LSMASH_DIR}/importer/vc1_imp.c
    ${LSMASH_DIR}/importer/wave_imp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pipeline/output/lsmash_ivf_stub.c
    ${LSMASH_DIR}/core/box.c
    ${LSMASH_DIR}/core/box_default.c
    ${LSMASH_DIR}/core/box_type.c
    ${LSMASH_DIR}/core/chapter.c
    ${LSMASH_DIR}/core/file.c
    ${LSMASH_DIR}/core/fragment.c
    ${LSMASH_DIR}/core/isom.c
    ${LSMASH_DIR}/core/meta.c
    ${LSMASH_DIR}/core/print.c
    ${LSMASH_DIR}/core/read.c
    ${LSMASH_DIR}/core/summary.c
    ${LSMASH_DIR}/core/timeline.c
    ${LSMASH_DIR}/core/write.c
)

add_library(lsmash STATIC ${LSMASH_SOURCES})
target_include_directories(lsmash PUBLIC
    ${LSMASH_DIR}
    ${CMAKE_BINARY_DIR}/generated/lsmash_config
)
if(NOT MSVC)
    target_compile_options(lsmash PRIVATE -w)
endif()

add_executable(sk265
    src/main.cpp
    src/core/x265_router.cpp
    src/config/cli_parser.cpp
    src/pipeline/input/y4m_input.cpp
    src/pipeline/input/avisynth_input.cpp
    src/pipeline/input/vapoursynth_input.cpp
    src/pipeline/output/raw_output.cpp
    src/pipeline/output/mp4_output.cpp
)

target_link_libraries(sk265 PRIVATE
    x265_8bit
    x265_10bit
    x265_12bit
    lsmash
)
if(WIN32)
    target_link_libraries(sk265 PRIVATE ws2_32)
endif()

enable_testing()
add_subdirectory(tests)
