#include "ecs/SparseSet.hpp"

namespace thj::ecs {

bool SparseSet::insert(EntityHandle entity) {
    if (!entity.valid()) {
        return false;
    }

    if (entity.index == std::numeric_limits<core::u32>::max()) {
        return false;
    }

    const auto sparseIndex = static_cast<std::size_t>(entity.index);

    if (sparseIndex >= sparse_.size()) {
        sparse_.resize(sparseIndex + 1, kInvalidDenseIndex);
    }

    if (sparse_[sparseIndex] != kInvalidDenseIndex) {
        return false;
    }

    const auto denseIndex = dense_.size();

    dense_.push_back(entity);

    sparse_[sparseIndex] = denseIndex;

    return true;
}

bool SparseSet::remove(EntityHandle entity) noexcept {
    const auto foundIndex = indexOf(entity);

    if (!foundIndex.has_value()) {
        return false;
    }

    const auto denseIndex = *foundIndex;

    const auto lastIndex = dense_.size() - 1;

    if (denseIndex != lastIndex) {
        const auto movedEntity = dense_[lastIndex];

        dense_[denseIndex] = movedEntity;

        sparse_[movedEntity.index] = denseIndex;
    }

    dense_.pop_back();

    sparse_[entity.index] = kInvalidDenseIndex;

    return true;
}

bool SparseSet::contains(EntityHandle entity) const noexcept { return indexOf(entity).has_value(); }

std::optional<SparseSet::DenseIndex> SparseSet::indexOf(EntityHandle entity) const noexcept {
    if (!entity.valid()) {
        return std::nullopt;
    }

    const auto sparseIndex = static_cast<std::size_t>(entity.index);

    if (sparseIndex >= sparse_.size()) {
        return std::nullopt;
    }

    const auto denseIndex = sparse_[sparseIndex];

    if (denseIndex == kInvalidDenseIndex || denseIndex >= dense_.size()) {
        return std::nullopt;
    }

    if (dense_[denseIndex] != entity) {
        return std::nullopt;
    }

    return denseIndex;
}

EntityHandle SparseSet::entityAt(DenseIndex index) const { return dense_.at(index); }

std::span<const EntityHandle> SparseSet::entities() const noexcept {
    return std::span<const EntityHandle>{dense_};
}

std::size_t SparseSet::size() const noexcept { return dense_.size(); }

bool SparseSet::empty() const noexcept { return dense_.empty(); }

void SparseSet::reserve(std::size_t capacity) { dense_.reserve(capacity); }

void SparseSet::clear() noexcept {
    dense_.clear();
    sparse_.clear();
}

} // namespace thj::ecs