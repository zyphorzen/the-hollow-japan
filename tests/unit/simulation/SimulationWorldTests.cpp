#include "simulation/SimulationWorld.hpp"

#include "core/time/FixedTimestep.hpp"
#include "simulation/Components.hpp"

#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <stdexcept>
#include <type_traits>

using thj::core::EntityHandle;
using thj::core::FixedPoint;
using thj::core::FixedTimestep;
using thj::core::i64;
using thj::core::Nanoseconds;
using thj::core::Position2;
using thj::core::SimulationTick;
using thj::core::u64;

using thj::simulation::LifetimeComponent;
using thj::simulation::SimulationWorld;
using thj::simulation::TransformComponent;
using thj::simulation::VelocityComponent;

using namespace std::chrono_literals;

namespace {

struct ThrowingComponent {
    explicit ThrowingComponent(bool fail) {
        if (fail) {
            throw std::runtime_error("Simulation component construction failed");
        }
    }
};

void advanceFrame(SimulationWorld &world, FixedTimestep &timestep, Nanoseconds elapsed) {
    timestep.beginFrame(elapsed);

    while (timestep.hasPendingTick()) {
        const auto tick = timestep.nextTick();

        const auto report = world.step(tick);

        CHECK(report.tick == tick);

        timestep.consumeTick();
    }

    (void)timestep.endFrame();
}

} // namespace

TEST_CASE("Simulation world starts empty at tick zero", "[integration][simulation][world]") {
    SimulationWorld world;

    CHECK(world.currentTick().value == 0);
    CHECK(world.registry().aliveCount() == 0);
    CHECK(world.registry().poolCount() == 0);

    CHECK(world.pendingCommands() == 0);

    CHECK_FALSE(world.failed());
    CHECK_FALSE(world.isStepping());

    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<SimulationWorld>);

    STATIC_REQUIRE_FALSE(std::is_move_constructible_v<SimulationWorld>);

    STATIC_REQUIRE(std::is_same_v<decltype(world.registry()), const thj::ecs::Registry &>);
}

TEST_CASE("Simulation world applies creation before movement", "[integration][simulation][world]") {
    SimulationWorld world;

    const auto pending = world.queueCreate();

    world.queueEmplace<TransformComponent>(
        pending, Position2{FixedPoint::fromPixels(100), FixedPoint::fromPixels(200)});

    world.queueEmplace<VelocityComponent>(pending, FixedPoint::fromPixels(60), FixedPoint{});

    CHECK(world.pendingCommands() == 3);
    CHECK(world.registry().aliveCount() == 0);

    const auto report = world.step({1});

    CHECK(report.tick.value == 1);
    CHECK(report.beginBarrier.applied == 3);
    CHECK(report.endBarrier.total() == 0);

    REQUIRE(report.beginBarrier.success());

    const auto entity = report.beginBarrier.resolve(pending);

    REQUIRE(entity.has_value());

    REQUIRE(world.registry().isAlive(*entity));

    const auto &transform = world.registry().get<TransformComponent>(*entity);

    CHECK(transform.previousPosition.x == FixedPoint::fromPixels(100));

    CHECK(transform.position.x == FixedPoint::fromPixels(101));

    CHECK(transform.position.y == FixedPoint::fromPixels(200));

    CHECK(world.currentTick().value == 1);
    CHECK(world.pendingCommands() == 0);
}

