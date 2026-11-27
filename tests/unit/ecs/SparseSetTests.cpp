#include "ecs/SparseSet.hpp"

#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <stdexcept>
#include <type_traits>

using thj::core::EntityHandle;
using thj::core::u32;
using thj::ecs::SparseSet;

TEST_CASE("Sparse set starts empty", "[unit][ecs][sparse-set]") {
    SparseSet storage;

    CHECK(storage.empty());
    CHECK(storage.size() == 0);
    CHECK(storage.entities().empty());

    CHECK_FALSE(storage.contains(EntityHandle{0, 1}));

    CHECK_FALSE(storage.indexOf(EntityHandle{0, 1}).has_value());
}

TEST_CASE("Sparse set inserts and locates entities", "[unit][ecs][sparse-set]") {
    SparseSet storage;

    constexpr EntityHandle first{1, 1};
    constexpr EntityHandle second{4, 1};
    constexpr EntityHandle third{7, 1};

    REQUIRE(storage.insert(first));
    REQUIRE(storage.insert(second));
    REQUIRE(storage.insert(third));

    CHECK_FALSE(storage.empty());
    CHECK(storage.size() == 3);

    CHECK(storage.contains(first));
    CHECK(storage.contains(second));
    CHECK(storage.contains(third));

    REQUIRE(storage.indexOf(first).has_value());
    REQUIRE(storage.indexOf(second).has_value());
    REQUIRE(storage.indexOf(third).has_value());

    CHECK(*storage.indexOf(first) == 0);
    CHECK(*storage.indexOf(second) == 1);
    CHECK(*storage.indexOf(third) == 2);

    CHECK(storage.entityAt(0) == first);
    CHECK(storage.entityAt(1) == second);
    CHECK(storage.entityAt(2) == third);
}

TEST_CASE("Sparse set rejects duplicate insertion", "[unit][ecs][sparse-set]") {
    SparseSet storage;

    constexpr EntityHandle entity{10, 1};

    REQUIRE(storage.insert(entity));

    CHECK_FALSE(storage.insert(entity));
    CHECK_FALSE(storage.insert(entity));

    CHECK(storage.size() == 1);
    CHECK(storage.contains(entity));

    CHECK(storage.entityAt(0) == entity);
}

TEST_CASE("Sparse set rejects invalid entity handles", "[unit][ecs][sparse-set]") {
    SparseSet storage;

    constexpr EntityHandle invalid{0, 0};

    constexpr EntityHandle invalidGeneration{42, 0};

    constexpr EntityHandle reservedIndex{std::numeric_limits<u32>::max(), 1};

    CHECK_FALSE(storage.insert(invalid));
    CHECK_FALSE(storage.insert(invalidGeneration));
    CHECK_FALSE(storage.insert(reservedIndex));

    CHECK_FALSE(storage.contains(invalid));
    CHECK_FALSE(storage.remove(invalid));

    CHECK(storage.empty());
}

