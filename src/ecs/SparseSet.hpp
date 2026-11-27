#pragma once

#include "core/Types.hpp"

#include <cstddef>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace thj::ecs {

using core::EntityHandle;

class SparseSet final {
  public:
    using DenseIndex = std::size_t;

    SparseSet() = default;
    ~SparseSet() = default;

    SparseSet(const SparseSet &) = delete;
    SparseSet &operator=(const SparseSet &) = delete;

    SparseSet(SparseSet &&) = delete;
    SparseSet &operator=(SparseSet &&) = delete;

    bool insert(EntityHandle entity);

    bool remove(EntityHandle entity) noexcept;

    [[nodiscard]] bool contains(EntityHandle entity) const noexcept;

    [[nodiscard]] std::optional<DenseIndex> indexOf(EntityHandle entity) const noexcept;

    [[nodiscard]] EntityHandle entityAt(DenseIndex index) const;

    [[nodiscard]] std::span<const EntityHandle> entities() const noexcept;

    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] bool empty() const noexcept;

    void reserve(std::size_t capacity);

    void clear() noexcept;

  private:
    static constexpr DenseIndex kInvalidDenseIndex = std::numeric_limits<DenseIndex>::max();

    std::vector<DenseIndex> sparse_;

    std::vector<EntityHandle> dense_;
};

} // namespace thj::ecs