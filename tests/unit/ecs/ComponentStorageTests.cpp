
#include "ecs/ComponentStorage.hpp"

#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

using thj::core::EntityHandle;
using thj::core::u32;

using thj::ecs::ComponentStorage;
using thj::ecs::ComponentType;

namespace {

struct Position {
    int x{};
    int y{};
};

struct Health {
    int current{};
    int maximum{};
};

struct MoveOnlyComponent {
    std::unique_ptr<int> value;

    explicit MoveOnlyComponent(int initialValue) : value(std::make_unique<int>(initialValue)) {}

    MoveOnlyComponent(const MoveOnlyComponent &) = delete;
    MoveOnlyComponent &operator=(const MoveOnlyComponent &) = delete;

    MoveOnlyComponent(MoveOnlyComponent &&) noexcept = default;

    MoveOnlyComponent &operator=(MoveOnlyComponent &&) noexcept = default;
};

struct ThrowingComponent {
    int value{};

    explicit ThrowingComponent(int initialValue, bool shouldThrow = false) : value(initialValue) {

        if (shouldThrow) {
            throw std::runtime_error("Component construction failed");
        }
    }
};

struct ThrowingMoveComponent {
    ThrowingMoveComponent() = default;

    ThrowingMoveComponent(ThrowingMoveComponent &&) noexcept(false) {}

    ThrowingMoveComponent &operator=(ThrowingMoveComponent &&) noexcept(false) { return *this; }
};

struct Resource {
    int &liveCount;

    explicit Resource(int &counter) : liveCount(counter) { ++liveCount; }

    ~Resource() { --liveCount; }

    Resource(const Resource &) = delete;
    Resource &operator=(const Resource &) = delete;
};

struct ResourceComponent {
    std::unique_ptr<Resource> resource;

    explicit ResourceComponent(int &liveCount) : resource(std::make_unique<Resource>(liveCount)) {}

    ResourceComponent(ResourceComponent &&) noexcept = default;

    ResourceComponent &operator=(ResourceComponent &&) noexcept = default;
};

} // namespace

TEST_CASE("Component storage starts empty", "[unit][ecs][component-storage]") {
    ComponentStorage<Position> storage;

    constexpr EntityHandle entity{0, 1};

    CHECK(storage.empty());
    CHECK(storage.size() == 0);

    CHECK(storage.entities().empty());
    CHECK(storage.components().empty());

    CHECK_FALSE(storage.contains(entity));

    CHECK(storage.tryGet(entity) == nullptr);
}

TEST_CASE("Component storage constructs component in place", "[unit][ecs][component-storage]") {
    ComponentStorage<Position> storage;

    constexpr EntityHandle entity{1, 1};

    auto &position = storage.emplace(entity, 32, 64);

    CHECK(position.x == 32);
    CHECK(position.y == 64);

    CHECK(storage.contains(entity));
    CHECK(storage.size() == 1);

    REQUIRE(storage.tryGet(entity) != nullptr);

    CHECK(storage.tryGet(entity) == &position);
    CHECK(&storage.get(entity) == &position);
}

TEST_CASE("Component storage supports default construction", "[unit][ecs][component-storage]") {
    ComponentStorage<Position> storage;

    constexpr EntityHandle entity{0, 1};

    const auto &component = storage.emplace(entity);

    CHECK(component.x == 0);
    CHECK(component.y == 0);

    CHECK(storage.contains(entity));
}

TEST_CASE("Component storage rejects duplicate components", "[unit][ecs][component-storage]") {
    ComponentStorage<Health> storage;

    constexpr EntityHandle entity{1, 1};

    storage.emplace(entity, 100, 150);

    REQUIRE_THROWS_AS(storage.emplace(entity, 50, 75), std::invalid_argument);

    CHECK(storage.size() == 1);
    CHECK(storage.contains(entity));

    CHECK(storage.get(entity).current == 100);
    CHECK(storage.get(entity).maximum == 150);
}

TEST_CASE("Component storage rejects invalid entity handles", "[unit][ecs][component-storage]") {
    ComponentStorage<Position> storage;

    constexpr EntityHandle invalid{0, 0};

    constexpr EntityHandle reserved{std::numeric_limits<u32>::max(), 1};

    REQUIRE_THROWS_AS(storage.emplace(invalid, 1, 2), std::invalid_argument);

    REQUIRE_THROWS_AS(storage.emplace(reserved, 1, 2), std::invalid_argument);

    CHECK(storage.empty());
    CHECK_FALSE(storage.contains(invalid));
    CHECK(storage.tryGet(reserved) == nullptr);
}

