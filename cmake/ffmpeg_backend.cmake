# cmake/ffmpeg_backend.cmake
# Automatically builds static FFmpeg libraries into SK265_FFMPEG_CACHE_DIR if missing

set(SK265_FFMPEG_AVCODEC_LIB "${SK265_FFMPEG_CACHE_DIR}/lib/libavcodec.a")

if(NOT EXISTS "${SK265_FFMPEG_AVCODEC_LIB}")
    message(STATUS "FFmpeg static libraries not found in ${SK265_FFMPEG_CACHE_DIR}. Building static FFmpeg...")

    if(WIN32)
        set(FFMPEG_BUILD_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/build_ffmpeg_win.sh")
    else()
        set(FFMPEG_BUILD_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/build_ffmpeg_linux.sh")
    endif()

    execute_process(
        COMMAND bash "${FFMPEG_BUILD_SCRIPT}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )

    message(STATUS "FFmpeg static libraries build completed successfully.")
endif()
