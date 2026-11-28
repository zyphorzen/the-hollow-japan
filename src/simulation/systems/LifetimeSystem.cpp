#include "simulation/systems/LifetimeSystem.hpp"
#include "simulation/Components.hpp"

namespace thj::simulation {

void LifetimeSystem::update(ecs::Registry &registry, ecs::CommandBuffer &commands) {
    registry.eachOrdered<LifetimeComponent>(
        [&](core::EntityHandle entity, LifetimeComponent &lifetime) {
            if (lifetime.remainingTicks > 0) {
                --lifetime.remainingTicks;
            }

            if (lifetime.remainingTicks == 0) {
                commands.destroy(entity);
            }
        });
}

} // namespace thj::simulation