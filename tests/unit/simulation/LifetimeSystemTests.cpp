#include "simulation/systems/LifetimeSystem.hpp"

#include "simulation/Components.hpp"

#include <catch2/catch_test_macros.hpp>

using thj::core::u64;

using thj::ecs::CommandBuffer;
using thj::ecs::Registry;

using thj::simulation::LifetimeComponent;
using thj::simulation::LifetimeSystem;
using thj::simulation::TransformComponent;

TEST_CASE("Lifetime system decrements remaining ticks", "[unit][simulation][lifetime]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto entity = registry.create();

    registry.emplace<LifetimeComponent>(entity, u64{3});

    LifetimeSystem::update(registry, commands);

    CHECK(registry.get<LifetimeComponent>(entity).remainingTicks == 2);

    CHECK(commands.empty());

    LifetimeSystem::update(registry, commands);

    CHECK(registry.get<LifetimeComponent>(entity).remainingTicks == 1);

    CHECK(commands.empty());

    LifetimeSystem::update(registry, commands);

    CHECK(registry.get<LifetimeComponent>(entity).remainingTicks == 0);

    CHECK(registry.isAlive(entity));
    CHECK(commands.size() == 1);

    const auto result = commands.playback();

    REQUIRE(result.success());

    CHECK(result.applied == 1);

    CHECK_FALSE(registry.isAlive(entity));
    CHECK(registry.aliveCount() == 0);
}

TEST_CASE("Lifetime system expires zero and one tick lifetimes", "[unit][simulation][lifetime]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto first = registry.create();
    const auto second = registry.create();

    registry.emplace<LifetimeComponent>(first, u64{0});

    registry.emplace<LifetimeComponent>(second, u64{1});

    LifetimeSystem::update(registry, commands);

    CHECK(registry.get<LifetimeComponent>(first).remainingTicks == 0);

    CHECK(registry.get<LifetimeComponent>(second).remainingTicks == 0);

    CHECK(commands.size() == 2);
    CHECK(registry.aliveCount() == 2);

    const auto result = commands.playback();

    REQUIRE(result.success());

    CHECK(result.applied == 2);
    CHECK(registry.aliveCount() == 0);
}

TEST_CASE("Lifetime system ignores entities without lifetime", "[unit][simulation][lifetime]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto permanent = registry.create();
    const auto temporary = registry.create();

    registry.emplace<TransformComponent>(permanent);

    registry.emplace<TransformComponent>(temporary);

    registry.emplace<LifetimeComponent>(temporary, u64{1});

    LifetimeSystem::update(registry, commands);

    const auto result = commands.playback();

    REQUIRE(result.success());

    CHECK(result.applied == 1);

    CHECK(registry.isAlive(permanent));

    CHECK_FALSE(registry.isAlive(temporary));

    CHECK(registry.componentCount<TransformComponent>() == 1);

    CHECK(registry.componentCount<LifetimeComponent>() == 0);
}

TEST_CASE("Lifetime system schedules deterministic destruction order",
          "[unit][simulation][lifetime]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto first = registry.create();
    const auto second = registry.create();
    const auto third = registry.create();

    registry.emplace<LifetimeComponent>(third, u64{1});

    registry.emplace<LifetimeComponent>(second, u64{1});

    registry.emplace<LifetimeComponent>(first, u64{1});

    LifetimeSystem::update(registry, commands);

    REQUIRE(commands.size() == 3);

    const auto result = commands.playback();

    REQUIRE(result.success());

    CHECK(result.applied == 3);

    CHECK(registry.aliveCount() == 0);

    const auto replacementA = registry.create();
    const auto replacementB = registry.create();
    const auto replacementC = registry.create();

    CHECK(replacementA.index == third.index);
    CHECK(replacementB.index == second.index);
    CHECK(replacementC.index == first.index);

    CHECK(replacementA.generation == 2);
    CHECK(replacementB.generation == 2);
    CHECK(replacementC.generation == 2);
}