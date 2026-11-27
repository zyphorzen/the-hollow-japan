
#include "ecs/Registry.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

using thj::core::EntityHandle;
using thj::core::u32;
using thj::ecs::QueryComponents;
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

struct EnemyTag {};

struct MutablePositionCallback {
    void operator()(EntityHandle, Position &) const {}
};

template <typename R, typename Fn, typename... Ts>
concept SupportsEach =
    requires(R &registry, Fn &callback) { registry.template each<Ts...>(callback); };

} // namespace

TEST_CASE("ECS query handles an empty registry", "[unit][ecs][query]") {
    Registry registry;

    const auto count = registry.each<Position>(
        [](EntityHandle, Position &) { FAIL("Empty query must not invoke callback"); });

    CHECK(count == 0);
    CHECK(registry.poolCount() == 0);
    CHECK_FALSE(registry.isIterating());
}

TEST_CASE("ECS query handles missing and empty pools", "[unit][ecs][query]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<Position>(entity, 10, 20);

    const auto callback = [](EntityHandle, Position &, Velocity &) {
        FAIL("Unmatched query must not invoke callback");
    };

    CHECK(registry.each<Position, Velocity>(callback) == 0);

    CHECK(registry.poolCount() == 1);

    registry.reserveComponents<Velocity>(16);

    CHECK(registry.poolCount() == 2);

    CHECK(registry.each<Position, Velocity>(callback) == 0);

    CHECK(registry.eachOrdered<Position, Velocity>(callback) == 0);

    CHECK_FALSE(registry.isIterating());
}

TEST_CASE("ECS query iterates a single component type", "[unit][ecs][query]") {
    Registry registry;

    const auto first = registry.create();
    const auto second = registry.create();
    const auto third = registry.create();

    registry.emplace<Position>(first, 10, 20);
    registry.emplace<Position>(second, 30, 40);

    std::vector<EntityHandle> visited;

    const auto count = registry.each<Position>([&](EntityHandle entity, Position &position) {
        visited.push_back(entity);

        position.x += 5;
    });

    CHECK(count == 2);
    REQUIRE(visited.size() == 2);

    CHECK(visited[0] == first);
    CHECK(visited[1] == second);

    CHECK(registry.get<Position>(first).x == 15);
    CHECK(registry.get<Position>(second).x == 35);

    CHECK_FALSE(registry.has<Position>(third));
}

TEST_CASE("ECS query matches component intersection", "[unit][ecs][query]") {
    Registry registry;

    const auto first = registry.create();
    const auto second = registry.create();
    const auto third = registry.create();
    const auto fourth = registry.create();

    registry.emplace<Position>(first, 10, 20);
    registry.emplace<Velocity>(first, 1, 2);

    registry.emplace<Position>(second, 30, 40);

    registry.emplace<Velocity>(third, 3, 4);

    registry.emplace<Position>(fourth, 50, 60);
    registry.emplace<Velocity>(fourth, 5, 6);

    std::vector<EntityHandle> visited;

    const auto count =
        registry.each<Position, Velocity>([&](EntityHandle entity, Position &p, Velocity &v) {
            visited.push_back(entity);

            p.x += v.x;
            p.y += v.y;
        });

    CHECK(count == 2);
    REQUIRE(visited.size() == 2);

    CHECK(visited[0] == first);
    CHECK(visited[1] == fourth);

    CHECK(registry.get<Position>(first).x == 11);
    CHECK(registry.get<Position>(first).y == 22);

    CHECK(registry.get<Position>(second).x == 30);

    CHECK(registry.get<Position>(fourth).x == 55);
    CHECK(registry.get<Position>(fourth).y == 66);
}

TEST_CASE("ECS query supports multiple components and tags", "[unit][ecs][query]") {
    Registry registry;

    const auto enemy = registry.create();
    const auto player = registry.create();
    const auto tree = registry.create();

    registry.emplace<Position>(enemy, 10, 20);
    registry.emplace<Health>(enemy, 100, 100);
    registry.emplace<EnemyTag>(enemy);

    registry.emplace<Position>(player, 30, 40);
    registry.emplace<Health>(player, 200, 200);

    registry.emplace<Position>(tree, 50, 60);

    const auto count = registry.each<Position, Health, EnemyTag>(
        [&](EntityHandle entity, Position &position, Health &health, EnemyTag &) {
            CHECK(entity == enemy);

            position.x += 5;
            health.current -= 25;
        });

    CHECK(count == 1);

    CHECK(registry.get<Position>(enemy).x == 15);
    CHECK(registry.get<Health>(enemy).current == 75);

    CHECK(registry.get<Health>(player).current == 200);

    CHECK(registry.get<Position>(tree).x == 50);
}

