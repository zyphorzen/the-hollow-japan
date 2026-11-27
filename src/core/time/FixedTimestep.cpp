#include "core/time/FixedTimestep.hpp"

#include <algorithm>
#include <stdexcept>

namespace thj::core {

void FixedTimestep::beginFrame(Nanoseconds elapsed) {
    if (frameOpen_) {
        throw std::logic_error("Previous simulation frame is still open");
    }

    frame_ = {};

    frame_.rawDelta = elapsed;
    frame_.paused = paused_;

    frame_.clampedDelta = std::clamp(elapsed, Nanoseconds::zero(), kMaxFrameDelta);

    frame_.deltaClamped = frame_.rawDelta != frame_.clampedDelta;

    frameOpen_ = true;

    if (paused_) {
        accumulatorUnits_ = 0;
        return;
    }

    accumulatorUnits_ += frame_.clampedDelta.count() * static_cast<i64>(kSimulationHz);
}

bool FixedTimestep::hasPendingTick() const noexcept {
    return frameOpen_ && !paused_ && accumulatorUnits_ >= kTickThreshold &&
           frame_.executedTicks < kMaxCatchUpTicks;
}

SimulationTick FixedTimestep::nextTick() const {
    if (!hasPendingTick()) {
        throw std::logic_error("No simulation tick pending");
    }

    const auto next = currentTick_.next();

    if (!next.has_value()) {
        throw std::overflow_error("Simulation tick overflow");
    }

    return *next;
}

void FixedTimestep::consumeTick() {
    const auto next = nextTick();

    accumulatorUnits_ -= kTickThreshold;

    currentTick_ = next;

    ++frame_.executedTicks;
}

FrameTiming FixedTimestep::endFrame() {
    if (!frameOpen_) {
        throw std::logic_error("No simulation frame open");
    }

    if (hasPendingTick()) {
        throw std::logic_error("Pending simulation ticks must be consumed");
    }

    if (!paused_ && accumulatorUnits_ >= kTickThreshold) {
        frame_.droppedTicks = static_cast<u32>(accumulatorUnits_ / kTickThreshold);

        accumulatorUnits_ %= kTickThreshold;
    }

    frame_.interpolationAlpha = interpolationAlpha();
    frameOpen_ = false;

    return frame_;
}

void FixedTimestep::setPaused(bool paused) noexcept {
    if (paused_ == paused) {
        return;
    }

    paused_ = paused;

    accumulatorUnits_ = 0;

    frame_.paused = paused;
}

bool FixedTimestep::isPaused() const noexcept { return paused_; }

SimulationTick FixedTimestep::currentTick() const noexcept { return currentTick_; }

double FixedTimestep::interpolationAlpha() const noexcept {
    if (paused_) {
        return 0.0;
    }

    const i64 remainder = accumulatorUnits_ % kTickThreshold;

    return static_cast<double>(remainder) / static_cast<double>(kTickThreshold);
}

} // namespace thj::core