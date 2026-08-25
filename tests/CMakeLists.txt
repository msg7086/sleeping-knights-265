add_executable(sk265_tests
    test_main.cpp
    test_core_router.cpp
    test_cli_parser.cpp
    test_y4m_input.cpp
    ../src/core/x265_router.cpp
    ../src/config/cli_parser.cpp
    ../src/pipeline/input/y4m_input.cpp
)

target_link_libraries(sk265_tests PRIVATE
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(sk265_tests)