TEST_CASE("ECS query selects the smallest component pool", "[unit][ecs][query]") {
    Registry registry;

    const auto first = registry.create();
    const auto second = registry.create();
    const auto third = registry.create();

    registry.emplace<Position>(first, 10, 20);
    registry.emplace<Position>(second, 30, 40);
    registry.emplace<Position>(third, 50, 60);

    registry.emplace<Velocity>(third, 3, 4);
    registry.emplace<Velocity>(first, 1, 2);

    std::vector<EntityHandle> visited;

    const auto count =
        registry.each<Position, Velocity>([&](EntityHandle entity, Position &p, Velocity &v) {
            visited.push_back(entity);

            if (entity == third) {
                CHECK(p.x == 50);
                CHECK(v.x == 3);
            }

            if (entity == first) {
                CHECK(p.x == 10);
                CHECK(v.x == 1);
            }
        });

    CHECK(count == 2);
    REQUIRE(visited.size() == 2);

    CHECK(visited[0] == third);
    CHECK(visited[1] == first);
}

TEST_CASE("ECS query supports const correct iteration", "[unit][ecs][query]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<Position>(entity, 10, 20);
    registry.emplace<Health>(entity, 75, 100);

    const auto &readOnly = std::as_const(registry);

    const auto count = readOnly.each<Position, Health>([&](EntityHandle current, auto &p, auto &h) {
        STATIC_REQUIRE(std::is_same_v<decltype(p), const Position &>);

        STATIC_REQUIRE(std::is_same_v<decltype(h), const Health &>);

        CHECK(current == entity);

        CHECK(p.x == 10);
        CHECK(p.y == 20);
        CHECK(h.current == 75);

        CHECK(readOnly.isIterating());
    });

    CHECK(count == 1);
    CHECK_FALSE(registry.isIterating());
}

TEST_CASE("ECS ordered query sorts entity indices", "[unit][ecs][query]") {
    Registry registry;

    const auto first = registry.create();
    const auto second = registry.create();
    const auto third = registry.create();

    registry.emplace<Position>(third, 30, 0);
    registry.emplace<Position>(first, 10, 0);
    registry.emplace<Position>(second, 20, 0);

    std::vector<EntityHandle> visited;

    const auto count = registry.eachOrdered<Position>([&](EntityHandle entity, Position &position) {
        visited.push_back(entity);

        position.x += 1;
    });

    CHECK(count == 3);
    REQUIRE(visited.size() == 3);

    CHECK(visited[0] == first);
    CHECK(visited[1] == second);
    CHECK(visited[2] == third);

    CHECK(registry.get<Position>(first).x == 11);
    CHECK(registry.get<Position>(second).x == 21);
    CHECK(registry.get<Position>(third).x == 31);

    visited.clear();

    registry.each<Position>([&](EntityHandle entity, Position &) { visited.push_back(entity); });

    REQUIRE(visited.size() == 3);

    CHECK(visited[0] == third);
    CHECK(visited[1] == first);
    CHECK(visited[2] == second);
}

TEST_CASE("ECS ordered const query handles recycled entities", "[unit][ecs][query]") {
    Registry registry;

    const auto original = registry.create();
    const auto second = registry.create();
    const auto third = registry.create();

    registry.emplace<Position>(original, 10, 0);
    registry.emplace<Position>(second, 20, 0);
    registry.emplace<Position>(third, 30, 0);

    registry.emplace<Health>(second, 100, 100);
    registry.emplace<Health>(third, 75, 100);

    REQUIRE(registry.destroy(original));

    const auto replacement = registry.create();

    REQUIRE(replacement.index == original.index);
    REQUIRE(replacement.generation != original.generation);

    registry.emplace<Position>(replacement, 40, 0);
    registry.emplace<Health>(replacement, 150, 150);

    const auto &readOnly = std::as_const(registry);

    std::vector<EntityHandle> visited;

    const auto count = readOnly.eachOrdered<Position, Health>(
        [&](EntityHandle entity, const Position &, const Health &) { visited.push_back(entity); });

    CHECK(count == 3);
    REQUIRE(visited.size() == 3);

    CHECK(visited[0] == replacement);
    CHECK(visited[1] == second);
    CHECK(visited[2] == third);

    CHECK(std::find(visited.begin(), visited.end(), original) == visited.end());
}