TEST_CASE("Simulation world destroys expired entities at end barrier",
          "[integration][simulation][world]") {
    SimulationWorld world;

    const auto pending = world.queueCreate();

    world.queueEmplace<TransformComponent>(pending);

    world.queueEmplace<LifetimeComponent>(pending, u64{3});

    const auto first = world.step({1});

    const auto entity = first.beginBarrier.resolve(pending);

    REQUIRE(entity.has_value());
    REQUIRE(world.registry().isAlive(*entity));

    CHECK(world.registry().get<LifetimeComponent>(*entity).remainingTicks == 2);

    CHECK(first.endBarrier.total() == 0);

    const auto second = world.step({2});

    CHECK(world.registry().isAlive(*entity));

    CHECK(second.endBarrier.total() == 0);

    const auto third = world.step({3});

    CHECK(third.endBarrier.applied == 1);

    CHECK_FALSE(world.registry().isAlive(*entity));

    CHECK(world.registry().componentCount<TransformComponent>() == 0);

    CHECK(world.registry().componentCount<LifetimeComponent>() == 0);

    CHECK(world.currentTick().value == 3);
    CHECK(world.pendingCommands() == 0);
}

TEST_CASE("Simulation world applies queued component and entity removal",
          "[integration][simulation][world]") {
    SimulationWorld world;

    const auto pending = world.queueCreate();

    world.queueEmplace<TransformComponent>(pending);

    world.queueEmplace<VelocityComponent>(pending, FixedPoint::fromPixels(60), FixedPoint{});

    const auto first = world.step({1});

    const auto entity = first.beginBarrier.resolve(pending);

    REQUIRE(entity.has_value());

    CHECK(world.registry().get<TransformComponent>(*entity).position.x ==
          FixedPoint::fromPixels(1));

    world.queueRemove<VelocityComponent>(*entity);

    CHECK(world.registry().has<VelocityComponent>(*entity));

    const auto second = world.step({2});

    CHECK(second.beginBarrier.applied == 1);

    CHECK_FALSE(world.registry().has<VelocityComponent>(*entity));

    CHECK(world.registry().get<TransformComponent>(*entity).position.x ==
          FixedPoint::fromPixels(1));

    world.queueDestroy(*entity);

    const auto third = world.step({3});

    CHECK(third.beginBarrier.applied == 1);

    CHECK_FALSE(world.registry().isAlive(*entity));
    CHECK(world.registry().aliveCount() == 0);

    world.queueDestroy(*entity);

    const auto fourth = world.step({4});

    CHECK(fourth.beginBarrier.skipped == 1);
    CHECK_FALSE(world.failed());
}

TEST_CASE("Simulation world rejects nonsequential ticks", "[integration][simulation][world]") {
    SimulationWorld world;

    const auto pending = world.queueCreate();

    REQUIRE_THROWS_AS(world.step({0}), std::invalid_argument);

    REQUIRE_THROWS_AS(world.step({2}), std::invalid_argument);

    CHECK(world.currentTick().value == 0);
    CHECK(world.pendingCommands() == 1);

    CHECK_FALSE(world.failed());

    const auto first = world.step({1});

    REQUIRE(first.beginBarrier.resolve(pending).has_value());

    REQUIRE_THROWS_AS(world.step({1}), std::invalid_argument);

    REQUIRE_THROWS_AS(world.step({3}), std::invalid_argument);

    CHECK_FALSE(world.failed());

    CHECK(world.currentTick().value == 1);

    const auto second = world.step({2});

    CHECK(second.tick.value == 2);
    CHECK(world.currentTick().value == 2);
}

TEST_CASE("Simulation world rejects stepping during active iteration",
          "[integration][simulation][world]") {
    SimulationWorld world;

    const auto pending = world.queueCreate();

    world.queueEmplace<TransformComponent>(pending);

    const auto first = world.step({1});

    REQUIRE(first.beginBarrier.resolve(pending).has_value());

    const auto count = world.registry().each<TransformComponent>(
        [&](EntityHandle entity, const TransformComponent &) {
            REQUIRE_THROWS_AS(world.step({2}), std::logic_error);

            world.queueDestroy(entity);

            CHECK(world.registry().isAlive(entity));
        });

    CHECK(count == 1);
    CHECK_FALSE(world.failed());

    CHECK(world.pendingCommands() == 1);
    CHECK(world.currentTick().value == 1);

    const auto second = world.step({2});

    CHECK(second.beginBarrier.applied == 1);

    CHECK(world.registry().aliveCount() == 0);
}

