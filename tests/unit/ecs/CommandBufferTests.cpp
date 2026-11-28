
#include "ecs/CommandBuffer.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using thj::core::EntityHandle;
using thj::core::u32;

using thj::ecs::CommandBuffer;
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

struct Name {
    std::string value;
};

struct EnemyTag {};

struct MoveOnlyComponent {
    std::unique_ptr<int> value;

    explicit MoveOnlyComponent(std::unique_ptr<int> resource) noexcept
        : value(std::move(resource)) {}

    MoveOnlyComponent(MoveOnlyComponent &&) noexcept = default;

    MoveOnlyComponent &operator=(MoveOnlyComponent &&) noexcept = default;
};

struct ThrowingComponent {
    int value{};

    explicit ThrowingComponent(bool shouldThrow) {
        if (shouldThrow) {
            throw std::runtime_error("Deferred component construction failed");
        }
    }
};

struct ReentrantComponent {
    explicit ReentrantComponent(CommandBuffer *commands) { commands->clear(); }
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

    explicit ResourceComponent(std::unique_ptr<TrackedResource> value) noexcept
        : resource(std::move(value)) {}

    ResourceComponent(ResourceComponent &&) noexcept = default;

    ResourceComponent &operator=(ResourceComponent &&) noexcept = default;
};

} // namespace

TEST_CASE("Command buffer starts empty", "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    CHECK(commands.empty());
    CHECK(commands.size() == 0);
    CHECK_FALSE(commands.isPlaying());

    const auto result = commands.playback();

    CHECK(result.success());

    CHECK(result.applied == 0);
    CHECK(result.skipped == 0);
    CHECK(result.failed == 0);
    CHECK(result.discarded == 0);
    CHECK(result.total() == 0);

    CHECK(registry.aliveCount() == 0);
    CHECK(commands.empty());
}

TEST_CASE("Command buffer defers entity creation", "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto first = commands.create();
    const auto second = commands.create();

    CHECK(commands.size() == 2);
    CHECK(registry.aliveCount() == 0);

    const auto result = commands.playback();

    REQUIRE(result.success());
    CHECK(result.applied == 2);

    const auto firstEntity = result.resolve(first);
    const auto secondEntity = result.resolve(second);

    REQUIRE(firstEntity.has_value());
    REQUIRE(secondEntity.has_value());

    CHECK(*firstEntity != *secondEntity);

    CHECK(registry.isAlive(*firstEntity));
    CHECK(registry.isAlive(*secondEntity));

    CHECK(registry.aliveCount() == 2);
    CHECK(commands.empty());
}

TEST_CASE("Command buffer attaches components to deferred entities",
          "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto pending = commands.create();

    commands.emplace<Position>(pending, 100, 200);

    commands.emplace<Velocity>(pending, 2, 3);

    commands.emplace<EnemyTag>(pending);

    CHECK(registry.aliveCount() == 0);
    CHECK(registry.poolCount() == 0);
    CHECK(commands.size() == 4);

    const auto result = commands.playback();

    REQUIRE(result.success());
    REQUIRE(result.applied == 4);

    const auto entity = result.resolve(pending);

    REQUIRE(entity.has_value());
    REQUIRE(registry.isAlive(*entity));

    CHECK(registry.get<Position>(*entity).x == 100);
    CHECK(registry.get<Position>(*entity).y == 200);

    CHECK(registry.get<Velocity>(*entity).x == 2);
    CHECK(registry.get<Velocity>(*entity).y == 3);

    CHECK(registry.has<EnemyTag>(*entity));
}

TEST_CASE("Command buffer defers changes on existing entities", "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto entity = registry.create();

    registry.emplace<Velocity>(entity, 1, 2);

    commands.remove<Velocity>(entity);
    commands.emplace<Health>(entity, 100, 150);

    CHECK(registry.has<Velocity>(entity));
    CHECK_FALSE(registry.has<Health>(entity));

    const auto result = commands.playback();

    REQUIRE(result.success());

    CHECK(result.applied == 2);
    CHECK_FALSE(registry.has<Velocity>(entity));
    CHECK(registry.has<Health>(entity));

    CHECK(registry.get<Health>(entity).current == 100);
    CHECK(registry.get<Health>(entity).maximum == 150);

    commands.destroy(entity);

    CHECK(registry.isAlive(entity));

    const auto destruction = commands.playback();

    CHECK(destruction.applied == 1);
    CHECK_FALSE(registry.isAlive(entity));
    CHECK(registry.componentCount<Health>() == 0);
}

TEST_CASE("Command buffer executes structural commands in FIFO order",
          "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto entity = registry.create();

    commands.emplace<Position>(entity, 10, 20);
    commands.remove<Position>(entity);
    commands.emplace<Position>(entity, 30, 40);

    const auto result = commands.playback();

    REQUIRE(result.success());

    CHECK(result.applied == 3);
    CHECK(result.skipped == 0);

    CHECK(registry.componentCount<Position>() == 1);

    CHECK(registry.get<Position>(entity).x == 30);
    CHECK(registry.get<Position>(entity).y == 40);
}