TEST_CASE("Component storage validates entity generations", "[unit][ecs][component-storage]") {
    ComponentStorage<Position> storage;

    constexpr EntityHandle current{42, 2};
    constexpr EntityHandle stale{42, 1};

    storage.emplace(current, 10, 20);

    CHECK(storage.contains(current));
    CHECK_FALSE(storage.contains(stale));

    CHECK(storage.tryGet(stale) == nullptr);

    CHECK_FALSE(storage.remove(stale));

    REQUIRE_THROWS_AS(storage.get(stale), std::out_of_range);

    REQUIRE_THROWS_AS(storage.emplace(stale, 99, 99), std::invalid_argument);

    CHECK(storage.size() == 1);

    CHECK(storage.get(current).x == 10);
    CHECK(storage.get(current).y == 20);
}

TEST_CASE("Component storage allows new generation after removal",
          "[unit][ecs][component-storage]") {
    ComponentStorage<Health> storage;

    constexpr EntityHandle original{5, 1};
    constexpr EntityHandle replacement{5, 2};

    storage.emplace(original, 100, 100);

    REQUIRE(storage.remove(original));

    storage.emplace(replacement, 200, 250);

    CHECK_FALSE(storage.contains(original));
    CHECK(storage.contains(replacement));

    CHECK(storage.tryGet(original) == nullptr);

    CHECK(storage.get(replacement).current == 200);
    CHECK(storage.get(replacement).maximum == 250);

    CHECK(storage.size() == 1);
}

TEST_CASE("Component storage supports mutable component access", "[unit][ecs][component-storage]") {
    ComponentStorage<Health> storage;

    constexpr EntityHandle entity{1, 1};

    storage.emplace(entity, 100, 150);

    storage.get(entity).current -= 25;

    auto *health = storage.tryGet(entity);

    REQUIRE(health != nullptr);

    health->maximum += 50;

    CHECK(storage.get(entity).current == 75);
    CHECK(storage.get(entity).maximum == 200);
}

TEST_CASE("Component storage supports const access", "[unit][ecs][component-storage]") {
    ComponentStorage<Position> storage;

    constexpr EntityHandle entity{1, 1};

    storage.emplace(entity, 32, 64);

    const auto &readOnly = std::as_const(storage);

    STATIC_REQUIRE(std::is_same_v<decltype(readOnly.get(entity)), const Position &>);

    STATIC_REQUIRE(std::is_same_v<decltype(readOnly.tryGet(entity)), const Position *>);

    STATIC_REQUIRE(std::is_same_v<decltype(readOnly.components()), std::span<const Position>>);

    REQUIRE(readOnly.tryGet(entity) != nullptr);

    CHECK(readOnly.get(entity).x == 32);
    CHECK(readOnly.get(entity).y == 64);

    CHECK(readOnly.components().size() == 1);
    CHECK(readOnly.entities().size() == 1);
}

TEST_CASE("Component storage rejects missing component access", "[unit][ecs][component-storage]") {
    ComponentStorage<Position> storage;

    constexpr EntityHandle missing{99, 1};

    CHECK_FALSE(storage.contains(missing));
    CHECK(storage.tryGet(missing) == nullptr);
    CHECK_FALSE(storage.remove(missing));

    REQUIRE_THROWS_AS(storage.get(missing), std::out_of_range);

    const auto &readOnly = std::as_const(storage);

    CHECK(readOnly.tryGet(missing) == nullptr);

    REQUIRE_THROWS_AS(readOnly.get(missing), std::out_of_range);
}

TEST_CASE("Component storage synchronizes swap and pop removal", "[unit][ecs][component-storage]") {
    ComponentStorage<Position> storage;

    constexpr EntityHandle first{1, 1};
    constexpr EntityHandle middle{4, 1};
    constexpr EntityHandle last{7, 1};

    storage.emplace(first, 10, 20);
    storage.emplace(middle, 30, 40);
    storage.emplace(last, 50, 60);

    REQUIRE(storage.remove(middle));

    REQUIRE(storage.size() == 2);

    const auto entities = storage.entities();
    const auto components = storage.components();

    REQUIRE(entities.size() == components.size());

    CHECK(entities[0] == first);
    CHECK(entities[1] == last);

    CHECK(components[0].x == 10);
    CHECK(components[0].y == 20);

    CHECK(components[1].x == 50);
    CHECK(components[1].y == 60);

    CHECK_FALSE(storage.contains(middle));

    CHECK(storage.get(last).x == 50);
    CHECK(storage.get(last).y == 60);
}

