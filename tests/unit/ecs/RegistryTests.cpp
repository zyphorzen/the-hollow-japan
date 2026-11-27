
#include "ecs/Registry.hpp"

#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

using thj::core::EntityHandle;
using thj::core::u32;
using thj::ecs::Registry;

namespace {

struct Position {
    int x{};
    int y{};
};

struct Velocity {
    int x{};
    int y{};
};

struct Health {
    int current{};
    int maximum{};
};

struct MoveOnlyComponent {
    std::unique_ptr<int> value;

    explicit MoveOnlyComponent(int initial) : value(std::make_unique<int>(initial)) {}

    MoveOnlyComponent(MoveOnlyComponent &&) noexcept = default;

    MoveOnlyComponent &operator=(MoveOnlyComponent &&) noexcept = default;
};

struct ThrowingComponent {
    int value{};

    explicit ThrowingComponent(int initial, bool shouldThrow = false) : value(initial) {
        if (shouldThrow) {
            throw std::runtime_error("Component construction failed");
        }
    }
};

struct TrackedResource {
    int &liveCount;

    explicit TrackedResource(int &counter) : liveCount(counter) { ++liveCount; }

    ~TrackedResource() { --liveCount; }

    TrackedResource(const TrackedResource &) = delete;

    TrackedResource &operator=(const TrackedResource &) = delete;
};

struct ResourceComponent {
    std::unique_ptr<TrackedResource> resource;

    explicit ResourceComponent(int &counter)
        : resource(std::make_unique<TrackedResource>(counter)) {}

    ResourceComponent(ResourceComponent &&) noexcept = default;

    ResourceComponent &operator=(ResourceComponent &&) noexcept = default;
};

} // namespace

TEST_CASE("Registry starts empty", "[unit][ecs][registry]") {
    Registry registry;

    CHECK(registry.aliveCount() == 0);
    CHECK(registry.slotCount() == 0);
    CHECK(registry.poolCount() == 0);

    CHECK(registry.componentCount<Position>() == 0);

    CHECK_FALSE(registry.isAlive(EntityHandle{}));
}

TEST_CASE("Registry creates and destroys entities", "[unit][ecs][registry]") {
    Registry registry;

    const auto first = registry.create();
    const auto second = registry.create();

    CHECK(first.valid());
    CHECK(second.valid());

    CHECK(first != second);

    CHECK(registry.isAlive(first));
    CHECK(registry.isAlive(second));

    CHECK(registry.aliveCount() == 2);
    CHECK(registry.slotCount() == 2);

    REQUIRE(registry.destroy(first));

    CHECK_FALSE(registry.isAlive(first));
    CHECK(registry.isAlive(second));

    CHECK(registry.aliveCount() == 1);

    CHECK_FALSE(registry.destroy(first));
}

TEST_CASE("Registry creates component pools lazily", "[unit][ecs][registry]") {
    Registry registry;

    const auto first = registry.create();
    const auto second = registry.create();

    CHECK(registry.poolCount() == 0);

    registry.emplace<Position>(first, 10, 20);

    CHECK(registry.poolCount() == 1);

    registry.emplace<Position>(second, 30, 40);

    CHECK(registry.poolCount() == 1);

    registry.emplace<Health>(first, 100, 100);

    CHECK(registry.poolCount() == 2);

    CHECK(registry.componentCount<Position>() == 2);
    CHECK(registry.componentCount<Health>() == 1);
}

TEST_CASE("Registry read access does not create pools", "[unit][ecs][registry]") {
    Registry registry;

    const auto entity = registry.create();

    CHECK_FALSE(registry.has<Position>(entity));

    CHECK(registry.tryGet<Position>(entity) == nullptr);

    CHECK_FALSE(registry.remove<Position>(entity));

    REQUIRE_THROWS_AS(registry.get<Position>(entity), std::out_of_range);

    const auto &readOnly = std::as_const(registry);

    CHECK_FALSE(readOnly.has<Position>(entity));

    CHECK(readOnly.tryGet<Position>(entity) == nullptr);

    CHECK(readOnly.componentCount<Position>() == 0);

    CHECK(registry.poolCount() == 0);
}

TEST_CASE("Registry isolates different component types", "[unit][ecs][registry]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<Position>(entity, 10, 20);
    registry.emplace<Velocity>(entity, 1, 2);
    registry.emplace<Health>(entity, 75, 100);

    CHECK(registry.poolCount() == 3);

    CHECK(registry.get<Position>(entity).x == 10);
    CHECK(registry.get<Position>(entity).y == 20);

    CHECK(registry.get<Velocity>(entity).x == 1);
    CHECK(registry.get<Velocity>(entity).y == 2);

    CHECK(registry.get<Health>(entity).current == 75);
    CHECK(registry.get<Health>(entity).maximum == 100);

    registry.get<Position>(entity).x += 16;

    CHECK(registry.get<Position>(entity).x == 26);
    CHECK(registry.get<Velocity>(entity).x == 1);
}

