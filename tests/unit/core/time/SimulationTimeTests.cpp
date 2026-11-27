#include "core/time/SimulationTime.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits>

using namespace thj::core;

TEST_CASE("Simulation time starts at tick zero", "[unit][time]") {
    constexpr SimulationTick initial{};

    CHECK(initial.value == 0);
    CHECK(kSimulationHz == 60);
    CHECK(MonotonicClock::is_steady);

    const auto next = initial.next();

    REQUIRE(next.has_value());
    CHECK(next->value == 1);

    SimulationTick tick{};

    for (u32 i = 0; i < kSimulationHz; ++i) {
        const auto advanced = tick.next();

        REQUIRE(advanced.has_value());

        tick = *advanced;
    }

    CHECK(tick.value == 60);
}

TEST_CASE("Simulation tick rejects overflow", "[unit][time]") {
    constexpr SimulationTick maximum{std::numeric_limits<u64>::max()};

    CHECK_FALSE(maximum.next().has_value());

    constexpr SimulationTick beforeMaximum{std::numeric_limits<u64>::max() - 1};

    const auto next = beforeMaximum.next();

    REQUIRE(next.has_value());

    CHECK(*next == maximum);
}