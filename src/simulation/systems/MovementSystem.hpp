#pragma once

#include "ecs/Registry.hpp"

namespace thj::simulation {

class MovementSystem final {
  public:
    static void update(ecs::Registry &registry);
};

} // namespace thj::simulation