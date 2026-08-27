add_executable(sk265
    main.cpp
    core/x265_router.cpp
    core/tuning_preset.cpp
    config/cli_parser.cpp
    config/config_file_parser.cpp
    config/cascading_config.cpp
    pipeline/input/y4m_input.cpp
    pipeline/input/avisynth_input.cpp
    pipeline/input/vapoursynth_input.cpp
    pipeline/input/lavf_input.cpp
    pipeline/output/raw_output.cpp
    pipeline/output/mp4_output.cpp
    pipeline/output/lavf_output.cpp
    pipeline/output/gop_output.cpp
    pipeline/output/output_factory.cpp
)

target_link_libraries(sk265 PRIVATE
    x265_10bit
    x265_12bit
    x265_8bit
    lsmash
    avformat
    avcodec
    swscale
    swresample
    avutil
)

if(WIN32)
    target_link_libraries(sk265 PRIVATE ws2_32 bcrypt secur32 strmiids mfplat mfuuid ole32 winpthread)
else()
    find_library(NUMA_LIBRARY numa)
    if(NUMA_LIBRARY)
        target_link_libraries(sk265 PRIVATE ${NUMA_LIBRARY})
    endif()
    target_link_libraries(sk265 PRIVATE ${CMAKE_DL_LIBS} pthread m)
endif()

include(GNUInstallDirs)
install(TARGETS sk265 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
