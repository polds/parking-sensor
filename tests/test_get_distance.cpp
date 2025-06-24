#define CATCH_CONFIG_MAIN
#include "ParkingSensor.h"
#include <catch2/catch_all.hpp>

TEST_CASE("getDistanceCM returns correct value") {
    auto fakePulseIn = []() { return 10000L; }; // 10,000 us
    float distance = getDistanceCM(fakePulseIn);
    REQUIRE(distance == Catch::Approx(171.5).epsilon(0.01));
}

TEST_CASE("getDistanceCM returns -1 for timeout") {
    auto timeoutPulseIn = []() { return 0L; };
    REQUIRE(getDistanceCM(timeoutPulseIn) == -1.0f);
} 