TEST_CASE("Registry rejects duplicate components", "[unit][ecs][registry]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<Health>(entity, 100, 150);

    REQUIRE_THROWS_AS(registry.emplace<Health>(entity, 50, 50), std::invalid_argument);

    CHECK(registry.get<Health>(entity).current == 100);
    CHECK(registry.get<Health>(entity).maximum == 150);

    CHECK(registry.componentCount<Health>() == 1);
    CHECK(registry.poolCount() == 1);
}

TEST_CASE("Registry rejects invalid entity operations", "[unit][ecs][registry]") {
    Registry registry;

    constexpr EntityHandle invalid{};

    constexpr EntityHandle missing{999, 1};

    constexpr EntityHandle reserved{std::numeric_limits<u32>::max(), 1};

    for (const auto entity : {invalid, missing, reserved}) {
        CHECK_FALSE(registry.isAlive(entity));
        CHECK_FALSE(registry.destroy(entity));

        CHECK_FALSE(registry.has<Position>(entity));
        CHECK_FALSE(registry.remove<Position>(entity));

        CHECK(registry.tryGet<Position>(entity) == nullptr);

        REQUIRE_THROWS_AS(registry.get<Position>(entity), std::out_of_range);

        REQUIRE_THROWS_AS(registry.emplace<Position>(entity, 1, 2), std::invalid_argument);
    }

    CHECK(registry.aliveCount() == 0);
    CHECK(registry.poolCount() == 0);
}

TEST_CASE("Registry provides mutable component access", "[unit][ecs][registry]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<Health>(entity, 100, 150);

    auto *health = registry.tryGet<Health>(entity);

    REQUIRE(health != nullptr);

    health->current -= 25;

    registry.get<Health>(entity).maximum += 50;

    CHECK(registry.get<Health>(entity).current == 75);
    CHECK(registry.get<Health>(entity).maximum == 200);

    CHECK(health == std::addressof(registry.get<Health>(entity)));
}

TEST_CASE("Registry provides const correct access", "[unit][ecs][registry]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<Position>(entity, 10, 20);

    const auto &readOnly = std::as_const(registry);

    STATIC_REQUIRE(std::is_same_v<decltype(readOnly.get<Position>(entity)), const Position &>);

    STATIC_REQUIRE(std::is_same_v<decltype(readOnly.tryGet<Position>(entity)), const Position *>);

    CHECK(readOnly.isAlive(entity));
    CHECK(readOnly.has<Position>(entity));

    REQUIRE(readOnly.tryGet<Position>(entity) != nullptr);

    CHECK(readOnly.get<Position>(entity).x == 10);
    CHECK(readOnly.get<Position>(entity).y == 20);
}

TEST_CASE("Registry removes individual components", "[unit][ecs][registry]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<Position>(entity, 10, 20);
    registry.emplace<Health>(entity, 100, 100);

    REQUIRE(registry.remove<Health>(entity));

    CHECK(registry.isAlive(entity));

    CHECK(registry.has<Position>(entity));
    CHECK_FALSE(registry.has<Health>(entity));

    CHECK(registry.tryGet<Health>(entity) == nullptr);

    CHECK_FALSE(registry.remove<Health>(entity));

    CHECK(registry.componentCount<Health>() == 0);
    CHECK(registry.componentCount<Position>() == 1);

    CHECK(registry.poolCount() == 2);

    registry.emplace<Health>(entity, 50, 100);

    CHECK(registry.has<Health>(entity));
    CHECK(registry.get<Health>(entity).current == 50);
}

TEST_CASE("Registry destroys all components automatically", "[unit][ecs][registry]") {
    Registry registry;

    const auto player = registry.create();
    const auto enemy = registry.create();

    registry.emplace<Position>(player, 10, 20);
    registry.emplace<Health>(player, 100, 100);
    registry.emplace<Velocity>(player, 1, 2);

    registry.emplace<Position>(enemy, 30, 40);
    registry.emplace<Health>(enemy, 75, 75);

    REQUIRE(registry.destroy(player));

    CHECK_FALSE(registry.isAlive(player));

    CHECK_FALSE(registry.has<Position>(player));
    CHECK_FALSE(registry.has<Health>(player));
    CHECK_FALSE(registry.has<Velocity>(player));

    CHECK(registry.componentCount<Position>() == 1);
    CHECK(registry.componentCount<Health>() == 1);
    CHECK(registry.componentCount<Velocity>() == 0);

    CHECK(registry.isAlive(enemy));

    CHECK(registry.get<Position>(enemy).x == 30);
    CHECK(registry.get<Position>(enemy).y == 40);

    CHECK(registry.get<Health>(enemy).current == 75);

    CHECK(registry.poolCount() == 3);
    CHECK(registry.aliveCount() == 1);
}

