#pragma once

#include "core/Types.hpp"

#include <cstddef>
#include <limits>
#include <vector>

namespace thj::ecs {

using core::EntityHandle;
using core::u32;

class EntityManager final {
  public:
    EntityManager() = default;
    ~EntityManager() = default;

    EntityManager(const EntityManager &) = delete;
    EntityManager &operator=(const EntityManager &) = delete;

    EntityManager(EntityManager &&) = delete;
    EntityManager &operator=(EntityManager &&) = delete;

    [[nodiscard]] EntityHandle create();

    bool destroy(EntityHandle entity) noexcept;

    [[nodiscard]] bool isAlive(EntityHandle entity) const noexcept;

    [[nodiscard]] std::size_t aliveCount() const noexcept;

    [[nodiscard]] std::size_t slotCount() const noexcept;

    void reserve(std::size_t capacity);

  private:
    static constexpr u32 kInvalidIndex = std::numeric_limits<u32>::max();

    struct EntitySlot {
        u32 generation{1};

        u32 nextFree{kInvalidIndex};

        bool alive{false};
    };

    std::vector<EntitySlot> slots_;

    u32 freeHead_{kInvalidIndex};

    std::size_t aliveCount_{};
};

} // namespace thj::ecs