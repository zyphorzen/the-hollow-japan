
#include "ecs/Registry.hpp"
#include "ecs/SparseSet.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <vector>

using thj::core::EntityHandle;
using thj::core::u32;
using thj::core::u64;

using thj::ecs::Registry;
using thj::ecs::SparseSet;

namespace {

struct Position {
    int x{};
    int y{};
};

struct Health {
    int value{};
};

struct ModelEntity {
    std::optional<Position> position;
    std::optional<Health> health;
};

} // namespace

TEST_CASE("Sparse set survives randomized structural mutations", "[unit][ecs][audit][randomized]") {
    constexpr std::array<u32, 3> seeds{42u, 1337u, 0xC0FFEEu};

    constexpr u32 indexCount = 128;
    constexpr int operationCount = 10'000;

    for (const auto seed : seeds) {
        CAPTURE(seed);

        std::mt19937 rng{seed};

        SparseSet storage;

        std::map<u32, u32> model;

        for (int operation = 0; operation < operationCount; ++operation) {
            CAPTURE(operation);

            const auto index = static_cast<u32>(rng() % indexCount);

            const auto generation = static_cast<u32>(1 + rng() % 8);

            const EntityHandle entity{index, generation};

            const auto action = rng() % 8;

            const auto existing = model.find(index);

            const bool occupied = existing != model.end();

            const bool matches = occupied && existing->second == generation;

            switch (action) {
            case 0:
            case 1:
            case 2: {
                const bool inserted = storage.insert(entity);

                REQUIRE(inserted == !occupied);

                if (inserted) {
                    model.emplace(index, generation);
                }

                break;
            }

            case 3: {
                const bool removed = storage.remove(entity);

                REQUIRE(removed == matches);

                if (removed) {
                    model.erase(index);
                }

                break;
            }

            case 4: {

                if (occupied) {
                    const EntityHandle current{index, existing->second};

                    REQUIRE(storage.remove(current));

                    model.erase(index);
                }

                break;
            }

            case 5:
                CHECK(storage.contains(entity) == matches);
                break;

            case 6:
                storage.reserve(static_cast<std::size_t>(rng() % 512));
                break;

            case 7:

                if (operation % 97 == 0) {
                    storage.clear();
                    model.clear();
                }
                break;

            default:
                FAIL("Unexpected randomized action");
            }

            REQUIRE(storage.size() == model.size());

            std::set<u32> visited;

            for (std::size_t denseIndex = 0; denseIndex < storage.size(); ++denseIndex) {
                const auto current = storage.entityAt(denseIndex);

                const auto expected = model.find(current.index);

                REQUIRE(expected != model.end());

                CHECK(current.generation == expected->second);

                REQUIRE(visited.insert(current.index).second);

                const auto mappedIndex = storage.indexOf(current);

                REQUIRE(mappedIndex.has_value());

                CHECK(*mappedIndex == denseIndex);
            }

            for (const auto &[id, gen] : model) {
                CHECK(storage.contains({id, gen}));
            }
        }
    }
}

