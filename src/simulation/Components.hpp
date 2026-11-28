#pragma once

#include "core/Types.hpp"
#include "core/world/Coordinates.hpp"

namespace thj::simulation {

struct TransformComponent {
    core::Position2 position{};
    core::Position2 previousPosition{};
};

struct VelocityComponent {
    core::FixedPoint x{};
    core::FixedPoint y{};

    core::i64 remainderX{};
    core::i64 remainderY{};
};

struct LifetimeComponent {
    core::u64 remainingTicks{};
};

} // namespace thj::simulation