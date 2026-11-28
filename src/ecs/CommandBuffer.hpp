
#pragma once

#include "ecs/Registry.hpp"

#include <concepts>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace thj::ecs {

class CommandBuffer final {
  private:
    struct BatchIdentity {};

  public:
    struct PlaybackResult;

    class DeferredEntity final {
      public:
        DeferredEntity(const DeferredEntity &) = default;

        DeferredEntity &operator=(const DeferredEntity &) = default;

      private:
        friend class CommandBuffer;
        friend struct PlaybackResult;

        DeferredEntity(std::shared_ptr<const BatchIdentity> identity, std::size_t slot) noexcept
            : identity_(std::move(identity)), slot_(slot) {}

        std::shared_ptr<const BatchIdentity> identity_;
        std::size_t slot_{};
    };

    struct PlaybackResult {
        std::size_t applied{};
        std::size_t skipped{};
        std::size_t failed{};
        std::size_t discarded{};

        std::exception_ptr error{};

        [[nodiscard]] bool success() const noexcept { return failed == 0; }

        [[nodiscard]] std::size_t total() const noexcept {
            return applied + skipped + failed + discarded;
        }

        [[nodiscard]] std::optional<EntityHandle>
        resolve(const DeferredEntity &entity) const noexcept {
            if (!identity_ || identity_ != entity.identity_ || entity.slot_ >= resolved_.size()) {
                return std::nullopt;
            }

            return resolved_[entity.slot_];
        }

        void rethrowIfFailed() const {
            if (error) {
                std::rethrow_exception(error);
            }
        }

      private:
        friend class CommandBuffer;

        std::shared_ptr<const BatchIdentity> identity_;

        std::vector<std::optional<EntityHandle>> resolved_;
    };

    explicit CommandBuffer(Registry &registry) noexcept;

    ~CommandBuffer() = default;

    CommandBuffer(const CommandBuffer &) = delete;

    CommandBuffer &operator=(const CommandBuffer &) = delete;

    CommandBuffer(CommandBuffer &&) = delete;

    CommandBuffer &operator=(CommandBuffer &&) = delete;

    [[nodiscard]] DeferredEntity create();

    template <typename Target>
        requires(std::same_as<Target, EntityHandle> || std::same_as<Target, DeferredEntity>)
    void destroy(Target target) {
        requireIdle();
        validateTarget(target);

        commands_.emplace_back(
            [target = std::move(target)](Registry &registry, PlaybackResult &result) {
                const auto entity = resolveTarget(target, result);

                if (!entity.has_value()) {
                    return false;
                }

                return registry.destroy(*entity);
            });
    }

    template <ComponentType T, typename Target, typename... Args>
        requires((std::same_as<Target, EntityHandle> || std::same_as<Target, DeferredEntity>) &&
                 (std::constructible_from<std::decay_t<Args>, Args &&> && ...) &&
                 std::constructible_from<T, std::decay_t<Args> && ...>)
    void emplace(Target target, Args &&...args) {
        requireIdle();
        validateTarget(target);

        using Payload = std::tuple<std::decay_t<Args>...>;

        commands_.emplace_back(
            [target = std::move(target), payload = Payload{std::forward<Args>(args)...}](
                Registry &registry, PlaybackResult &result) mutable {
                const auto entity = resolveTarget(target, result);

                if (!entity.has_value() || !registry.isAlive(*entity)) {
                    return false;
                }

                if (registry.has<T>(*entity)) {
                    return false;
                }

                std::apply(
                    [&](auto &&...values) {
                        registry.emplace<T>(*entity, std::forward<decltype(values)>(values)...);
                    },
                    std::move(payload));

                return true;
            });
    }

    template <ComponentType T, typename Target>
        requires(std::same_as<Target, EntityHandle> || std::same_as<Target, DeferredEntity>)
    void remove(Target target) {
        requireIdle();
        validateTarget(target);

        commands_.emplace_back(
            [target = std::move(target)](Registry &registry, PlaybackResult &result) {
                const auto entity = resolveTarget(target, result);

                if (!entity.has_value()) {
                    return false;
                }

                return registry.remove<T>(*entity);
            });
    }

    PlaybackResult playback();

    void clear();

    void reserve(std::size_t capacity);

    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] bool isPlaying() const noexcept;

  private:
    using Command = std::move_only_function<bool(Registry &, PlaybackResult &)>;

    void requireIdle() const;

    void resetBatch() noexcept;

    void validateTarget(EntityHandle) const noexcept {}

    void validateTarget(const DeferredEntity &target) const;

    [[nodiscard]] static std::optional<EntityHandle>
    resolveTarget(EntityHandle target, const PlaybackResult &) noexcept {
        return target;
    }

    [[nodiscard]] static std::optional<EntityHandle>
    resolveTarget(const DeferredEntity &target, const PlaybackResult &result) noexcept {
        return result.resolve(target);
    }

    Registry &registry_;

    std::shared_ptr<const BatchIdentity> identity_;

    std::vector<Command> commands_;

    std::size_t deferredCount_{};

    bool playing_{false};
};

} // namespace thj::ecs
