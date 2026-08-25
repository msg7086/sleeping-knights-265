# cmake/x265_backend.cmake
# Automatically builds x265 8-bit, 10-bit, and 12-bit static libraries into SK265_BACKEND_CACHE_DIR in parallel

set(SK265_X265_8BIT_LIB "${SK265_BACKEND_CACHE_DIR}/8bit/libx265.a")
set(SK265_X265_10BIT_LIB "${SK265_BACKEND_CACHE_DIR}/10bit/libx265.a")
set(SK265_X265_12BIT_LIB "${SK265_BACKEND_CACHE_DIR}/12bit/libx265.a")

if(NOT EXISTS "${SK265_X265_8BIT_LIB}" OR NOT EXISTS "${SK265_X265_10BIT_LIB}" OR NOT EXISTS "${SK265_X265_12BIT_LIB}")
    message(STATUS "x265 backend static libraries not found in ${SK265_BACKEND_CACHE_DIR}. Building multi-depth x265 static libraries in parallel...")

    if(WIN32)
        set(X265_BUILD_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/build_x265_win.sh")
    else()
        set(X265_BUILD_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/build_x265_linux.sh")
    endif()

    execute_process(
        COMMAND bash "${X265_BUILD_SCRIPT}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )

    message(STATUS "x265 backend multi-depth static libraries build completed successfully.")
endif()
