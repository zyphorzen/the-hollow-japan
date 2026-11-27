#include "ecs/EntityManager.hpp"

#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

using thj::core::EntityHandle;
using thj::core::u32;
using thj::ecs::EntityManager;

TEST_CASE("Entity manager starts empty", "[unit][ecs][entity]") {
    EntityManager manager;

    CHECK(manager.aliveCount() == 0);
    CHECK(manager.slotCount() == 0);

    CHECK_FALSE(manager.isAlive(EntityHandle{}));
}

TEST_CASE("Entity manager creates a valid entity", "[unit][ecs][entity]") {
    EntityManager manager;

    const auto entity = manager.create();

    CHECK(entity.valid());
    CHECK(entity.index == 0);
    CHECK(entity.generation == 1);

    CHECK(manager.isAlive(entity));

    CHECK(manager.aliveCount() == 1);
    CHECK(manager.slotCount() == 1);
}

TEST_CASE("Entity manager creates unique handles", "[unit][ecs][entity]") {
    EntityManager manager;

    const auto first = manager.create();
    const auto second = manager.create();
    const auto third = manager.create();

    CHECK(first != second);
    CHECK(second != third);
    CHECK(first != third);

    CHECK(first.index == 0);
    CHECK(second.index == 1);
    CHECK(third.index == 2);

    CHECK(manager.isAlive(first));
    CHECK(manager.isAlive(second));
    CHECK(manager.isAlive(third));

    CHECK(manager.aliveCount() == 3);
    CHECK(manager.slotCount() == 3);
}

TEST_CASE("Destroyed entity is no longer alive", "[unit][ecs][entity]") {
    EntityManager manager;

    const auto entity = manager.create();

    REQUIRE(manager.destroy(entity));

    CHECK_FALSE(manager.isAlive(entity));

    CHECK(entity.valid());

    CHECK(manager.aliveCount() == 0);
    CHECK(manager.slotCount() == 1);
}

TEST_CASE("Entity manager rejects double destruction", "[unit][ecs][entity]") {
    EntityManager manager;

    const auto entity = manager.create();

    REQUIRE(manager.destroy(entity));

    CHECK_FALSE(manager.destroy(entity));
    CHECK_FALSE(manager.destroy(entity));

    CHECK(manager.aliveCount() == 0);

    const auto reused = manager.create();
    const auto other = manager.create();

    CHECK(reused.index == entity.index);
    CHECK(other.index != reused.index);

    CHECK(manager.aliveCount() == 2);
}

TEST_CASE("Entity manager recycles index with new generation", "[unit][ecs][entity]") {
    EntityManager manager;

    const auto original = manager.create();

    REQUIRE(manager.destroy(original));

    const auto replacement = manager.create();

    CHECK(replacement.index == original.index);

    CHECK(replacement.generation == original.generation + 1);

    CHECK(replacement != original);

    CHECK_FALSE(manager.isAlive(original));
    CHECK(manager.isAlive(replacement));

    CHECK(manager.slotCount() == 1);
    CHECK(manager.aliveCount() == 1);
}

TEST_CASE("Stale entity cannot destroy its replacement", "[unit][ecs][entity]") {
    EntityManager manager;

    const auto oldEntity = manager.create();

    REQUIRE(manager.destroy(oldEntity));

    const auto newEntity = manager.create();

    REQUIRE(newEntity.index == oldEntity.index);

    CHECK_FALSE(manager.destroy(oldEntity));

    CHECK(manager.isAlive(newEntity));
    CHECK(manager.aliveCount() == 1);
}

TEST_CASE("Entity manager rejects invalid handles", "[unit][ecs][entity]") {
    EntityManager manager;

    const auto entity = manager.create();

    const EntityHandle invalidGeneration{entity.index, 0};

    const EntityHandle incorrectGeneration{entity.index, entity.generation + 1};

    const EntityHandle invalidIndex{std::numeric_limits<u32>::max(), 1};

    CHECK_FALSE(manager.isAlive(invalidGeneration));

    CHECK_FALSE(manager.isAlive(incorrectGeneration));

    CHECK_FALSE(manager.isAlive(invalidIndex));

    CHECK_FALSE(manager.destroy(invalidGeneration));

    CHECK_FALSE(manager.destroy(incorrectGeneration));

    CHECK_FALSE(manager.destroy(invalidIndex));

    CHECK(manager.isAlive(entity));
    CHECK(manager.aliveCount() == 1);
}

