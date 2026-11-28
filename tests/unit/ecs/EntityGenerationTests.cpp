#include "ecs/EntityManager.hpp"
#include "support/EntityManagerTestAccess.hpp"

#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <stdexcept>

using thj::core::EntityHandle;
using thj::core::u32;
using thj::ecs::EntityManager;
using thj::ecs::testing::EntityManagerTestAccess;

TEST_CASE("Entity generation reaches maximum without wrapping", "[unit][ecs][audit][generation]") {
    EntityManager manager;

    constexpr auto maximum = std::numeric_limits<u32>::max();

    const auto original = manager.create();

    const auto nearMaximum = EntityManagerTestAccess::setGeneration(manager, original, maximum - 1);

    CHECK_FALSE(manager.isAlive(original));

    REQUIRE(manager.isAlive(nearMaximum));

    REQUIRE(manager.destroy(nearMaximum));

    const auto finalGeneration = manager.create();

    CHECK(finalGeneration.index == nearMaximum.index);

    CHECK(finalGeneration.generation == maximum);

    CHECK(manager.isAlive(finalGeneration));
    CHECK_FALSE(manager.isAlive(nearMaximum));

    CHECK(manager.aliveCount() == 1);
    CHECK(manager.slotCount() == 1);
}

TEST_CASE("Entity manager permanently retires exhausted slots", "[unit][ecs][audit][generation]") {
    EntityManager manager;

    constexpr auto maximum = std::numeric_limits<u32>::max();

    const auto first = manager.create();
    const auto reusable = manager.create();

    const auto exhausted = EntityManagerTestAccess::setGeneration(manager, first, maximum);

    REQUIRE(manager.destroy(reusable));
    REQUIRE(manager.destroy(exhausted));

    CHECK_FALSE(manager.isAlive(exhausted));
    CHECK_FALSE(manager.destroy(exhausted));

    CHECK(manager.aliveCount() == 0);
    CHECK(manager.slotCount() == 2);

    const auto replacement = manager.create();

    CHECK(replacement.index == reusable.index);
    CHECK(replacement.generation == 2);

    const auto fresh = manager.create();

    CHECK(fresh.index == 2);
    CHECK(fresh.generation == 1);

    CHECK(fresh.index != exhausted.index);

    CHECK_FALSE(manager.isAlive(exhausted));

    CHECK(manager.aliveCount() == 2);
    CHECK(manager.slotCount() == 3);
}

TEST_CASE("Entity generation test accessor rejects invalid state",
          "[unit][ecs][audit][generation]") {
    EntityManager manager;

    const auto entity = manager.create();

    REQUIRE_THROWS_AS(EntityManagerTestAccess::setGeneration(manager, entity, 0),
                      std::invalid_argument);

    CHECK(manager.isAlive(entity));

    REQUIRE(manager.destroy(entity));

    REQUIRE_THROWS_AS(EntityManagerTestAccess::setGeneration(manager, entity, 10),
                      std::invalid_argument);

    REQUIRE_THROWS_AS(EntityManagerTestAccess::setGeneration(manager, EntityHandle{}, 10),
                      std::invalid_argument);
}