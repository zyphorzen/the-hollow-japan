#include "simulation/SimulationWorld.hpp"

#include "simulation/systems/LifetimeSystem.hpp"
#include "simulation/systems/MovementSystem.hpp"

#include <stdexcept>

namespace thj::simulation {

SimulationWorld::SimulationWorld() : commands_(registry_) {}

SimulationWorld::DeferredEntity SimulationWorld::queueCreate() {
    requireRecordingAllowed();

    return commands_.create();
}

TickReport SimulationWorld::step(core::SimulationTick tick) {
    if (failed_) {
        throw std::logic_error("Simulation world has failed");
    }

    if (stepping_) {
        throw std::logic_error("Reentrant simulation step");
    }

    if (registry_.isIterating()) {
        throw std::logic_error("Cannot step during active ECS iteration");
    }

    const auto expected = currentTick_.next();

    if (!expected.has_value()) {
        throw std::overflow_error("Simulation tick capacity exhausted");
    }

    if (tick != *expected) {
        throw std::invalid_argument("Simulation ticks must be sequential");
    }

    stepping_ = true;

    try {
        TickReport report{};

        report.tick = tick;

        report.beginBarrier = commands_.playback();

        report.beginBarrier.rethrowIfFailed();

        MovementSystem::update(registry_);

        LifetimeSystem::update(registry_, commands_);

        report.endBarrier = commands_.playback();

        report.endBarrier.rethrowIfFailed();

        currentTick_ = tick;

        stepping_ = false;

        return report;
    } catch (...) {
        failed_ = true;

        stepping_ = false;

        throw;
    }
}

const ecs::Registry &SimulationWorld::registry() const noexcept { return registry_; }

core::SimulationTick SimulationWorld::currentTick() const noexcept { return currentTick_; }

bool SimulationWorld::failed() const noexcept { return failed_; }

bool SimulationWorld::isStepping() const noexcept { return stepping_; }

std::size_t SimulationWorld::pendingCommands() const noexcept { return commands_.size(); }

void SimulationWorld::requireRecordingAllowed() const {
    if (failed_) {
        throw std::logic_error("Cannot record commands in a failed world");
    }

    if (stepping_) {
        throw std::logic_error("External command recording during simulation step");
    }
}

} // namespace thj::simulation