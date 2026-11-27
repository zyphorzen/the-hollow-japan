#include "ecs/Registry.hpp"

namespace thj::ecs {

EntityHandle Registry::create() {
    requireStructuralMutationAllowed();

    return entityManager_.create();
}

bool Registry::destroy(EntityHandle entity) {
    requireStructuralMutationAllowed();

    if (!entityManager_.isAlive(entity)) {
        return false;
    }

    for (auto &pool : pools_) {
        pool->remove(entity);
    }

    return entityManager_.destroy(entity);
}

bool Registry::isAlive(EntityHandle entity) const noexcept {
    return entityManager_.isAlive(entity);
}

std::size_t Registry::aliveCount() const noexcept { return entityManager_.aliveCount(); }

std::size_t Registry::slotCount() const noexcept { return entityManager_.slotCount(); }

void Registry::reserveEntities(std::size_t capacity) {
    requireStructuralMutationAllowed();

    entityManager_.reserve(capacity);
}

std::size_t Registry::poolCount() const noexcept { return pools_.size(); }

bool Registry::isIterating() const noexcept { return iterationDepth_ != 0; }

void Registry::requireAlive(EntityHandle entity) const {
    if (!entityManager_.isAlive(entity)) {
        throw std::invalid_argument("Entity is not alive");
    }
}

void Registry::requireStructuralMutationAllowed() const {
    if (isIterating()) {
        throw std::logic_error("Structural mutation is not allowed during ECS iteration");
    }
}

} // namespace thj::ecs