TEST_CASE("Sparse set expands sparse mapping", "[unit][ecs][sparse-set]") {
    SparseSet storage;

    constexpr EntityHandle low{0, 1};
    constexpr EntityHandle high{10'000, 1};

    REQUIRE(storage.insert(low));
    REQUIRE(storage.insert(high));

    CHECK(storage.size() == 2);

    CHECK(storage.contains(low));
    CHECK(storage.contains(high));

    REQUIRE(storage.indexOf(high).has_value());

    CHECK(*storage.indexOf(high) == 1);

    CHECK_FALSE(storage.contains(EntityHandle{9'999, 1}));

    CHECK_FALSE(storage.contains(EntityHandle{20'000, 1}));
}

TEST_CASE("Sparse set lookup validates generation", "[unit][ecs][sparse-set]") {
    SparseSet storage;

    constexpr EntityHandle current{42, 2};
    constexpr EntityHandle stale{42, 1};
    constexpr EntityHandle future{42, 3};

    REQUIRE(storage.insert(current));

    CHECK(storage.contains(current));

    CHECK_FALSE(storage.contains(stale));
    CHECK_FALSE(storage.contains(future));

    CHECK_FALSE(storage.indexOf(stale).has_value());

    CHECK_FALSE(storage.indexOf(future).has_value());

    CHECK_FALSE(storage.remove(stale));
    CHECK_FALSE(storage.remove(future));

    CHECK(storage.contains(current));
    CHECK(storage.size() == 1);
}

TEST_CASE("Sparse set rejects generation conflicts", "[unit][ecs][sparse-set]") {
    SparseSet storage;

    constexpr EntityHandle original{5, 1};
    constexpr EntityHandle replacement{5, 2};

    REQUIRE(storage.insert(original));

    CHECK_FALSE(storage.insert(replacement));

    CHECK(storage.contains(original));
    CHECK_FALSE(storage.contains(replacement));

    CHECK(storage.size() == 1);

    REQUIRE(storage.remove(original));

    REQUIRE(storage.insert(replacement));

    CHECK_FALSE(storage.contains(original));
    CHECK(storage.contains(replacement));

    CHECK_FALSE(storage.remove(original));

    CHECK(storage.size() == 1);
}

TEST_CASE("Sparse set removes middle entity by swap and pop", "[unit][ecs][sparse-set]") {
    SparseSet storage;

    constexpr EntityHandle first{1, 1};
    constexpr EntityHandle middle{4, 1};
    constexpr EntityHandle last{7, 1};

    REQUIRE(storage.insert(first));
    REQUIRE(storage.insert(middle));
    REQUIRE(storage.insert(last));

    REQUIRE(storage.remove(middle));

    CHECK(storage.size() == 2);

    CHECK_FALSE(storage.contains(middle));

    CHECK(storage.contains(first));
    CHECK(storage.contains(last));

    CHECK(storage.entityAt(0) == first);
    CHECK(storage.entityAt(1) == last);

    REQUIRE(storage.indexOf(last).has_value());

    CHECK(*storage.indexOf(last) == 1);

    CHECK_FALSE(storage.indexOf(middle).has_value());
}

TEST_CASE("Sparse set removes first entity correctly", "[unit][ecs][sparse-set]") {
    SparseSet storage;

    constexpr EntityHandle first{10, 1};
    constexpr EntityHandle second{20, 1};
    constexpr EntityHandle last{30, 1};

    REQUIRE(storage.insert(first));
    REQUIRE(storage.insert(second));
    REQUIRE(storage.insert(last));

    REQUIRE(storage.remove(first));

    CHECK(storage.size() == 2);

    CHECK(storage.entityAt(0) == last);
    CHECK(storage.entityAt(1) == second);

    CHECK_FALSE(storage.contains(first));
    CHECK(storage.contains(second));
    CHECK(storage.contains(last));

    REQUIRE(storage.indexOf(last).has_value());

    CHECK(*storage.indexOf(last) == 0);
}

TEST_CASE("Sparse set removes last and sole entity", "[unit][ecs][sparse-set]") {
    SparseSet storage;

    constexpr EntityHandle first{10, 1};
    constexpr EntityHandle last{20, 1};

    REQUIRE(storage.insert(first));
    REQUIRE(storage.insert(last));

    REQUIRE(storage.remove(last));

    CHECK(storage.size() == 1);

    CHECK(storage.entityAt(0) == first);

    CHECK_FALSE(storage.contains(last));

    REQUIRE(storage.remove(first));

    CHECK(storage.empty());
    CHECK(storage.entities().empty());

    CHECK_FALSE(storage.contains(first));
}

TEST_CASE("Sparse set rejects removal of absent entities", "[unit][ecs][sparse-set]") {
    SparseSet storage;

    constexpr EntityHandle entity{15, 1};
    constexpr EntityHandle missing{99, 1};

    CHECK_FALSE(storage.remove(entity));

    REQUIRE(storage.insert(entity));

    CHECK_FALSE(storage.remove(missing));

    REQUIRE(storage.remove(entity));

    CHECK_FALSE(storage.remove(entity));

    CHECK(storage.empty());
}

TEST_CASE("Sparse set exposes contiguous read only iteration", "[unit][ecs][sparse-set]") {
    SparseSet storage;

    REQUIRE(storage.insert({2, 1}));
    REQUIRE(storage.insert({5, 1}));
    REQUIRE(storage.insert({8, 1}));

    const auto view = storage.entities();

    REQUIRE(view.size() == 3);

    CHECK(view[0] == (EntityHandle{2, 1}));
    CHECK(view[1] == (EntityHandle{5, 1}));
    CHECK(view[2] == (EntityHandle{8, 1}));

    std::size_t visited = 0;

    for (const auto entity : view) {
        CHECK(storage.contains(entity));

        ++visited;
    }

    CHECK(visited == storage.size());

    STATIC_REQUIRE(std::is_same_v<decltype(storage.entities()), std::span<const EntityHandle>>);
}

TEST_CASE("Sparse set rejects out of range dense access", "[unit][ecs][sparse-set]") {
    SparseSet storage;

    REQUIRE_THROWS_AS(storage.entityAt(0), std::out_of_range);

    REQUIRE(storage.insert({1, 1}));

    REQUIRE_THROWS_AS(storage.entityAt(1), std::out_of_range);
}

TEST_CASE("Sparse set reserve preserves membership", "[unit][ecs][sparse-set]") {
    SparseSet storage;

    constexpr EntityHandle first{4, 1};
    constexpr EntityHandle second{8, 1};

    REQUIRE(storage.insert(first));
    REQUIRE(storage.insert(second));

    storage.reserve(1024);

    CHECK(storage.size() == 2);

    CHECK(storage.contains(first));
    CHECK(storage.contains(second));

    CHECK(storage.entityAt(0) == first);
    CHECK(storage.entityAt(1) == second);
}

TEST_CASE("Sparse set clear resets membership", "[unit][ecs][sparse-set]") {
    SparseSet storage;

    constexpr EntityHandle first{1, 1};
    constexpr EntityHandle second{10, 1};

    REQUIRE(storage.insert(first));
    REQUIRE(storage.insert(second));

    storage.clear();

    CHECK(storage.empty());
    CHECK(storage.size() == 0);

    CHECK_FALSE(storage.contains(first));
    CHECK_FALSE(storage.contains(second));

    CHECK_FALSE(storage.remove(first));

    constexpr EntityHandle replacement{1, 2};

    REQUIRE(storage.insert(replacement));

    CHECK(storage.contains(replacement));
    CHECK_FALSE(storage.contains(first));

    CHECK(storage.size() == 1);

    storage.clear();
    storage.clear();

    CHECK(storage.empty());
}

TEST_CASE("Sparse set maintains bulk mapping invariants", "[unit][ecs][sparse-set][stress]") {
    SparseSet storage;

    constexpr u32 entityCount = 10'000;

    storage.reserve(entityCount);

    for (u32 index = 0; index < entityCount; ++index) {
        REQUIRE(storage.insert({index, 1}));
    }

    REQUIRE(storage.size() == entityCount);

    for (u32 index = 0; index < entityCount; index += 2) {
        REQUIRE(storage.remove({index, 1}));
    }

    REQUIRE(storage.size() == entityCount / 2);

    for (u32 index = 0; index < entityCount; ++index) {
        CHECK(storage.contains({index, 1}) == (index % 2 != 0));
    }

    for (std::size_t index = 0; index < storage.size(); ++index) {
        const auto entity = storage.entityAt(index);

        const auto mappedIndex = storage.indexOf(entity);

        REQUIRE(mappedIndex.has_value());

        CHECK(*mappedIndex == index);

        CHECK(entity.index % 2 != 0);
    }

    for (u32 index = 0; index < entityCount; index += 2) {
        REQUIRE(storage.insert({index, 2}));
    }

    REQUIRE(storage.size() == entityCount);

    for (u32 index = 0; index < entityCount; index += 2) {
        CHECK_FALSE(storage.contains({index, 1}));

        CHECK_FALSE(storage.remove({index, 1}));

        CHECK(storage.contains({index, 2}));
    }

    for (std::size_t index = 0; index < storage.size(); ++index) {
        const auto entity = storage.entityAt(index);

        const auto mappedIndex = storage.indexOf(entity);

        REQUIRE(mappedIndex.has_value());

        CHECK(*mappedIndex == index);

        CHECK(entity.generation == (entity.index % 2 == 0 ? 2u : 1u));
    }

    while (!storage.empty()) {
        const auto entity = storage.entityAt(storage.size() - 1);

        REQUIRE(storage.remove(entity));
    }

    CHECK(storage.empty());
    CHECK(storage.entities().empty());
}