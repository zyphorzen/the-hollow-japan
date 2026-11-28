#pragma once

#include "core/time/SimulationTime.hpp"
#include "ecs/CommandBuffer.hpp"
#include "ecs/Registry.hpp"

#include <utility>

namespace thj::simulation {

struct TickReport {
    core::SimulationTick tick{};

    ecs::CommandBuffer::PlaybackResult beginBarrier;

    ecs::CommandBuffer::PlaybackResult endBarrier;
};

class SimulationWorld final {
  public:
    using DeferredEntity = ecs::CommandBuffer::DeferredEntity;

    SimulationWorld();

    ~SimulationWorld() = default;

    SimulationWorld(const SimulationWorld &) = delete;

    SimulationWorld &operator=(const SimulationWorld &) = delete;

    SimulationWorld(SimulationWorld &&) = delete;

    SimulationWorld &operator=(SimulationWorld &&) = delete;

    [[nodiscard]] DeferredEntity queueCreate();

    template <typename Target> void queueDestroy(Target target) {
        requireRecordingAllowed();

        commands_.destroy(std::move(target));
    }

    template <ecs::ComponentType T, typename Target, typename... Args>
    void queueEmplace(Target target, Args &&...args) {
        requireRecordingAllowed();

        commands_.emplace<T>(std::move(target), std::forward<Args>(args)...);
    }

    template <ecs::ComponentType T, typename Target> void queueRemove(Target target) {
        requireRecordingAllowed();

        commands_.remove<T>(std::move(target));
    }

    [[nodiscard]] TickReport step(core::SimulationTick tick);

    [[nodiscard]] const ecs::Registry &registry() const noexcept;

    [[nodiscard]] core::SimulationTick currentTick() const noexcept;

    [[nodiscard]] bool failed() const noexcept;

    [[nodiscard]] bool isStepping() const noexcept;

    [[nodiscard]] std::size_t pendingCommands() const noexcept;

  private:
    void requireRecordingAllowed() const;

    ecs::Registry registry_;

    ecs::CommandBuffer commands_;

    core::SimulationTick currentTick_{};

    bool stepping_{false};

    bool failed_{false};
};

} // namespace thj::simulation