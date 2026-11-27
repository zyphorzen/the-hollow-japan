#include "core/Types.hpp"

#include <catch2/catch_test_macros.hpp>

using thj::core::EntityHandle;

TEST_CASE("Entity handle validity", "[unit][entity]") {
    constexpr EntityHandle invalid{};
    constexpr EntityHandle valid{0, 1};

    CHECK_FALSE(invalid.valid());
    CHECK(valid.valid());
}

TEST_CASE("Entity handle packing round trip", "[unit][entity]") {
    constexpr EntityHandle entity{42, 7};

    const auto packed = entity.packed();
    const auto restored = EntityHandle::fromPacked(packed);

    CHECK(restored == entity);
    CHECK(restored.index == 42);
    CHECK(restored.generation == 7);

    constexpr EntityHandle oldEntity{42, 1};
    constexpr EntityHandle newEntity{42, 2};

    CHECK_FALSE(oldEntity == newEntity);
    CHECK(oldEntity.packed() != newEntity.packed());
}