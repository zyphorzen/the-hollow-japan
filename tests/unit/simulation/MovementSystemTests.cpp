#include "simulation/systems/MovementSystem.hpp"

#include "simulation/Components.hpp"

#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <stdexcept>

using thj::core::FixedPoint;
using thj::core::i64;
using thj::core::Position2;

using thj::ecs::Registry;

using thj::simulation::MovementSystem;
using thj::simulation::TransformComponent;
using thj::simulation::VelocityComponent;

TEST_CASE("Movement system integrates configured movement speeds", "[unit][simulation][movement]") {
    constexpr int speeds[] = {40, 64, 96, 144};

    for (const int speed : speeds) {
        CAPTURE(speed);

        Registry registry;

        const auto entity = registry.create();

        registry.emplace<TransformComponent>(entity);

        registry.emplace<VelocityComponent>(entity, FixedPoint::fromPixels(speed), FixedPoint{});

        for (int tick = 0; tick < 60; ++tick) {
            MovementSystem::update(registry);
        }

        const auto &transform = registry.get<TransformComponent>(entity);

        const auto &velocity = registry.get<VelocityComponent>(entity);

        CHECK(transform.position.x == FixedPoint::fromPixels(speed));

        CHECK(transform.position.y.raw() == 0);

        CHECK(velocity.remainderX == 0);
        CHECK(velocity.remainderY == 0);
    }
}

TEST_CASE("Movement system integrates negative velocity", "[unit][simulation][movement]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<TransformComponent>(entity);

    registry.emplace<VelocityComponent>(entity, FixedPoint::fromPixels(-96),
                                        FixedPoint::fromPixels(-40));

    MovementSystem::update(registry);

    CHECK(registry.get<TransformComponent>(entity).position.x.raw() == -104857);

    CHECK(registry.get<VelocityComponent>(entity).remainderX == -36);

    for (int tick = 1; tick < 60; ++tick) {
        MovementSystem::update(registry);
    }

    const auto &transform = registry.get<TransformComponent>(entity);

    const auto &velocity = registry.get<VelocityComponent>(entity);

    CHECK(transform.position.x == FixedPoint::fromPixels(-96));

    CHECK(transform.position.y == FixedPoint::fromPixels(-40));

    CHECK(velocity.remainderX == 0);
    CHECK(velocity.remainderY == 0);
}

TEST_CASE("Movement system integrates both axes independently", "[unit][simulation][movement]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<TransformComponent>(
        entity, Position2{FixedPoint::fromPixels(100), FixedPoint::fromPixels(200)});

    registry.emplace<VelocityComponent>(entity, FixedPoint::fromPixels(96),
                                        FixedPoint::fromPixels(-64));

    for (int tick = 0; tick < 120; ++tick) {
        MovementSystem::update(registry);
    }

    const auto &transform = registry.get<TransformComponent>(entity);

    CHECK(transform.position.x == FixedPoint::fromPixels(292));

    CHECK(transform.position.y == FixedPoint::fromPixels(72));
}

TEST_CASE("Movement system preserves previous transforms", "[unit][simulation][movement]") {
    Registry registry;

    const auto entity = registry.create();

    const Position2 initialPosition{FixedPoint::fromPixels(100), FixedPoint::fromPixels(200)};

    registry.emplace<TransformComponent>(entity, initialPosition);

    registry.emplace<VelocityComponent>(entity, FixedPoint::fromPixels(60), FixedPoint{});

    MovementSystem::update(registry);

    const auto first = registry.get<TransformComponent>(entity);

    CHECK(first.previousPosition == initialPosition);

    CHECK(first.position.x == FixedPoint::fromPixels(101));

    MovementSystem::update(registry);

    const auto &second = registry.get<TransformComponent>(entity);

    CHECK(second.previousPosition == first.position);

    CHECK(second.position.x == FixedPoint::fromPixels(102));
}

