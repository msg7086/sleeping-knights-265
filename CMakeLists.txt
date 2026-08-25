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

add_executable(sk265
    src/main.cpp
    src/core/x265_router.cpp
    src/config/cli_parser.cpp
    src/pipeline/input/y4m_input.cpp
    src/pipeline/input/avisynth_input.cpp
    src/pipeline/output/raw_output.cpp
)

target_link_libraries(sk265 PRIVATE
    x265_8bit
    x265_10bit
    x265_12bit
)
if(WIN32)
    target_link_libraries(sk265 PRIVATE ws2_32)
endif()

enable_testing()
add_subdirectory(tests)
