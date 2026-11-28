
#include "simulation/SimulationWorld.hpp"

#include "core/time/FixedTimestep.hpp"
#include "simulation/Components.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <vector>

using thj::core::EntityHandle;
using thj::core::FixedPoint;
using thj::core::FixedTimestep;
using thj::core::i64;
using thj::core::Nanoseconds;
using thj::core::Position2;
using thj::core::u64;

using thj::simulation::LifetimeComponent;
using thj::simulation::SimulationWorld;
using thj::simulation::TransformComponent;
using thj::simulation::VelocityComponent;

using namespace std::chrono_literals;

namespace {

struct EntityState {
    EntityHandle entity{};

    i64 x{};
    i64 y{};

    i64 previousX{};
    i64 previousY{};

    bool hasVelocity{};

    i64 velocityX{};
    i64 velocityY{};

    i64 remainderX{};
    i64 remainderY{};

    std::optional<u64> remainingLifetime;

    bool operator==(const EntityState &) const = default;
};

struct WorldState {
    u64 tick{};

    std::size_t aliveCount{};
    std::size_t slotCount{};
    std::size_t poolCount{};

    std::vector<EntityState> entities;

    bool operator==(const WorldState &) const = default;
};

WorldState captureState(const SimulationWorld &world) {
    WorldState result;

    result.tick = world.currentTick().value;

    const auto &registry = world.registry();

    result.aliveCount = registry.aliveCount();
    result.slotCount = registry.slotCount();
    result.poolCount = registry.poolCount();

    registry.eachOrdered<TransformComponent>(
        [&](EntityHandle entity, const TransformComponent &transform) {
            EntityState state;

            state.entity = entity;

            state.x = transform.position.x.raw();
            state.y = transform.position.y.raw();

            state.previousX = transform.previousPosition.x.raw();

            state.previousY = transform.previousPosition.y.raw();

            if (const auto *velocity = registry.tryGet<VelocityComponent>(entity)) {
                state.hasVelocity = true;

                state.velocityX = velocity->x.raw();
                state.velocityY = velocity->y.raw();

                state.remainderX = velocity->remainderX;

                state.remainderY = velocity->remainderY;
            }

            if (const auto *lifetime = registry.tryGet<LifetimeComponent>(entity)) {
                state.remainingLifetime = lifetime->remainingTicks;
            }

            result.entities.push_back(state);
        });

    REQUIRE(result.entities.size() == result.aliveCount);

    return result;
}

SimulationWorld::DeferredEntity queueMovingEntity(SimulationWorld &world, int x, int y,
                                                  int velocityX, int velocityY,
                                                  std::optional<u64> lifetime = std::nullopt) {
    const auto pending = world.queueCreate();

    const Position2 initial{FixedPoint::fromPixels(x), FixedPoint::fromPixels(y)};

    world.queueEmplace<TransformComponent>(pending, initial, initial);

    world.queueEmplace<VelocityComponent>(pending, FixedPoint::fromPixels(velocityX),
                                          FixedPoint::fromPixels(velocityY));

    if (lifetime) {
        world.queueEmplace<LifetimeComponent>(pending, *lifetime);
    }

    return pending;
}

std::vector<WorldState> runReplay(int fps) {
    SimulationWorld world;
    FixedTimestep timestep;

    EntityHandle permanent{};
    EntityHandle moving{};
    EntityHandle temporary{};

    std::optional<SimulationWorld::DeferredEntity> permanentToken;

    std::optional<SimulationWorld::DeferredEntity> movingToken;

    std::optional<SimulationWorld::DeferredEntity> temporaryToken;

    std::vector<WorldState> history;

    history.reserve(180);

    constexpr i64 duration = 3'000'000'000LL;

    const int frameCount = fps * 3;

    for (int frame = 0; frame < frameCount; ++frame) {
        const auto start = duration * frame / frameCount;

        const auto end = duration * (frame + 1) / frameCount;

        timestep.beginFrame(Nanoseconds{end - start});

        while (timestep.hasPendingTick()) {
            const auto tick = timestep.nextTick();

            switch (tick.value) {
            case 1:
                permanentToken = queueMovingEntity(world, 10, 20, 40, 0);

                movingToken = queueMovingEntity(world, 50, 60, 96, -64);

                temporaryToken = queueMovingEntity(world, 0, 0, -40, 20, u64{60});
                break;

            case 30:
                world.queueRemove<VelocityComponent>(moving);
                break;

            case 61:
                REQUIRE_FALSE(world.registry().isAlive(temporary));

                world.queueDestroy(temporary);

                (void)queueMovingEntity(world, 100, 100, 64, 40);
                break;

            case 90:
                world.queueEmplace<VelocityComponent>(moving, FixedPoint::fromPixels(-96),
                                                      FixedPoint::fromPixels(64));
                break;

            case 120:
                world.queueDestroy(permanent);
                break;

            case 150:
                (void)queueMovingEntity(world, -20, -10, 144, -40);
                break;

            default:
                break;
            }

            const auto report = world.step(tick);

            if (tick.value == 1) {
                const auto first = report.beginBarrier.resolve(*permanentToken);

                const auto second = report.beginBarrier.resolve(*movingToken);

                const auto third = report.beginBarrier.resolve(*temporaryToken);

                REQUIRE(first.has_value());
                REQUIRE(second.has_value());
                REQUIRE(third.has_value());

                permanent = *first;
                moving = *second;
                temporary = *third;
            }

            if (tick.value == 61) {
                CHECK(report.beginBarrier.skipped == 1);
            }

            timestep.consumeTick();

            history.push_back(captureState(world));
        }

        const auto timing = timestep.endFrame();

        CHECK(timing.droppedTicks == 0);
        CHECK_FALSE(timing.deltaClamped);
    }

    REQUIRE(world.currentTick().value == 180);

    REQUIRE(timestep.currentTick() == world.currentTick());

    REQUIRE(history.size() == 180);

    CHECK_FALSE(world.failed());

    return history;
}

} // namespace

