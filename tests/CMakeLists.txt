add_executable(sk265_tests
    test_main.cpp
    test_core_router.cpp
    test_tuning_preset.cpp
    test_cli_parser.cpp
    test_config_file_parser.cpp
    test_cascading_config.cpp
    test_y4m_input.cpp
    test_avisynth_input.cpp
    test_vapoursynth_input.cpp
    test_lavf_input.cpp
    test_raw_output.cpp
    test_mp4_output.cpp
    test_lavf_output.cpp
    test_gop_output.cpp
    test_async_output.cpp
    test_output_factory.cpp
    test_golden_cross_validation.cpp
    test_progress.cpp
    test_signal_and_pipe.cpp
    test_bounded_queue.cpp
    test_log_mirror.cpp
    ../src/core/x265_router.cpp
    ../src/core/tuning_preset.cpp
    ../src/config/cli_parser.cpp
    ../src/config/config_file_parser.cpp
    ../src/config/cascading_config.cpp
    ../src/pipeline/input/y4m_input.cpp
    ../src/pipeline/input/avisynth_input.cpp
    ../src/pipeline/input/vapoursynth_input.cpp
    ../src/pipeline/input/lavf_input.cpp
    ../src/pipeline/output/raw_output.cpp
    ../src/pipeline/output/mp4_output.cpp
    ../src/pipeline/output/lavf_output.cpp
    ../src/pipeline/output/gop_output.cpp
    ../src/pipeline/output/async_output.cpp
    ../src/pipeline/output/output_factory.cpp
    ../src/utils/log_mirror.cpp
)

target_link_libraries(sk265_tests PRIVATE
    Catch2::Catch2WithMain
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
    target_link_libraries(sk265_tests PRIVATE ws2_32 bcrypt secur32 strmiids mfplat mfuuid ole32 winpthread)
else()
    find_library(NUMA_LIBRARY numa)
    if(NUMA_LIBRARY)
        target_link_libraries(sk265_tests PRIVATE ${NUMA_LIBRARY})
    endif()
    target_link_libraries(sk265_tests PRIVATE ${CMAKE_DL_LIBS} pthread m)
endif()

include(CTest)
include(Catch)
catch_discover_tests(sk265_tests)
