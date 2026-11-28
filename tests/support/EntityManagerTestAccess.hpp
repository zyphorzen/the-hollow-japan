#pragma once

#include "ecs/EntityManager.hpp"

#include <stdexcept>

namespace thj::ecs::testing {

struct EntityManagerTestAccess final {
    [[nodiscard]] static EntityHandle setGeneration(EntityManager &manager, EntityHandle entity,
                                                    core::u32 generation) {
        if (!manager.isAlive(entity)) {
            throw std::invalid_argument("Test accessor requires a live entity");
        }

        if (generation == 0) {
            throw std::invalid_argument("Generation zero is invalid");
        }

        manager.slots_[entity.index].generation = generation;

        return EntityHandle{entity.index, generation};
    }
};

} // namespace thj::ecs::testing