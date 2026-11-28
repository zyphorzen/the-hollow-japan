#pragma once

#include "ecs/CommandBuffer.hpp"
#include "ecs/Registry.hpp"

namespace thj::simulation {

class LifetimeSystem final {
  public:
    static void update(ecs::Registry &registry, ecs::CommandBuffer &commands);
};

} // namespace thj::simulation