TEST_CASE("Command buffer destroys newly created entities", "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto pending = commands.create();

    commands.emplace<Position>(pending, 10, 20);

    commands.destroy(pending);

    commands.emplace<Health>(pending, 100, 100);

    const auto result = commands.playback();

    REQUIRE(result.success());

    CHECK(result.applied == 3);
    CHECK(result.skipped == 1);

    const auto entity = result.resolve(pending);

    REQUIRE(entity.has_value());

    CHECK_FALSE(registry.isAlive(*entity));

    CHECK(registry.aliveCount() == 0);
    CHECK(registry.componentCount<Position>() == 0);
    CHECK(registry.componentCount<Health>() == 0);
}

TEST_CASE("Command buffer skips stale generational handles", "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto original = registry.create();

    registry.emplace<Health>(original, 100, 100);

    commands.destroy(original);

    const auto pendingReplacement = commands.create();

    commands.emplace<Position>(pendingReplacement, 50, 60);

    commands.emplace<Health>(original, 1, 1);
    commands.remove<Position>(original);
    commands.destroy(original);

    const auto result = commands.playback();

    REQUIRE(result.success());

    CHECK(result.applied == 3);
    CHECK(result.skipped == 3);

    const auto replacement = result.resolve(pendingReplacement);

    REQUIRE(replacement.has_value());

    CHECK(replacement->index == original.index);

    CHECK(replacement->generation == original.generation + 1);

    CHECK(registry.isAlive(*replacement));
    CHECK_FALSE(registry.isAlive(original));

    CHECK(registry.has<Position>(*replacement));
    CHECK_FALSE(registry.has<Health>(*replacement));

    CHECK(registry.get<Position>(*replacement).x == 50);
}

TEST_CASE("Command buffer skips duplicate component insertion", "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto entity = registry.create();

    commands.emplace<Health>(entity, 100, 150);
    commands.emplace<Health>(entity, 1, 1);

    const auto result = commands.playback();

    REQUIRE(result.success());

    CHECK(result.applied == 1);
    CHECK(result.skipped == 1);

    CHECK(registry.componentCount<Health>() == 1);

    CHECK(registry.get<Health>(entity).current == 100);
    CHECK(registry.get<Health>(entity).maximum == 150);
}

TEST_CASE("Command buffer skips absent components and invalid entities",
          "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto entity = registry.create();

    commands.remove<Velocity>(entity);

    commands.destroy(EntityHandle{});

    commands.emplace<Position>(EntityHandle{999, 1}, 10, 20);

    commands.remove<Health>(EntityHandle{999, 1});

    const auto result = commands.playback();

    CHECK(result.success());

    CHECK(result.applied == 0);
    CHECK(result.skipped == 4);
    CHECK(result.failed == 0);

    CHECK(registry.aliveCount() == 1);
    CHECK(registry.poolCount() == 0);
}

TEST_CASE("Command buffer records safely during ECS queries", "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto first = registry.create();
    const auto second = registry.create();

    registry.emplace<Health>(first, 0, 100);
    registry.emplace<Health>(second, 50, 100);

    const auto visited = registry.eachOrdered<Health>([&](EntityHandle entity, Health &health) {
        if (health.current == 0) {
            commands.destroy(entity);

            const auto effect = commands.create();

            commands.emplace<Position>(effect, 100, 200);
        }

        CHECK(registry.aliveCount() == 2);
    });

    CHECK(visited == 2);
    CHECK(commands.size() == 3);

    CHECK(registry.isAlive(first));
    CHECK(registry.isAlive(second));

    const auto result = commands.playback();

    REQUIRE(result.success());

    CHECK(result.applied == 3);
    CHECK(registry.aliveCount() == 2);

    CHECK_FALSE(registry.isAlive(first));
    CHECK(registry.isAlive(second));

    CHECK(registry.componentCount<Health>() == 1);
    CHECK(registry.componentCount<Position>() == 1);
}

TEST_CASE("Command buffer rejects playback during active queries", "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto entity = registry.create();

    registry.emplace<Position>(entity, 10, 20);

    registry.each<Position>([&](EntityHandle current, Position &position) {
        commands.destroy(current);

        REQUIRE_THROWS_AS(commands.playback(), std::logic_error);

        CHECK(commands.size() == 1);
        CHECK_FALSE(commands.isPlaying());

        CHECK(registry.isAlive(current));
        CHECK(position.x == 10);
    });

    const auto result = commands.playback();

    CHECK(result.success());
    CHECK(result.applied == 1);

    CHECK_FALSE(registry.isAlive(entity));
    CHECK(commands.empty());
}