TEST_CASE("Component storage removes last and sole component", "[unit][ecs][component-storage]") {
    ComponentStorage<Health> storage;

    constexpr EntityHandle first{1, 1};
    constexpr EntityHandle last{2, 1};

    storage.emplace(first, 100, 100);
    storage.emplace(last, 200, 200);

    REQUIRE(storage.remove(last));

    REQUIRE(storage.size() == 1);

    CHECK(storage.contains(first));
    CHECK(storage.get(first).current == 100);

    CHECK_FALSE(storage.contains(last));
    CHECK_FALSE(storage.remove(last));

    REQUIRE(storage.remove(first));

    CHECK(storage.empty());
    CHECK(storage.entities().empty());
    CHECK(storage.components().empty());
}

TEST_CASE("Component storage supports dense iteration", "[unit][ecs][component-storage]") {
    ComponentStorage<Health> storage;

    storage.emplace({1, 1}, 10, 100);
    storage.emplace({4, 1}, 20, 100);
    storage.emplace({7, 1}, 30, 100);

    for (auto &component : storage.components()) {
        component.current += 5;
    }

    CHECK(storage.get({1, 1}).current == 15);
    CHECK(storage.get({4, 1}).current == 25);
    CHECK(storage.get({7, 1}).current == 35);

    const auto entities = storage.entities();
    const auto components = storage.components();

    REQUIRE(entities.size() == components.size());

    for (std::size_t index = 0; index < entities.size(); ++index) {
        CHECK(&storage.get(entities[index]) == &components[index]);
    }
}

TEST_CASE("Component storage supports move only components", "[unit][ecs][component-storage]") {
    ComponentStorage<MoveOnlyComponent> storage;

    constexpr EntityHandle first{1, 1};
    constexpr EntityHandle second{2, 1};

    storage.emplace(first, 100);
    storage.emplace(second, 200);

    REQUIRE(storage.get(first).value != nullptr);
    REQUIRE(storage.get(second).value != nullptr);

    CHECK(*storage.get(first).value == 100);
    CHECK(*storage.get(second).value == 200);

    storage.reserve(1024);

    CHECK(*storage.get(first).value == 100);
    CHECK(*storage.get(second).value == 200);

    REQUIRE(storage.remove(first));

    REQUIRE(storage.size() == 1);
    REQUIRE(storage.get(second).value != nullptr);

    CHECK(*storage.get(second).value == 200);

    CHECK_FALSE(storage.contains(first));
}

TEST_CASE("Component storage manages resource lifetimes", "[unit][ecs][component-storage]") {
    int liveResources = 0;

    {
        ComponentStorage<ResourceComponent> storage;

        storage.emplace({1, 1}, liveResources);
        storage.emplace({2, 1}, liveResources);
        storage.emplace({3, 1}, liveResources);

        CHECK(liveResources == 3);

        REQUIRE(storage.remove({2, 1}));

        CHECK(liveResources == 2);

        storage.reserve(1024);

        CHECK(liveResources == 2);

        storage.clear();

        CHECK(liveResources == 0);
        CHECK(storage.empty());

        storage.emplace({4, 1}, liveResources);

        CHECK(liveResources == 1);
    }

    CHECK(liveResources == 0);
}

TEST_CASE("Component storage rolls back failed construction", "[unit][ecs][component-storage]") {
    ComponentStorage<ThrowingComponent> storage;

    constexpr EntityHandle existing{1, 1};
    constexpr EntityHandle failing{2, 1};

    storage.emplace(existing, 100);

    REQUIRE_THROWS_AS(storage.emplace(failing, 200, true), std::runtime_error);

    CHECK(storage.size() == 1);

    CHECK_FALSE(storage.contains(failing));
    CHECK(storage.tryGet(failing) == nullptr);

    CHECK(storage.contains(existing));
    CHECK(storage.get(existing).value == 100);

    REQUIRE(storage.entities().size() == storage.components().size());

    storage.emplace(failing, 300, false);

    CHECK(storage.size() == 2);

    CHECK(storage.contains(failing));
    CHECK(storage.get(failing).value == 300);
}