TEST_CASE("Movement system handles entities missing components", "[unit][simulation][movement]") {
    Registry registry;

    const auto stationary = registry.create();
    const auto velocityOnly = registry.create();
    const auto emptyEntity = registry.create();

    const Position2 position{FixedPoint::fromPixels(20), FixedPoint::fromPixels(30)};

    registry.emplace<TransformComponent>(stationary, position);

    registry.emplace<VelocityComponent>(velocityOnly, FixedPoint::fromPixels(96), FixedPoint{});

    REQUIRE_NOTHROW(MovementSystem::update(registry));

    const auto &transform = registry.get<TransformComponent>(stationary);

    CHECK(transform.position == position);
    CHECK(transform.previousPosition == position);

    CHECK_FALSE(registry.has<TransformComponent>(velocityOnly));

    CHECK(registry.isAlive(emptyEntity));
    CHECK(registry.aliveCount() == 3);
}

TEST_CASE("Movement system preserves subpixel movement", "[unit][simulation][movement]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<TransformComponent>(entity);

    registry.emplace<VelocityComponent>(entity, FixedPoint::fromRaw(1), FixedPoint::fromRaw(-1));

    for (int tick = 0; tick < 59; ++tick) {
        MovementSystem::update(registry);
    }

    CHECK(registry.get<TransformComponent>(entity).position.x.raw() == 0);

    CHECK(registry.get<VelocityComponent>(entity).remainderX == 59);

    MovementSystem::update(registry);

    const auto &transform = registry.get<TransformComponent>(entity);

    const auto &velocity = registry.get<VelocityComponent>(entity);

    CHECK(transform.position.x.raw() == 1);
    CHECK(transform.position.y.raw() == -1);

    CHECK(velocity.remainderX == 0);
    CHECK(velocity.remainderY == 0);
}

TEST_CASE("Movement system preserves remainder on velocity reversal",
          "[unit][simulation][movement]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<TransformComponent>(entity);

    registry.emplace<VelocityComponent>(entity, FixedPoint::fromRaw(1), FixedPoint{});

    for (int tick = 0; tick < 59; ++tick) {
        MovementSystem::update(registry);
    }

    registry.get<VelocityComponent>(entity).x = FixedPoint::fromRaw(-120);

    MovementSystem::update(registry);

    const auto &transform = registry.get<TransformComponent>(entity);

    const auto &velocity = registry.get<VelocityComponent>(entity);

    CHECK(transform.position.x.raw() == -1);

    CHECK(velocity.remainderX == -1);
}

TEST_CASE("Movement system safely integrates extreme velocities", "[unit][simulation][movement]") {
    Registry registry;

    const auto entity = registry.create();

    constexpr auto maximum = std::numeric_limits<i64>::max();

    constexpr auto minimum = std::numeric_limits<i64>::min();

    registry.emplace<TransformComponent>(entity);

    registry.emplace<VelocityComponent>(entity, FixedPoint::fromRaw(maximum),
                                        FixedPoint::fromRaw(minimum), i64{59}, i64{-59});

    REQUIRE_NOTHROW(MovementSystem::update(registry));

    const auto &transform = registry.get<TransformComponent>(entity);

    const auto &velocity = registry.get<VelocityComponent>(entity);

    CHECK(transform.position.x.raw() == maximum / 60 + (maximum % 60 + 59) / 60);

    CHECK(transform.position.y.raw() == minimum / 60 + (minimum % 60 - 59) / 60);

    CHECK(velocity.remainderX > -60);
    CHECK(velocity.remainderX < 60);

    CHECK(velocity.remainderY > -60);
    CHECK(velocity.remainderY < 60);
}

TEST_CASE("Movement system rejects overflow and invalid remainders",
          "[unit][simulation][movement]") {
    Registry registry;

    const auto entity = registry.create();

    const Position2 initial{FixedPoint{}, FixedPoint::fromRaw(std::numeric_limits<i64>::max())};

    registry.emplace<TransformComponent>(entity, initial);

    registry.emplace<VelocityComponent>(entity, FixedPoint::fromPixels(60),
                                        FixedPoint::fromPixels(60));

    SECTION("Position overflow") {
        REQUIRE_THROWS_AS(MovementSystem::update(registry), std::overflow_error);
    }

    SECTION("Invalid remainder") {
        registry.get<VelocityComponent>(entity).remainderY = 60;

        REQUIRE_THROWS_AS(MovementSystem::update(registry), std::logic_error);
    }

    CHECK(registry.get<TransformComponent>(entity).position == initial);

    CHECK(registry.get<VelocityComponent>(entity).remainderX == 0);

    CHECK_FALSE(registry.isIterating());
}