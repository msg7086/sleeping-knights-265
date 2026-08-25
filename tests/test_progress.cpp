#include <catch2/catch_test_macros.hpp>
#include "utils/progress.h"

TEST_CASE("ConsoleProgress formats ETA correctly", "[utils][progress]") {
    REQUIRE(sk265::utils::ConsoleProgress::formatEta(0) == "0:00:00");
    REQUIRE(sk265::utils::ConsoleProgress::formatEta(45) == "0:00:45");
    REQUIRE(sk265::utils::ConsoleProgress::formatEta(125) == "0:02:05");
    REQUIRE(sk265::utils::ConsoleProgress::formatEta(3665) == "1:01:05");
}

TEST_CASE("ConsoleProgress stylish mode format and lifecycle", "[utils][progress]") {
    sk265::utils::ConsoleProgress stylishProgress(200, 25, 1, true, true);
    REQUIRE(stylishProgress.isStylish());

    // Should update in stylish mode without error
    stylishProgress.update(50, 1048576, true);
    stylishProgress.finish(50);
}

TEST_CASE("ConsoleProgress updates and finishes cleanly", "[utils][progress]") {
    sk265::utils::ConsoleProgress progress(100, 25, 1, true);
    // Should update without throwing
    progress.update(10, 50000, true);
    progress.update(50, 250000, true);
    progress.finish(50);

    // Disabled progress should be a no-op
    sk265::utils::ConsoleProgress disabledProgress(100, 25, 1, false);
    disabledProgress.update(10, 50000, true);
    disabledProgress.finish(10);
}
