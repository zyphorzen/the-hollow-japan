#include "simulation/systems/MovementSystem.hpp"

#include "core/time/SimulationTime.hpp"
#include "simulation/Components.hpp"

#include <stdexcept>

namespace thj::simulation {

namespace {

struct AxisStep {
    core::FixedPoint position{};
    core::i64 remainder{};
};

[[nodiscard]] AxisStep integrateAxis(core::FixedPoint position, core::FixedPoint velocity,
                                     core::i64 remainder) {
    constexpr auto hz = static_cast<core::i64>(core::kSimulationHz);

    static_assert(hz > 0);

    if (remainder <= -hz || remainder >= hz) {
        throw std::logic_error("Invalid movement integration remainder");
    }

    const auto rawVelocity = velocity.raw();

    auto displacement = rawVelocity / hz;

    const auto fractional = rawVelocity % hz + remainder;

    displacement += fractional / hz;

    auto nextRemainder = fractional % hz;

    if (displacement > 0 && nextRemainder < 0) {
        --displacement;
        nextRemainder += hz;
    } else if (displacement < 0 && nextRemainder > 0) {
        ++displacement;
        nextRemainder -= hz;
    }

    const auto nextPosition = position.checkedAdd(core::FixedPoint::fromRaw(displacement));

    if (!nextPosition.has_value()) {
        throw std::overflow_error("Simulation position overflow");
    }

    return AxisStep{*nextPosition, nextRemainder};
}

} // namespace

void MovementSystem::update(ecs::Registry &registry) {

    registry.each<TransformComponent>([](core::EntityHandle, TransformComponent &transform) {
        transform.previousPosition = transform.position;
    });

    registry.each<TransformComponent, VelocityComponent>(
        [](core::EntityHandle, TransformComponent &transform, VelocityComponent &velocity) {
            const auto nextX = integrateAxis(transform.position.x, velocity.x, velocity.remainderX);
            const auto nextY = integrateAxis(transform.position.y, velocity.y, velocity.remainderY);

            transform.position = core::Position2{nextX.position, nextY.position};

            velocity.remainderX = nextX.remainder;
            velocity.remainderY = nextY.remainder;
        });
}

} // namespace thj::simulation