TEST_CASE("ECS query rejects structural mutations", "[unit][ecs][query]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<Position>(entity, 10, 20);
    registry.emplace<Health>(entity, 100, 100);

    const auto count = registry.each<Position>([&](EntityHandle current, Position &position) {
        CHECK(registry.isIterating());

        REQUIRE_THROWS_AS(registry.create(), std::logic_error);

        REQUIRE_THROWS_AS(registry.destroy(current), std::logic_error);

        REQUIRE_THROWS_AS(registry.emplace<Velocity>(current, 1, 2), std::logic_error);

        REQUIRE_THROWS_AS(registry.remove<Health>(current), std::logic_error);

        REQUIRE_THROWS_AS(registry.reserveEntities(1024), std::logic_error);

        REQUIRE_THROWS_AS(registry.reserveComponents<Position>(1024), std::logic_error);

        REQUIRE_THROWS_AS(registry.reserveComponents<Velocity>(1024), std::logic_error);

        position.x += 5;

        CHECK(registry.has<Health>(current));

        registry.get<Health>(current).current -= 10;

        CHECK(registry.tryGet<Position>(current) != nullptr);
    });

    CHECK(count == 1);

    CHECK_FALSE(registry.isIterating());

    CHECK(registry.isAlive(entity));
    CHECK(registry.aliveCount() == 1);
    CHECK(registry.poolCount() == 2);

    CHECK(registry.get<Position>(entity).x == 15);
    CHECK(registry.get<Health>(entity).current == 90);

    CHECK_FALSE(registry.has<Velocity>(entity));

    REQUIRE(registry.destroy(entity));
}

TEST_CASE("ECS const query blocks structural mutation through alias", "[unit][ecs][query]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<Position>(entity, 10, 20);

    const auto &readOnly = std::as_const(registry);

    const auto count = readOnly.eachOrdered<Position>([&](EntityHandle current, const Position &p) {
        CHECK(readOnly.isIterating());

        REQUIRE_THROWS_AS(registry.destroy(current), std::logic_error);

        REQUIRE_THROWS_AS(registry.emplace<Health>(current, 100, 100), std::logic_error);

        CHECK(p.x == 10);
    });

    CHECK(count == 1);

    CHECK_FALSE(registry.isIterating());

    CHECK(registry.isAlive(entity));
    CHECK(registry.poolCount() == 1);

    REQUIRE(registry.destroy(entity));
}

TEST_CASE("ECS query supports nested iteration guards", "[unit][ecs][query]") {
    Registry registry;

    const auto first = registry.create();
    const auto second = registry.create();

    registry.emplace<Position>(first, 10, 20);
    registry.emplace<Position>(second, 30, 40);

    registry.emplace<Health>(first, 100, 100);
    registry.emplace<Health>(second, 75, 100);

    std::size_t innerVisits = 0;

    const auto outerCount = registry.each<Position>([&](EntityHandle outerEntity, Position &) {
        CHECK(registry.isIterating());

        const auto innerCount = registry.eachOrdered<Health>([&](EntityHandle, Health &) {
            CHECK(registry.isIterating());

            ++innerVisits;

            REQUIRE_THROWS_AS(registry.destroy(outerEntity), std::logic_error);
        });

        CHECK(innerCount == 2);

        CHECK(registry.isIterating());

        REQUIRE_THROWS_AS(registry.remove<Position>(outerEntity), std::logic_error);
    });

    CHECK(outerCount == 2);
    CHECK(innerVisits == 4);

    CHECK_FALSE(registry.isIterating());

    REQUIRE(registry.destroy(first));
    REQUIRE(registry.destroy(second));
}

TEST_CASE("ECS query releases guard after callback exception", "[unit][ecs][query]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<Position>(entity, 10, 20);

    const auto throwingCallback = [](EntityHandle, Position &position) {
        position.x += 1;

        throw std::runtime_error("Query callback failed");
    };

    REQUIRE_THROWS_AS(registry.each<Position>(throwingCallback), std::runtime_error);

    CHECK_FALSE(registry.isIterating());

    CHECK(registry.get<Position>(entity).x == 11);

    REQUIRE_THROWS_AS(registry.eachOrdered<Position>(throwingCallback), std::runtime_error);

    CHECK_FALSE(registry.isIterating());

    CHECK(registry.get<Position>(entity).x == 12);

    const auto next = registry.create();

    registry.emplace<Position>(next, 30, 40);

    CHECK(registry.aliveCount() == 2);

    REQUIRE(registry.destroy(entity));
    REQUIRE(registry.destroy(next));
}