TEST_CASE("Entity manager recycles slots in LIFO order", "[unit][ecs][entity]") {
    EntityManager manager;

    const auto first = manager.create();
    const auto second = manager.create();
    const auto third = manager.create();
    const auto fourth = manager.create();

    REQUIRE(manager.destroy(second));
    REQUIRE(manager.destroy(fourth));

    const auto reusedFourth = manager.create();
    const auto reusedSecond = manager.create();

    CHECK(reusedFourth.index == fourth.index);
    CHECK(reusedSecond.index == second.index);

    CHECK(reusedFourth.generation == 2);
    CHECK(reusedSecond.generation == 2);

    CHECK(manager.isAlive(first));
    CHECK(manager.isAlive(third));

    CHECK_FALSE(manager.isAlive(second));
    CHECK_FALSE(manager.isAlive(fourth));

    CHECK(manager.aliveCount() == 4);
    CHECK(manager.slotCount() == 4);
}

TEST_CASE("Entity manager reserve preserves existing handles", "[unit][ecs][entity]") {
    EntityManager manager;

    const auto first = manager.create();
    const auto second = manager.create();

    REQUIRE(manager.destroy(second));

    manager.reserve(1024);

    CHECK(manager.isAlive(first));
    CHECK_FALSE(manager.isAlive(second));

    CHECK(manager.aliveCount() == 1);
    CHECK(manager.slotCount() == 2);

    const auto replacement = manager.create();

    CHECK(replacement.index == second.index);
    CHECK(replacement.generation == 2);
    CHECK(manager.isAlive(replacement));
}

TEST_CASE("Entity manager supports repeated index recycling", "[unit][ecs][entity]") {
    EntityManager manager;

    auto entity = manager.create();

    for (u32 cycle = 0; cycle < 1000; ++cycle) {
        const auto previous = entity;

        REQUIRE(manager.destroy(previous));

        CHECK_FALSE(manager.isAlive(previous));

        entity = manager.create();

        CHECK(entity.index == previous.index);

        CHECK(entity.generation == previous.generation + 1);

        CHECK(manager.isAlive(entity));
        CHECK_FALSE(manager.isAlive(previous));
    }

    CHECK(entity.generation == 1001);
    CHECK(manager.slotCount() == 1);
    CHECK(manager.aliveCount() == 1);
}

TEST_CASE("Entity manager maintains bulk lifecycle invariants", "[unit][ecs][entity][stress]") {
    EntityManager manager;

    constexpr std::size_t entityCount = 10'000;

    manager.reserve(entityCount);

    std::vector<EntityHandle> entities;

    entities.reserve(entityCount);

    for (std::size_t i = 0; i < entityCount; ++i) {
        entities.push_back(manager.create());
    }

    REQUIRE(manager.aliveCount() == entityCount);
    REQUIRE(manager.slotCount() == entityCount);

    for (std::size_t i = 0; i < entityCount; i += 2) {
        REQUIRE(manager.destroy(entities[i]));
    }

    CHECK(manager.aliveCount() == entityCount / 2);

    for (std::size_t i = 0; i < entityCount; ++i) {
        CHECK(manager.isAlive(entities[i]) == (i % 2 != 0));
    }

    std::vector<EntityHandle> replacements;

    replacements.reserve(entityCount / 2);

    for (std::size_t i = 0; i < entityCount / 2; ++i) {
        replacements.push_back(manager.create());
    }

    CHECK(manager.aliveCount() == entityCount);
    CHECK(manager.slotCount() == entityCount);

    for (const auto entity : replacements) {
        CHECK(manager.isAlive(entity));
        CHECK(entity.generation == 2);
    }

    for (std::size_t i = 0; i < entityCount; i += 2) {
        CHECK_FALSE(manager.isAlive(entities[i]));

        CHECK_FALSE(manager.destroy(entities[i]));
    }

    for (std::size_t i = 1; i < entityCount; i += 2) {
        REQUIRE(manager.destroy(entities[i]));
    }

    for (const auto entity : replacements) {
        REQUIRE(manager.destroy(entity));
    }

    CHECK(manager.aliveCount() == 0);
    CHECK(manager.slotCount() == entityCount);
}

TEST_CASE("Entity manager rejects oversized reservation", "[unit][ecs][entity]") {
    if constexpr (std::numeric_limits<std::size_t>::max() > std::numeric_limits<u32>::max()) {
        EntityManager manager;

        const auto oversized =
            static_cast<std::size_t>(std::numeric_limits<u32>::max()) + std::size_t{1};

        REQUIRE_THROWS_AS(manager.reserve(oversized), std::length_error);

        CHECK(manager.aliveCount() == 0);
        CHECK(manager.slotCount() == 0);
    }
}

TEST_CASE("Entity manager has exclusive ownership", "[unit][ecs][entity]") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<EntityManager>);

    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<EntityManager>);

    STATIC_REQUIRE_FALSE(std::is_move_constructible_v<EntityManager>);

    STATIC_REQUIRE_FALSE(std::is_move_assignable_v<EntityManager>);
}