TEST_CASE("Command buffer rejects foreign and expired deferred tokens",
          "[unit][ecs][command-buffer]") {
    Registry registry;

    CommandBuffer firstBuffer{registry};
    CommandBuffer secondBuffer{registry};

    const auto first = firstBuffer.create();
    const auto second = secondBuffer.create();

    REQUIRE_THROWS_AS(secondBuffer.destroy(first), std::invalid_argument);

    REQUIRE_THROWS_AS(firstBuffer.emplace<Position>(second, 10, 20), std::invalid_argument);

    CHECK(firstBuffer.size() == 1);
    CHECK(secondBuffer.size() == 1);

    const auto result = firstBuffer.playback();

    REQUIRE(result.success());

    CHECK(result.resolve(first).has_value());
    CHECK_FALSE(result.resolve(second).has_value());

    REQUIRE_THROWS_AS(firstBuffer.remove<Position>(first), std::invalid_argument);

    const auto newBatch = firstBuffer.create();

    REQUIRE_THROWS_AS(firstBuffer.destroy(first), std::invalid_argument);

    const auto newResult = firstBuffer.playback();

    REQUIRE(newResult.success());

    CHECK(newResult.resolve(newBatch).has_value());
    CHECK_FALSE(newResult.resolve(first).has_value());

    CHECK_FALSE(result.resolve(newBatch).has_value());

    const auto secondResult = secondBuffer.playback();

    CHECK(secondResult.resolve(second).has_value());
}

TEST_CASE("Command buffer owns copied argument payloads", "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto entity = registry.create();

    std::string source = "Hollow Stag";

    commands.emplace<Name>(entity, source);

    source = "Changed after recording";

    const auto result = commands.playback();

    REQUIRE(result.success());

    CHECK(registry.get<Name>(entity).value == "Hollow Stag");
}

TEST_CASE("Command buffer supports move only payloads", "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto entity = registry.create();

    auto resource = std::make_unique<int>(42);

    commands.emplace<MoveOnlyComponent>(entity, std::move(resource));

    CHECK(resource == nullptr);

    CHECK_FALSE(registry.has<MoveOnlyComponent>(entity));

    commands.reserve(1024);

    const auto result = commands.playback();

    REQUIRE(result.success());
    CHECK(result.applied == 1);

    const auto &component = registry.get<MoveOnlyComponent>(entity);

    REQUIRE(component.value != nullptr);

    CHECK(*component.value == 42);
}

TEST_CASE("Command buffer clear releases payloads and invalidates tokens",
          "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    int liveResources = 0;

    const auto pending = commands.create();

    commands.emplace<ResourceComponent>(pending, std::make_unique<TrackedResource>(liveResources));

    CHECK(liveResources == 1);
    CHECK(commands.size() == 2);

    commands.clear();

    CHECK(commands.empty());
    CHECK(liveResources == 0);
    CHECK(registry.aliveCount() == 0);

    REQUIRE_THROWS_AS(commands.destroy(pending), std::invalid_argument);

    const auto next = commands.create();

    commands.emplace<ResourceComponent>(next, std::make_unique<TrackedResource>(liveResources));

    const auto result = commands.playback();

    REQUIRE(result.success());

    CHECK_FALSE(result.resolve(pending).has_value());

    const auto entity = result.resolve(next);

    REQUIRE(entity.has_value());

    CHECK(liveResources == 1);

    REQUIRE(registry.destroy(*entity));

    CHECK(liveResources == 0);

    commands.clear();
    commands.clear();

    CHECK(commands.empty());
}

TEST_CASE("Command buffer stops playback after command failure", "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto entity = registry.create();

    int liveResources = 0;

    commands.emplace<Position>(entity, 10, 20);

    commands.emplace<ThrowingComponent>(entity, true);

    const auto discardedCreation = commands.create();

    commands.emplace<ResourceComponent>(discardedCreation,
                                        std::make_unique<TrackedResource>(liveResources));

    REQUIRE(liveResources == 1);

    const auto result = commands.playback();

    CHECK_FALSE(result.success());

    CHECK(result.applied == 1);
    CHECK(result.skipped == 0);
    CHECK(result.failed == 1);
    CHECK(result.discarded == 2);
    CHECK(result.total() == 4);

    REQUIRE(result.error != nullptr);

    REQUIRE_THROWS_AS(result.rethrowIfFailed(), std::runtime_error);

    CHECK(registry.get<Position>(entity).x == 10);

    CHECK_FALSE(registry.has<ThrowingComponent>(entity));

    CHECK_FALSE(result.resolve(discardedCreation).has_value());

    CHECK(registry.aliveCount() == 1);
    CHECK(liveResources == 0);

    CHECK(commands.empty());
    CHECK_FALSE(commands.isPlaying());

    commands.emplace<Health>(entity, 100, 100);

    const auto recovery = commands.playback();

    CHECK(recovery.success());
    CHECK(recovery.applied == 1);

    CHECK(registry.has<Health>(entity));
}

