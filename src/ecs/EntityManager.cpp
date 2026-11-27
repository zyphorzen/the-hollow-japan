#include "ecs/EntityManager.hpp"
#include <stdexcept>

namespace thj::ecs {

EntityHandle EntityManager::create() {
    if (freeHead_ != kInvalidIndex) {
        const u32 index = freeHead_;

        auto &slot = slots_[index];

        freeHead_ = slot.nextFree;

        slot.nextFree = kInvalidIndex;
        slot.alive = true;

        ++aliveCount_;

        return EntityHandle{index, slot.generation};
    }

    if (slots_.size() >= kInvalidIndex) {
        throw std::length_error("Entity index capacity exhausted");
    }

    const auto index = static_cast<u32>(slots_.size());

    slots_.push_back(EntitySlot{1, kInvalidIndex, true});

    ++aliveCount_;

    return EntityHandle{index, 1};
}

bool EntityManager::destroy(EntityHandle entity) noexcept {
    if (!isAlive(entity)) {
        return false;
    }

    auto &slot = slots_[entity.index];

    slot.alive = false;

    --aliveCount_;

    if (slot.generation == std::numeric_limits<u32>::max()) {
        slot.nextFree = kInvalidIndex;

        return true;
    }

    ++slot.generation;

    slot.nextFree = freeHead_;

    freeHead_ = entity.index;

    return true;
}

bool EntityManager::isAlive(EntityHandle entity) const noexcept {
    if (!entity.valid()) {
        return false;
    }

    if (entity.index >= slots_.size()) {
        return false;
    }

    const auto &slot = slots_[entity.index];

    return slot.alive && slot.generation == entity.generation;
}

std::size_t EntityManager::aliveCount() const noexcept { return aliveCount_; }

std::size_t EntityManager::slotCount() const noexcept { return slots_.size(); }

void EntityManager::reserve(std::size_t capacity) {
    if (capacity > kInvalidIndex) {
        throw std::length_error("Entity reserve capacity exceeds index range");
    }

    slots_.reserve(capacity);
}

} // namespace thj::ecs