TEST_CASE("Simulation replay produces identical state across frame rates",
          "[integration][simulation][audit][replay]") {
    const auto reference = runReplay(60);

    constexpr std::array<int, 4> frameRates{30, 120, 144, 240};

    for (const auto fps : frameRates) {
        CAPTURE(fps);

        const auto replay = runReplay(fps);

        REQUIRE(replay.size() == reference.size());

        for (std::size_t i = 0; i < replay.size(); ++i) {
            CAPTURE(i);

            REQUIRE(replay[i] == reference[i]);
        }
    }
}

TEST_CASE("Simulation replay preserves expected entity recycling",
          "[integration][simulation][audit][replay]") {
    const auto history = runReplay(60);

    const auto &initial = history.front();

    CHECK(initial.aliveCount == 3);
    CHECK(initial.slotCount == 3);

    const auto &expired = history[59];

    CHECK(expired.tick == 60);
    CHECK(expired.aliveCount == 2);

    const auto &replacement = history[60];

    REQUIRE(replacement.entities.size() == 3);

    CHECK(replacement.slotCount == 3);

    CHECK(replacement.entities.back().entity.index == 2);

    CHECK(replacement.entities.back().entity.generation == 2);

    const auto &finalState = history.back();

    CHECK(finalState.tick == 180);
    CHECK(finalState.aliveCount == 3);
    CHECK(finalState.slotCount == 3);
    CHECK(finalState.poolCount == 3);

    REQUIRE(finalState.entities.size() == 3);

    CHECK(finalState.entities.front().entity.index == 0);

    CHECK(finalState.entities.front().entity.generation == 2);

    const auto &moving = finalState.entities[1];

    CHECK(moving.entity.index == 1);
    CHECK(moving.entity.generation == 1);

    REQUIRE(moving.hasVelocity);

    const auto initialX = FixedPoint::fromPixels(50).raw();

    const auto initialY = FixedPoint::fromPixels(60).raw();

    const auto forwardX = FixedPoint::fromPixels(96).raw() * 29 / 60;

    const auto forwardY = FixedPoint::fromPixels(-64).raw() * 29 / 60;

    const auto reverseX = FixedPoint::fromPixels(-96).raw() * 91 / 60;

    const auto reverseY = FixedPoint::fromPixels(64).raw() * 91 / 60;

    CHECK(moving.x == initialX + forwardX + reverseX);
    CHECK(moving.y == initialY + forwardY + reverseY);
}

TEST_CASE("Simulation overload advances only executed ticks",
          "[integration][simulation][audit][replay]") {
    SimulationWorld world;
    FixedTimestep timestep;

    const auto pending = queueMovingEntity(world, 0, 0, 60, 0, u64{6});

    EntityHandle entity{};

    timestep.beginFrame(500ms);

    while (timestep.hasPendingTick()) {
        const auto tick = timestep.nextTick();

        const auto report = world.step(tick);

        if (tick.value == 1) {
            const auto resolved = report.beginBarrier.resolve(pending);

            REQUIRE(resolved.has_value());

            entity = *resolved;
        }

        timestep.consumeTick();
    }

    const auto overloaded = timestep.endFrame();

    CHECK(overloaded.executedTicks == 5);
    CHECK(overloaded.droppedTicks == 10);
    CHECK(overloaded.deltaClamped);

    CHECK(world.currentTick().value == 5);

    REQUIRE(world.registry().isAlive(entity));

    CHECK(world.registry().get<TransformComponent>(entity).position.x == FixedPoint::fromPixels(5));

    CHECK(world.registry().get<LifetimeComponent>(entity).remainingTicks == 1);

    timestep.beginFrame(17ms);

    REQUIRE(timestep.hasPendingTick());

    const auto report = world.step(timestep.nextTick());

    timestep.consumeTick();

    const auto normal = timestep.endFrame();

    CHECK(normal.executedTicks == 1);
    CHECK(normal.droppedTicks == 0);

    CHECK(report.endBarrier.applied == 1);

    CHECK(world.currentTick().value == 6);

    CHECK_FALSE(world.registry().isAlive(entity));
}
