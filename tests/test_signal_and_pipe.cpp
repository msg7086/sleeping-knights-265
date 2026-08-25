#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include "utils/signal_handler.h"
#include "pipeline/input/y4m_input.h"

TEST_CASE("SignalHandler manages interrupt state cleanly", "[utils][signal]") {
    sk265::utils::installSignalHandler();
    REQUIRE_FALSE(sk265::utils::isInterrupted());

    sk265::utils::requestStop();
    REQUIRE(sk265::utils::isInterrupted());

    sk265::utils::installSignalHandler();
    REQUIRE_FALSE(sk265::utils::isInterrupted());
}

TEST_CASE("Y4mInput reads from piped stream via openFromStream", "[pipeline][pipe]") {
    std::string header = "YUV4MPEG2 W4 H4 F30:1 Ip A1:1 C420\nFRAME\n";
    std::string payload(24, static_cast<char>(128));
    std::istringstream pipeStream(header + payload);

    sk265::pipeline::input::Y4mInput input;
    REQUIRE(input.openFromStream(pipeStream));

    auto frame = input.readFrame();
    REQUIRE(frame.has_value());
    REQUIRE(frame->width == 4);
    REQUIRE(frame->height == 4);
    REQUIRE(frame->planes[0].size() == 16);

    REQUIRE_FALSE(input.readFrame().has_value());
    REQUIRE(input.isEof());
}