TEST_CASE("Registry recycling prevents stale component access", "[unit][ecs][registry]") {
    Registry registry;

    const auto original = registry.create();

    registry.emplace<Position>(original, 10, 20);
    registry.emplace<Health>(original, 100, 100);

    REQUIRE(registry.destroy(original));

    const auto replacement = registry.create();

    REQUIRE(replacement.index == original.index);

    CHECK(replacement.generation == original.generation + 1);

    CHECK_FALSE(registry.isAlive(original));
    CHECK(registry.isAlive(replacement));

    CHECK_FALSE(registry.has<Position>(replacement));
    CHECK_FALSE(registry.has<Health>(replacement));

    registry.emplace<Position>(replacement, 50, 60);

    CHECK_FALSE(registry.has<Position>(original));

    CHECK(registry.tryGet<Position>(original) == nullptr);

    CHECK_FALSE(registry.remove<Position>(original));
    CHECK_FALSE(registry.destroy(original));

    REQUIRE_THROWS_AS(registry.emplace<Health>(original, 1, 1), std::invalid_argument);

    REQUIRE_THROWS_AS(registry.get<Position>(original), std::out_of_range);

    CHECK(registry.isAlive(replacement));

    CHECK(registry.get<Position>(replacement).x == 50);
    CHECK(registry.get<Position>(replacement).y == 60);

    CHECK(registry.componentCount<Position>() == 1);
    CHECK(registry.componentCount<Health>() == 0);
}

TEST_CASE("Registry supports move only components", "[unit][ecs][registry]") {
    Registry registry;

    const auto first = registry.create();
    const auto second = registry.create();

    registry.emplace<MoveOnlyComponent>(first, 100);
    registry.emplace<MoveOnlyComponent>(second, 200);

    REQUIRE(registry.get<MoveOnlyComponent>(first).value != nullptr);

    CHECK(*registry.get<MoveOnlyComponent>(first).value == 100);

    REQUIRE(registry.destroy(first));

    REQUIRE(registry.get<MoveOnlyComponent>(second).value != nullptr);

    CHECK(*registry.get<MoveOnlyComponent>(second).value == 200);

    CHECK(registry.componentCount<MoveOnlyComponent>() == 1);
}

TEST_CASE("Registry releases owned component resources", "[unit][ecs][registry]") {
    int liveResources = 0;

    {
        Registry registry;

        const auto first = registry.create();
        const auto second = registry.create();
        const auto third = registry.create();

        registry.emplace<ResourceComponent>(first, liveResources);

        registry.emplace<ResourceComponent>(second, liveResources);

        registry.emplace<ResourceComponent>(third, liveResources);

        CHECK(liveResources == 3);

        REQUIRE(registry.destroy(second));

        CHECK(liveResources == 2);

        REQUIRE(registry.remove<ResourceComponent>(first));

        CHECK(liveResources == 1);

        CHECK(registry.isAlive(first));
        CHECK(registry.isAlive(third));
    }

    CHECK(liveResources == 0);
}

TEST_CASE("Registry preserves state after component construction failure",
          "[unit][ecs][registry]") {
    Registry registry;

    const auto first = registry.create();
    const auto second = registry.create();

    REQUIRE_THROWS_AS(registry.emplace<ThrowingComponent>(first, 100, true), std::runtime_error);

    CHECK(registry.isAlive(first));

    CHECK_FALSE(registry.has<ThrowingComponent>(first));

    CHECK(registry.componentCount<ThrowingComponent>() == 0);

    CHECK(registry.poolCount() == 1);

    registry.emplace<ThrowingComponent>(first, 200);

    REQUIRE_THROWS_AS(registry.emplace<ThrowingComponent>(second, 300, true), std::runtime_error);

    CHECK(registry.isAlive(second));

    CHECK_FALSE(registry.has<ThrowingComponent>(second));

    CHECK(registry.get<ThrowingComponent>(first).value == 200);

    CHECK(registry.componentCount<ThrowingComponent>() == 1);

    registry.emplace<ThrowingComponent>(second, 400);

    CHECK(registry.get<ThrowingComponent>(second).value == 400);

    CHECK(registry.componentCount<ThrowingComponent>() == 2);
}

