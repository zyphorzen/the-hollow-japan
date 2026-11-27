#pragma once

#include "core/time/SimulationTime.hpp"

namespace thj::core {

struct FrameTiming {
    Nanoseconds rawDelta{};
    Nanoseconds clampedDelta{};

    u32 executedTicks{};
    u32 droppedTicks{};

    double interpolationAlpha{};

    bool paused{};
    bool deltaClamped{};
};

class FixedTimestep final {
  public:
    static constexpr i64 kTickThreshold = 1'000'000'000;

    static constexpr Nanoseconds kMaxFrameDelta{250'000'000};

    static constexpr u32 kMaxCatchUpTicks = 5;

    FixedTimestep() noexcept = default;

    void beginFrame(Nanoseconds elapsed);

    [[nodiscard]] bool hasPendingTick() const noexcept;

    [[nodiscard]] SimulationTick nextTick() const;

    void consumeTick();

    [[nodiscard]] FrameTiming endFrame();

    void setPaused(bool paused) noexcept;

    [[nodiscard]] bool isPaused() const noexcept;

    [[nodiscard]] SimulationTick currentTick() const noexcept;

    [[nodiscard]] double interpolationAlpha() const noexcept;

  private:
    SimulationTick currentTick_{};

    i64 accumulatorUnits_{};

    FrameTiming frame_{};

    bool paused_{false};
    bool frameOpen_{false};
};

} // namespace thj::core