TEST_CASE("Registry survives randomized entity and component operations",
          "[unit][ecs][audit][randomized]") {
    constexpr std::array<u32, 3> seeds{7u, 2026u, 0xDEADBEEFu};

    constexpr int operationCount = 10'000;

    constexpr std::size_t maximumLiveEntities = 128;

    for (const auto seed : seeds) {
        CAPTURE(seed);

        Registry registry;

        std::mt19937 rng{seed};

        std::map<u64, ModelEntity> model;

        std::vector<EntityHandle> live;
        std::vector<EntityHandle> stale;

        live.reserve(maximumLiveEntities);
        stale.reserve(operationCount);

        for (int operation = 0; operation < operationCount; ++operation) {
            CAPTURE(operation);

            const auto action = rng() % 8;

            if (live.empty() || (action == 0 && live.size() < maximumLiveEntities)) {
                const auto entity = registry.create();

                REQUIRE(entity.valid());

                REQUIRE(model.emplace(entity.packed(), ModelEntity{}).second);

                live.push_back(entity);
            } else {
                const auto selected = static_cast<std::size_t>(rng() % live.size());

                const auto entity = live[selected];

                auto &expected = model.at(entity.packed());

                switch (action) {
                case 0:

                    break;

                case 1:
                    REQUIRE(registry.destroy(entity));

                    stale.push_back(entity);

                    model.erase(entity.packed());

                    live[selected] = live.back();
                    live.pop_back();
                    break;

                case 2:
                    if (!expected.position) {
                        const Position value{static_cast<int>(rng() % 1000),
                                             static_cast<int>(rng() % 1000)};

                        registry.emplace<Position>(entity, value);

                        expected.position = value;
                    } else {
                        REQUIRE_THROWS_AS(registry.emplace<Position>(entity, 0, 0),
                                          std::invalid_argument);
                    }

                    break;

                case 3:
                    CHECK(registry.remove<Position>(entity) == expected.position.has_value());

                    expected.position.reset();
                    break;

                case 4:
                    if (!expected.health) {
                        const Health value{static_cast<int>(rng() % 101)};

                        registry.emplace<Health>(entity, value);

                        expected.health = value;
                    }

                    break;

                case 5:
                    CHECK(registry.remove<Health>(entity) == expected.health.has_value());

                    expected.health.reset();
                    break;

                case 6:
                    if (expected.position) {
                        registry.get<Position>(entity).x += 1;

                        expected.position->x += 1;
                    }

                    if (expected.health) {
                        registry.get<Health>(entity).value -= 1;

                        expected.health->value -= 1;
                    }

                    break;

                case 7:
                    if (!stale.empty()) {
                        const auto previous = stale[rng() % stale.size()];

                        CHECK_FALSE(registry.isAlive(previous));

                        CHECK_FALSE(registry.destroy(previous));

                        CHECK_FALSE(registry.remove<Health>(previous));

                        CHECK(registry.tryGet<Position>(previous) == nullptr);
                    }

                    break;

                default:
                    FAIL("Unexpected randomized action");
                }
            }

            REQUIRE(registry.aliveCount() == model.size());

            REQUIRE(live.size() == model.size());

            std::size_t expectedPositions = 0;
            std::size_t expectedHealths = 0;

            std::set<u64> expectedIntersection;

            for (const auto &[packed, expected] : model) {
                const auto entity = EntityHandle::fromPacked(packed);

                REQUIRE(registry.isAlive(entity));

                CHECK(registry.has<Position>(entity) == expected.position.has_value());

                CHECK(registry.has<Health>(entity) == expected.health.has_value());

                if (expected.position) {
                    ++expectedPositions;

                    const auto &actual = registry.get<Position>(entity);

                    CHECK(actual.x == expected.position->x);

                    CHECK(actual.y == expected.position->y);
                }

                if (expected.health) {
                    ++expectedHealths;

                    CHECK(registry.get<Health>(entity).value == expected.health->value);
                }

                if (expected.position && expected.health) {
                    expectedIntersection.insert(packed);
                }
            }

            CHECK(registry.componentCount<Position>() == expectedPositions);

            CHECK(registry.componentCount<Health>() == expectedHealths);

            std::set<u64> actualIntersection;

            const auto &readOnly = std::as_const(registry);

            const auto matched = readOnly.each<Position, Health>(
                [&](EntityHandle current, const Position &position, const Health &health) {
                    REQUIRE(actualIntersection.insert(current.packed()).second);

                    const auto &expected = model.at(current.packed());

                    REQUIRE(expected.position);
                    REQUIRE(expected.health);

                    CHECK(position.x == expected.position->x);

                    CHECK(health.value == expected.health->value);
                });

            CHECK(matched == expectedIntersection.size());

            CHECK(actualIntersection == expectedIntersection);

            CHECK_FALSE(registry.isIterating());
        }
    }
}