TEST_CASE("Registry reserve preserves entities and components", "[unit][ecs][registry]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<Position>(entity, 10, 20);

    registry.reserveEntities(4096);

    registry.reserveComponents<Position>(4096);

    CHECK(registry.isAlive(entity));
    CHECK(registry.has<Position>(entity));

    CHECK(registry.get<Position>(entity).x == 10);
    CHECK(registry.get<Position>(entity).y == 20);

    CHECK(registry.aliveCount() == 1);
    CHECK(registry.slotCount() == 1);
    CHECK(registry.poolCount() == 1);

    registry.reserveComponents<Health>(4096);

    CHECK(registry.poolCount() == 2);
    CHECK(registry.componentCount<Health>() == 0);
}

TEST_CASE("Registry has exclusive ownership", "[unit][ecs][registry]") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<Registry>);

    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<Registry>);

    STATIC_REQUIRE_FALSE(std::is_move_constructible_v<Registry>);

    STATIC_REQUIRE_FALSE(std::is_move_assignable_v<Registry>);
}

TEST_CASE("Registry maintains bulk lifecycle invariants", "[unit][ecs][registry][stress]") {
    Registry registry;

    constexpr u32 entityCount = 10'000;

    registry.reserveEntities(entityCount);

    registry.reserveComponents<Position>(entityCount);
    registry.reserveComponents<Health>(entityCount);

    std::vector<EntityHandle> originalEntities;

    originalEntities.reserve(entityCount);

    for (u32 index = 0; index < entityCount; ++index) {
        const auto entity = registry.create();

        originalEntities.push_back(entity);

        registry.emplace<Position>(entity, static_cast<int>(index), -static_cast<int>(index));

        if (index % 2 == 0) {
            registry.emplace<Health>(entity, 100, 100);
        }
    }

    REQUIRE(registry.aliveCount() == entityCount);

    REQUIRE(registry.componentCount<Position>() == entityCount);

    REQUIRE(registry.componentCount<Health>() == entityCount / 2);

    for (u32 index = 0; index < entityCount; index += 2) {
        REQUIRE(registry.destroy(originalEntities[index]));
    }

    REQUIRE(registry.aliveCount() == entityCount / 2);

    CHECK(registry.componentCount<Position>() == entityCount / 2);

    CHECK(registry.componentCount<Health>() == 0);

    for (u32 index = 1; index < entityCount; index += 2) {
        const auto entity = originalEntities[index];

        REQUIRE(registry.isAlive(entity));

        CHECK(registry.get<Position>(entity).x == static_cast<int>(index));

        CHECK(registry.get<Position>(entity).y == -static_cast<int>(index));
    }

    std::vector<EntityHandle> replacements;

    replacements.reserve(entityCount / 2);

    for (u32 index = 0; index < entityCount / 2; ++index) {
        const auto entity = registry.create();

        replacements.push_back(entity);

        CHECK(entity.generation == 2);

        CHECK(entity.index % 2 == 0);

        CHECK_FALSE(registry.has<Position>(entity));
        CHECK_FALSE(registry.has<Health>(entity));

        registry.emplace<Health>(entity, 200, 250);
    }

    CHECK(registry.aliveCount() == entityCount);
    CHECK(registry.slotCount() == entityCount);

    CHECK(registry.componentCount<Position>() == entityCount / 2);

    CHECK(registry.componentCount<Health>() == entityCount / 2);

    for (u32 index = 0; index < entityCount; index += 2) {
        const auto stale = originalEntities[index];

        CHECK_FALSE(registry.isAlive(stale));

        CHECK_FALSE(registry.destroy(stale));

        CHECK_FALSE(registry.remove<Health>(stale));

        CHECK(registry.tryGet<Health>(stale) == nullptr);
    }

    for (u32 index = 1; index < entityCount; index += 2) {
        REQUIRE(registry.destroy(originalEntities[index]));
    }

    for (const auto entity : replacements) {
        REQUIRE(registry.isAlive(entity));

        CHECK(registry.get<Health>(entity).current == 200);
        CHECK(registry.get<Health>(entity).maximum == 250);

        REQUIRE(registry.destroy(entity));
    }

    CHECK(registry.aliveCount() == 0);

    CHECK(registry.componentCount<Position>() == 0);
    CHECK(registry.componentCount<Health>() == 0);

    CHECK(registry.slotCount() == entityCount);
    CHECK(registry.poolCount() == 2);
}