TEST_CASE("Simulation world enters fail stop state after system failure",
          "[integration][simulation][world]") {
    SimulationWorld world;

    const auto pending = world.queueCreate();

    world.queueEmplace<TransformComponent>(
        pending, Position2{FixedPoint::fromRaw(std::numeric_limits<i64>::max()), FixedPoint{}});

    world.queueEmplace<VelocityComponent>(pending, FixedPoint::fromPixels(60), FixedPoint{});

    REQUIRE_THROWS_AS(world.step({1}), std::overflow_error);

    CHECK(world.failed());
    CHECK_FALSE(world.isStepping());

    CHECK(world.currentTick().value == 0);

    CHECK(world.registry().aliveCount() == 1);

    REQUIRE_THROWS_AS(world.step({1}), std::logic_error);

    REQUIRE_THROWS_AS(world.queueCreate(), std::logic_error);
}

TEST_CASE("Simulation world enters fail stop state after barrier failure",
          "[integration][simulation][world]") {
    SimulationWorld world;

    const auto pending = world.queueCreate();

    world.queueEmplace<ThrowingComponent>(pending, true);

    world.queueEmplace<TransformComponent>(pending);

    REQUIRE_THROWS_AS(world.step({1}), std::runtime_error);

    CHECK(world.failed());
    CHECK_FALSE(world.isStepping());

    CHECK(world.currentTick().value == 0);

    CHECK(world.registry().aliveCount() == 1);

    CHECK(world.registry().componentCount<ThrowingComponent>() == 0);

    CHECK(world.registry().componentCount<TransformComponent>() == 0);

    CHECK(world.pendingCommands() == 0);
}

TEST_CASE("Simulation world integrates fixed timestep and pause",
          "[integration][simulation][world]") {
    constexpr int frameRates[] = {30, 60, 120, 144, 240};

    for (const int fps : frameRates) {
        CAPTURE(fps);

        SimulationWorld world;
        FixedTimestep timestep;

        const auto pending = world.queueCreate();

        world.queueEmplace<TransformComponent>(pending);

        world.queueEmplace<VelocityComponent>(pending, FixedPoint::fromPixels(96), FixedPoint{});

        world.queueEmplace<LifetimeComponent>(pending, u64{120});

        constexpr auto totalNanoseconds = 1'000'000'000LL;

        for (int frame = 0; frame < fps; ++frame) {
            const auto start = totalNanoseconds * frame / fps;

            const auto end = totalNanoseconds * (frame + 1) / fps;

            advanceFrame(world, timestep, Nanoseconds{end - start});
        }

        REQUIRE(world.currentTick().value == 60);

        CHECK(timestep.currentTick() == world.currentTick());

        EntityHandle entity{};

        const auto count = world.registry().each<TransformComponent>(
            [&](EntityHandle current, const TransformComponent &transform) {
                entity = current;

                CHECK(transform.position.x == FixedPoint::fromPixels(96));
            });

        REQUIRE(count == 1);
        REQUIRE(entity.valid());

        CHECK(world.registry().get<LifetimeComponent>(entity).remainingTicks == 60);

        timestep.setPaused(true);

        for (int frame = 0; frame < 100; ++frame) {
            advanceFrame(world, timestep, 10ms);
        }

        CHECK(world.currentTick().value == 60);

        CHECK(world.registry().get<TransformComponent>(entity).position.x ==
              FixedPoint::fromPixels(96));

        CHECK(world.registry().get<LifetimeComponent>(entity).remainingTicks == 60);

        timestep.setPaused(false);

        for (int frame = 0; frame < 100; ++frame) {
            advanceFrame(world, timestep, 10ms);
        }

        CHECK(world.currentTick().value == 120);

        CHECK(timestep.currentTick() == world.currentTick());

        CHECK_FALSE(world.registry().isAlive(entity));

        CHECK(world.registry().aliveCount() == 0);
        CHECK_FALSE(world.failed());
    }
}