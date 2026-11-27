#pragma once

#include "core/Types.hpp"

#include <chrono>
#include <limits>
#include <optional>

namespace thj::core {

using MonotonicClock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;

inline constexpr u32 kSimulationHz = 60;

struct SimulationTick {
    u64 value{};

    [[nodiscard]] constexpr std::optional<SimulationTick> next() const noexcept {
        if (value == std::numeric_limits<u64>::max()) {
            return std::nullopt;
        }

        return SimulationTick{value + 1};
    }

    constexpr bool operator==(const SimulationTick &) const noexcept = default;
};

static_assert(MonotonicClock::is_steady);

} // namespace thj::core