TEST_CASE("Command buffer rejects reentrant mutation during playback",
          "[unit][ecs][command-buffer]") {
    Registry registry;
    CommandBuffer commands{registry};

    const auto entity = registry.create();

    commands.emplace<ReentrantComponent>(entity, &commands);

    const auto result = commands.playback();

    CHECK_FALSE(result.success());

    CHECK(result.failed == 1);
    CHECK(result.applied == 0);

    REQUIRE_THROWS_AS(result.rethrowIfFailed(), std::logic_error);

    CHECK_FALSE(registry.has<ReentrantComponent>(entity));

    CHECK_FALSE(commands.isPlaying());
    CHECK(commands.empty());

    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<CommandBuffer>);

    STATIC_REQUIRE_FALSE(std::is_move_constructible_v<CommandBuffer>);
}

TEST_CASE("Command buffer maintains bulk structural invariants",
          "[unit][ecs][command-buffer][stress]") {
    Registry registry;
    CommandBuffer commands{registry};

    constexpr u32 entityCount = 10'000;

    registry.reserveEntities(entityCount);

    registry.reserveComponents<Position>(entityCount);

    registry.reserveComponents<Health>(entityCount);

    commands.reserve(entityCount * 3);

    std::vector<CommandBuffer::DeferredEntity> tokens;

    tokens.reserve(entityCount);

    for (u32 i = 0; i < entityCount; ++i) {
        const auto pending = commands.create();

        tokens.push_back(pending);

        commands.emplace<Position>(pending, static_cast<int>(i), 0);

        commands.emplace<Health>(pending, i % 2 == 0 ? 0 : 100, 100);
    }

    CHECK(registry.aliveCount() == 0);
    CHECK(commands.size() == entityCount * 3);

    const auto creation = commands.playback();

    REQUIRE(creation.success());

    CHECK(creation.applied == entityCount * 3);

    REQUIRE(registry.aliveCount() == entityCount);

    CHECK(registry.componentCount<Position>() == entityCount);

    CHECK(registry.componentCount<Health>() == entityCount);

    std::vector<EntityHandle> originalEntities;

    originalEntities.reserve(entityCount);

    for (const auto &token : tokens) {
        const auto entity = creation.resolve(token);

        REQUIRE(entity.has_value());

        originalEntities.push_back(*entity);
    }

    const auto visited = registry.eachOrdered<Health>([&](EntityHandle entity, Health &health) {
        if (health.current == 0) {
            commands.destroy(entity);
        }
    });

    CHECK(visited == entityCount);
    CHECK(commands.size() == entityCount / 2);

    CHECK(registry.aliveCount() == entityCount);

    const auto destruction = commands.playback();

    REQUIRE(destruction.success());

    CHECK(destruction.applied == entityCount / 2);

    CHECK(registry.aliveCount() == entityCount / 2);

    for (u32 i = 0; i < entityCount; ++i) {
        const auto entity = originalEntities[i];

        if (i % 2 == 0) {
            CHECK_FALSE(registry.isAlive(entity));

            commands.destroy(entity);
        } else {
            REQUIRE(registry.isAlive(entity));

            CHECK(registry.get<Position>(entity).x == static_cast<int>(i));

            CHECK(registry.get<Health>(entity).current == 100);
        }
    }

    std::vector<CommandBuffer::DeferredEntity> replacementTokens;

    replacementTokens.reserve(entityCount / 2);

    for (u32 i = 0; i < entityCount / 2; ++i) {
        const auto pending = commands.create();

        replacementTokens.push_back(pending);

        commands.emplace<Health>(pending, 200, 250);
    }

    for (u32 i = 0; i < entityCount; i += 2) {
        commands.remove<Health>(originalEntities[i]);
    }

    const auto replacement = commands.playback();

    REQUIRE(replacement.success());

    CHECK(replacement.applied == entityCount);
    CHECK(replacement.skipped == entityCount);

    CHECK(registry.aliveCount() == entityCount);
    CHECK(registry.slotCount() == entityCount);

    for (const auto &token : replacementTokens) {
        const auto entity = replacement.resolve(token);

        REQUIRE(entity.has_value());

        CHECK(entity->generation == 2);
        CHECK(entity->index % 2 == 0);

        CHECK(registry.isAlive(*entity));

        CHECK_FALSE(registry.has<Position>(*entity));

        CHECK(registry.get<Health>(*entity).current == 200);
    }

    CHECK(registry.componentCount<Position>() == entityCount / 2);

    CHECK(registry.componentCount<Health>() == entityCount);

    CHECK(commands.empty());
    CHECK_FALSE(commands.isPlaying());
}
