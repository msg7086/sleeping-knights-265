add_executable(sk265_tests
    test_main.cpp
    test_core_router.cpp
    ../src/core/x265_router.cpp
)

target_link_libraries(sk265_tests PRIVATE
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(sk265_tests)
