#include <catch2/catch_test_macros.hpp>
#include "pipeline/input/avisynth_input.h"

TEST_CASE("AviSynthInput handles non-existent file gracefully without crashing", "[input]") {
    sk265::pipeline::input::AviSynthInput avsInput;
    REQUIRE_FALSE(avsInput.open("non_existent_script_12345.avs"));
    REQUIRE(avsInput.isEof());
    REQUIRE_FALSE(avsInput.readFrame().has_value());
}

TEST_CASE("AviSynthInput handles empty path", "[input]") {
    sk265::pipeline::input::AviSynthInput avsInput;
    REQUIRE_FALSE(avsInput.open(""));
    REQUIRE(avsInput.isEof());
}