TEST_CASE("ECS nested query exception preserves outer guard", "[unit][ecs][query]") {
    Registry registry;

    const auto entity = registry.create();

    registry.emplace<Position>(entity, 10, 20);
    registry.emplace<Health>(entity, 100, 100);

    const auto count = registry.each<Position>([&](EntityHandle current, Position &) {
        REQUIRE_THROWS_AS(registry.each<Health>([](EntityHandle, Health &) {
            throw std::runtime_error("Nested query failed");
        }),
                          std::runtime_error);

        CHECK(registry.isIterating());

        REQUIRE_THROWS_AS(registry.destroy(current), std::logic_error);
    });

    CHECK(count == 1);
    CHECK_FALSE(registry.isIterating());

    REQUIRE(registry.destroy(entity));
}

TEST_CASE("ECS query enforces compile time constraints", "[unit][ecs][query]") {
    STATIC_REQUIRE(QueryComponents<Position>);

    STATIC_REQUIRE(QueryComponents<Position, Velocity>);

    STATIC_REQUIRE(QueryComponents<Position, Velocity, Health>);

    STATIC_REQUIRE_FALSE(QueryComponents<>);

    STATIC_REQUIRE_FALSE(QueryComponents<Position, Position>);

    STATIC_REQUIRE_FALSE(QueryComponents<Position, Health, Position>);

    STATIC_REQUIRE_FALSE(QueryComponents<const Position>);

    STATIC_REQUIRE_FALSE(QueryComponents<bool>);

    STATIC_REQUIRE(SupportsEach<Registry, MutablePositionCallback, Position>);

    STATIC_REQUIRE_FALSE(SupportsEach<const Registry, MutablePositionCallback, Position>);

    STATIC_REQUIRE_FALSE(SupportsEach<Registry, MutablePositionCallback, Position, Position>);
}

TEST_CASE("ECS query maintains bulk intersection invariants", "[unit][ecs][query][stress]") {
    Registry registry;

    constexpr u32 entityCount = 10'000;

    registry.reserveEntities(entityCount);
    registry.reserveComponents<Position>(entityCount);

    std::vector<EntityHandle> entities;

    entities.reserve(entityCount);

    std::size_t expectedMatches = 0;

    for (u32 i = 0; i < entityCount; ++i) {
        const auto entity = registry.create();

        entities.push_back(entity);

        registry.emplace<Position>(entity, static_cast<int>(i), 0);

        if (i % 2 == 0) {
            registry.emplace<Velocity>(entity, 2, 3);
        }

        if (i % 5 == 0) {
            registry.emplace<Health>(entity, 100, 100);
        }

        if (i % 10 == 0) {
            ++expectedMatches;
        }
    }

    REQUIRE(expectedMatches == 1000);

    const auto matchedCount = registry.each<Position, Velocity, Health>(
        [](EntityHandle entity, Position &position, Velocity &velocity, Health &health) {
            CHECK(entity.index % 10 == 0);

            position.x += velocity.x;
            position.y += velocity.y;

            health.current -= 1;
        });

    CHECK(matchedCount == expectedMatches);

    for (u32 i = 0; i < entityCount; ++i) {
        const auto entity = entities[i];

        const auto &position = registry.get<Position>(entity);

        if (i % 10 == 0) {
            CHECK(position.x == static_cast<int>(i) + 2);
            CHECK(position.y == 3);

            CHECK(registry.get<Health>(entity).current == 99);
        } else {
            CHECK(position.x == static_cast<int>(i));
            CHECK(position.y == 0);
        }
    }

    for (u32 i = 0; i < entityCount; i += 20) {
        REQUIRE(registry.destroy(entities[i]));
    }

    CHECK(registry.aliveCount() == 9500);

    std::vector<u32> visitedIndices;

    const auto &readOnly = std::as_const(registry);

    const auto remainingCount = readOnly.eachOrdered<Position, Velocity, Health>(
        [&](EntityHandle entity, const Position &position, const Velocity &, const Health &health) {
            CHECK(registry.isAlive(entity));

            CHECK(entity.index % 10 == 0);
            CHECK(entity.index % 20 != 0);

            CHECK(position.y == 3);
            CHECK(health.current == 99);

            visitedIndices.push_back(entity.index);
        });

    CHECK(remainingCount == 500);
    CHECK(visitedIndices.size() == 500);

    CHECK(std::is_sorted(visitedIndices.begin(), visitedIndices.end()));

    CHECK(std::adjacent_find(visitedIndices.begin(), visitedIndices.end()) == visitedIndices.end());

    CHECK_FALSE(registry.isIterating());
}