TEST_CASE("Component storage reserve preserves data", "[unit][ecs][component-storage]") {
    ComponentStorage<Position> storage;

    constexpr EntityHandle first{1, 1};
    constexpr EntityHandle second{2, 1};

    storage.emplace(first, 10, 20);
    storage.emplace(second, 30, 40);

    storage.reserve(2048);

    REQUIRE(storage.size() == 2);

    CHECK(storage.get(first).x == 10);
    CHECK(storage.get(first).y == 20);

    CHECK(storage.get(second).x == 30);
    CHECK(storage.get(second).y == 40);
}

TEST_CASE("Component storage clear resets all membership", "[unit][ecs][component-storage]") {
    ComponentStorage<Position> storage;

    constexpr EntityHandle first{1, 1};
    constexpr EntityHandle second{2, 1};

    storage.emplace(first, 10, 20);
    storage.emplace(second, 30, 40);

    storage.clear();

    CHECK(storage.empty());

    CHECK(storage.entities().empty());
    CHECK(storage.components().empty());

    CHECK_FALSE(storage.contains(first));
    CHECK_FALSE(storage.contains(second));

    constexpr EntityHandle replacement{1, 2};

    storage.emplace(replacement, 50, 60);

    CHECK(storage.contains(replacement));
    CHECK_FALSE(storage.contains(first));

    CHECK(storage.get(replacement).x == 50);

    storage.clear();
    storage.clear();

    CHECK(storage.empty());
}

TEST_CASE("Component storage enforces component type requirements",
          "[unit][ecs][component-storage]") {
    STATIC_REQUIRE(ComponentType<Position>);
    STATIC_REQUIRE(ComponentType<Health>);

    STATIC_REQUIRE(ComponentType<MoveOnlyComponent>);
    STATIC_REQUIRE(ComponentType<ResourceComponent>);
    STATIC_REQUIRE(ComponentType<ThrowingComponent>);

    STATIC_REQUIRE_FALSE(ComponentType<bool>);
    STATIC_REQUIRE_FALSE(ComponentType<const Position>);
    STATIC_REQUIRE_FALSE(ComponentType<Position &>);
    STATIC_REQUIRE_FALSE(ComponentType<Position[2]>);

    STATIC_REQUIRE_FALSE(ComponentType<ThrowingMoveComponent>);

    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<ComponentStorage<Position>>);

    STATIC_REQUIRE_FALSE(std::is_move_constructible_v<ComponentStorage<Position>>);
}

TEST_CASE("Component storage maintains bulk data invariants",
          "[unit][ecs][component-storage][stress]") {
    ComponentStorage<Position> storage;

    constexpr u32 entityCount = 10'000;

    storage.reserve(entityCount);

    for (u32 i = 0; i < entityCount; ++i) {
        const auto value = static_cast<int>(i);

        storage.emplace(EntityHandle{i, 1}, value, -value);
    }

    REQUIRE(storage.size() == entityCount);

    for (u32 i = 0; i < entityCount; i += 2) {
        REQUIRE(storage.remove({i, 1}));
    }

    REQUIRE(storage.size() == entityCount / 2);

    for (u32 i = 0; i < entityCount; ++i) {
        const auto *position = storage.tryGet({i, 1});

        if (i % 2 == 0) {
            CHECK(position == nullptr);
        } else {
            REQUIRE(position != nullptr);

            CHECK(position->x == static_cast<int>(i));

            CHECK(position->y == -static_cast<int>(i));
        }
    }

    for (u32 i = 0; i < entityCount; i += 2) {
        const auto value = static_cast<int>(i) + 100'000;

        storage.emplace(EntityHandle{i, 2}, value, -value);
    }

    REQUIRE(storage.size() == entityCount);

    for (u32 i = 0; i < entityCount; i += 2) {
        CHECK(storage.tryGet({i, 1}) == nullptr);

        CHECK_FALSE(storage.remove({i, 1}));

        CHECK(storage.contains({i, 2}));
    }

    const auto entities = storage.entities();
    const auto components = storage.components();

    REQUIRE(entities.size() == components.size());

    for (std::size_t index = 0; index < entities.size(); ++index) {
        const auto entity = entities[index];

        const auto &position = components[index];

        int expected = static_cast<int>(entity.index);

        if (entity.generation == 2) {
            expected += 100'000;
        }

        CHECK(position.x == expected);
        CHECK(position.y == -expected);

        CHECK(storage.tryGet(entity) == &components[index]);
    }

    storage.clear();

    CHECK(storage.empty());
    CHECK(storage.entities().empty());
    CHECK(storage.components().empty());
}
