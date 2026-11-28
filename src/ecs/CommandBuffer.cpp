
#include "ecs/CommandBuffer.hpp"

#include <limits>

namespace thj::ecs {

namespace {

class PlaybackGuard final {
  public:
    explicit PlaybackGuard(bool &playing) noexcept : playing_(playing) { playing_ = true; }

    ~PlaybackGuard() noexcept { playing_ = false; }

    PlaybackGuard(const PlaybackGuard &) = delete;
    PlaybackGuard &operator=(const PlaybackGuard &) = delete;

  private:
    bool &playing_;
};

} // namespace

CommandBuffer::CommandBuffer(Registry &registry) noexcept : registry_(registry) {}

CommandBuffer::DeferredEntity CommandBuffer::create() {
    requireIdle();

    if (deferredCount_ == std::numeric_limits<std::size_t>::max()) {
        throw std::length_error("Deferred entity capacity exhausted");
    }

    if (!identity_) {
        identity_ = std::make_shared<const BatchIdentity>();
    }

    const auto slot = deferredCount_;

    commands_.emplace_back([slot](Registry &registry, PlaybackResult &result) {
        const auto entity = registry.create();

        result.resolved_[slot] = entity;

        return true;
    });

    ++deferredCount_;

    return DeferredEntity{identity_, slot};
}

CommandBuffer::PlaybackResult CommandBuffer::playback() {
    requireIdle();

    if (registry_.isIterating()) {
        throw std::logic_error("Cannot play ECS commands during active iteration");
    }

    PlaybackResult result;

    result.identity_ = identity_;

    result.resolved_.resize(deferredCount_);

    const PlaybackGuard guard{playing_};

    for (std::size_t index = 0; index < commands_.size(); ++index) {
        try {
            const bool applied = commands_[index](registry_, result);

            if (applied) {
                ++result.applied;
            } else {
                ++result.skipped;
            }
        } catch (...) {
            result.error = std::current_exception();

            ++result.failed;

            result.discarded = commands_.size() - index - 1;

            break;
        }
    }

    resetBatch();

    return result;
}

void CommandBuffer::clear() {
    requireIdle();

    const PlaybackGuard guard{playing_};

    resetBatch();
}

void CommandBuffer::reserve(std::size_t capacity) {
    requireIdle();

    commands_.reserve(capacity);
}

std::size_t CommandBuffer::size() const noexcept { return commands_.size(); }

bool CommandBuffer::empty() const noexcept { return commands_.empty(); }

bool CommandBuffer::isPlaying() const noexcept { return playing_; }

void CommandBuffer::requireIdle() const {
    if (playing_) {
        throw std::logic_error("Command buffer cannot be modified during playback or cleanup");
    }
}

void CommandBuffer::validateTarget(const DeferredEntity &target) const {
    if (!identity_ || target.identity_ != identity_ || target.slot_ >= deferredCount_) {
        throw std::invalid_argument("Deferred entity belongs to another buffer or batch");
    }
}

void CommandBuffer::resetBatch() noexcept {
    commands_.clear();

    identity_.reset();

    deferredCount_ = 0;
}

} // namespace thj::ecs
