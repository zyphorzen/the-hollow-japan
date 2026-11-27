#include "core/time/FixedTimestep.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <stdexcept>

using thj::core::FixedTimestep;
using thj::core::FrameTiming;
using thj::core::Nanoseconds;
using thj::core::u64;

using namespace std::chrono_literals;

namespace {

FrameTiming advanceFrame(FixedTimestep &timestep, Nanoseconds elapsed) {
    timestep.beginFrame(elapsed);

    while (timestep.hasPendingTick()) {
        timestep.consumeTick();
    }

    return timestep.endFrame();
}

} // namespace

TEST_CASE("Fixed timestep starts at tick zero", "[unit][time][timestep]") {
    FixedTimestep timestep;

    CHECK(timestep.currentTick().value == 0);
    CHECK_FALSE(timestep.isPaused());
    CHECK_FALSE(timestep.hasPendingTick());
    CHECK(timestep.interpolationAlpha() == 0.0);
}

TEST_CASE("Zero elapsed time executes no ticks", "[unit][time][timestep]") {
    FixedTimestep timestep;

    const auto result = advanceFrame(timestep, 0ns);

    CHECK(result.executedTicks == 0);
    CHECK(result.droppedTicks == 0);
    CHECK(result.interpolationAlpha == 0.0);
    CHECK(timestep.currentTick().value == 0);
}

TEST_CASE("Sub-tick elapsed time accumulates correctly", "[unit][time][timestep]") {
    FixedTimestep timestep;

    const auto first = advanceFrame(timestep, 8ms);

    CHECK(first.executedTicks == 0);

    CHECK(first.interpolationAlpha == Catch::Approx(0.48));

    const auto second = advanceFrame(timestep, 9ms);

    CHECK(second.executedTicks == 1);

    CHECK(second.interpolationAlpha == Catch::Approx(0.02));

    CHECK(timestep.currentTick().value == 1);
}

TEST_CASE("Rational accumulator handles exact tick boundary", "[unit][time][timestep]") {
    FixedTimestep timestep;

    const auto first = advanceFrame(timestep, 16'666'666ns);

    CHECK(first.executedTicks == 0);
    CHECK(first.interpolationAlpha < 1.0);

    const auto second = advanceFrame(timestep, 1ns);

    CHECK(second.executedTicks == 1);

    CHECK(second.interpolationAlpha == Catch::Approx(0.00000002));
}

TEST_CASE("Fixed timestep executes exactly sixty ticks per second", "[unit][time][timestep]") {
    FixedTimestep timestep;

    u64 executed = 0;

    for (int frame = 0; frame < 100; ++frame) {
        const auto result = advanceFrame(timestep, 10ms);

        executed += result.executedTicks;

        CHECK(result.droppedTicks == 0);
    }

    CHECK(executed == 60);
    CHECK(timestep.currentTick().value == 60);
    CHECK(timestep.interpolationAlpha() == 0.0);
}

TEST_CASE("Different render frequencies preserve simulation rate", "[unit][time][timestep]") {
    constexpr int frameRates[] = {30, 60, 120, 144, 240};

    for (const int fps : frameRates) {
        CAPTURE(fps);

        FixedTimestep timestep;

        constexpr auto totalNanoseconds = 1'000'000'000LL;

        for (int frame = 0; frame < fps; ++frame) {
            const auto start = totalNanoseconds * frame / fps;

            const auto end = totalNanoseconds * (frame + 1) / fps;

            const auto elapsed = Nanoseconds{end - start};

            const auto result = advanceFrame(timestep, elapsed);

            CHECK(result.droppedTicks == 0);
            CHECK(result.interpolationAlpha >= 0.0);
            CHECK(result.interpolationAlpha < 1.0);
        }

        CHECK(timestep.currentTick().value == 60);

        CHECK(timestep.interpolationAlpha() == 0.0);
    }
}

TEST_CASE("Negative elapsed time is clamped to zero", "[unit][time][timestep]") {
    FixedTimestep timestep;

    const auto result = advanceFrame(timestep, -100ms);

    CHECK(result.rawDelta == -100ms);
    CHECK(result.clampedDelta == 0ns);
    CHECK(result.deltaClamped);
    CHECK(result.executedTicks == 0);
    CHECK(timestep.currentTick().value == 0);
}

