add_executable(sk265_tests
    test_main.cpp
    test_core_router.cpp
    test_cli_parser.cpp
    test_y4m_input.cpp
    test_avisynth_input.cpp
    test_raw_output.cpp
    test_golden_cross_validation.cpp
    test_progress.cpp
    test_signal_and_pipe.cpp
    test_bounded_queue.cpp
    ../src/core/x265_router.cpp
    ../src/config/cli_parser.cpp
    ../src/pipeline/input/y4m_input.cpp
    ../src/pipeline/input/avisynth_input.cpp
    ../src/pipeline/output/raw_output.cpp
)

target_link_libraries(sk265_tests PRIVATE
    Catch2::Catch2WithMain
    x265_8bit
    x265_10bit
    x265_12bit
)
if(WIN32)
    target_link_libraries(sk265_tests PRIVATE ws2_32)
endif()

include(CTest)
include(Catch)
catch_discover_tests(sk265_tests)