TEST_CASE("Large frame delta is clamped", "[unit][time][timestep]") {
    FixedTimestep timestep;

    const auto result = advanceFrame(timestep, 500ms);

    CHECK(result.rawDelta == 500ms);
    CHECK(result.clampedDelta == 250ms);
    CHECK(result.deltaClamped);

    CHECK(result.executedTicks == 5);
    CHECK(result.droppedTicks == 10);

    CHECK(timestep.currentTick().value == 5);
    CHECK(result.interpolationAlpha == 0.0);
}

TEST_CASE("Catch-up limit preserves fractional remainder", "[unit][time][timestep]") {
    FixedTimestep timestep;

    const auto result = advanceFrame(timestep, 110ms);

    CHECK_FALSE(result.deltaClamped);

    CHECK(result.executedTicks == 5);
    CHECK(result.droppedTicks == 1);

    CHECK(result.interpolationAlpha == Catch::Approx(0.6));

    CHECK(timestep.currentTick().value == 5);

    const auto next = advanceFrame(timestep, 0ns);

    CHECK(next.executedTicks == 0);
    CHECK(next.droppedTicks == 0);

    CHECK(next.interpolationAlpha == Catch::Approx(0.6));
}

TEST_CASE("Paused simulation executes no ticks", "[unit][time][timestep]") {
    FixedTimestep timestep;

    advanceFrame(timestep, 20ms);

    const auto beforePause = timestep.currentTick();

    timestep.setPaused(true);

    REQUIRE(timestep.isPaused());

    const auto result = advanceFrame(timestep, 500ms);

    CHECK(result.paused);
    CHECK(result.executedTicks == 0);
    CHECK(result.droppedTicks == 0);
    CHECK(result.interpolationAlpha == 0.0);

    CHECK(timestep.currentTick() == beforePause);
}

TEST_CASE("Resume does not accumulate paused time", "[unit][time][timestep]") {
    FixedTimestep timestep;

    advanceFrame(timestep, 10ms);

    CHECK(timestep.interpolationAlpha() == Catch::Approx(0.6));

    timestep.setPaused(true);

    advanceFrame(timestep, 250ms);
    advanceFrame(timestep, 250ms);

    timestep.setPaused(false);

    CHECK_FALSE(timestep.isPaused());
    CHECK(timestep.interpolationAlpha() == 0.0);

    const auto first = advanceFrame(timestep, 10ms);

    CHECK(first.executedTicks == 0);

    const auto second = advanceFrame(timestep, 7ms);

    CHECK(second.executedTicks == 1);
    CHECK(second.droppedTicks == 0);

    CHECK(timestep.currentTick().value == 1);
}

TEST_CASE("Simulation tick advances only after consumption", "[unit][time][timestep]") {
    FixedTimestep timestep;

    timestep.beginFrame(20ms);

    REQUIRE(timestep.hasPendingTick());

    CHECK(timestep.currentTick().value == 0);
    CHECK(timestep.nextTick().value == 1);
    CHECK(timestep.currentTick().value == 0);

    timestep.consumeTick();

    CHECK(timestep.currentTick().value == 1);
    CHECK_FALSE(timestep.hasPendingTick());

    const auto result = timestep.endFrame();

    CHECK(result.executedTicks == 1);
}

TEST_CASE("Fixed timestep rejects invalid frame lifecycle", "[unit][time][timestep]") {
    FixedTimestep timestep;

    REQUIRE_THROWS_AS(timestep.consumeTick(), std::logic_error);

    REQUIRE_THROWS_AS(timestep.endFrame(), std::logic_error);

    timestep.beginFrame(20ms);

    REQUIRE_THROWS_AS(timestep.beginFrame(20ms), std::logic_error);

    REQUIRE_THROWS_AS(timestep.endFrame(), std::logic_error);

    timestep.consumeTick();

    REQUIRE_NOTHROW(timestep.endFrame());

    CHECK_FALSE(timestep.hasPendingTick());
}

TEST_CASE("Rational accumulator avoids long-term timing drift", "[unit][time][timestep]") {
    FixedTimestep timestep;

    constexpr int seconds = 600;
    constexpr int framesPerSecond = 100;

    for (int frame = 0; frame < seconds * framesPerSecond; ++frame) {
        advanceFrame(timestep, 10ms);
    }

    CHECK(timestep.currentTick().value == 36'000);

    CHECK(timestep.interpolationAlpha() == 0.0);
}