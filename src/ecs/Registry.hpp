
#pragma once

#include "ecs/ComponentPool.hpp"
#include "ecs/EntityManager.hpp"
#include "ecs/QueryTraits.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <span>
#include <stdexcept>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace thj::ecs {

class Registry final {
  public:
    Registry() = default;
    ~Registry() = default;

    Registry(const Registry &) = delete;
    Registry &operator=(const Registry &) = delete;

    Registry(Registry &&) = delete;
    Registry &operator=(Registry &&) = delete;

    [[nodiscard]] EntityHandle create();

    bool destroy(EntityHandle entity);

    [[nodiscard]] bool isAlive(EntityHandle entity) const noexcept;

    [[nodiscard]] std::size_t aliveCount() const noexcept;

    [[nodiscard]] std::size_t slotCount() const noexcept;

    void reserveEntities(std::size_t capacity);

    [[nodiscard]] std::size_t poolCount() const noexcept;

    [[nodiscard]] bool isIterating() const noexcept;

    template <ComponentType T> [[nodiscard]] std::size_t componentCount() const {
        const auto *pool = findPool<T>();

        return pool != nullptr ? pool->size() : 0;
    }

    template <ComponentType T, typename... Args>
        requires std::constructible_from<T, Args...>
    T &emplace(EntityHandle entity, Args &&...args) {
        requireStructuralMutationAllowed();
        requireAlive(entity);

        auto &pool = assurePool<T>();

        return pool.storage().emplace(entity, std::forward<Args>(args)...);
    }

    template <ComponentType T> [[nodiscard]] bool has(EntityHandle entity) const {
        if (!isAlive(entity)) {
            return false;
        }

        const auto *pool = findPool<T>();

        return pool != nullptr && pool->contains(entity);
    }

    template <ComponentType T> [[nodiscard]] T *tryGet(EntityHandle entity) {
        if (!isAlive(entity)) {
            return nullptr;
        }

        auto *pool = findPool<T>();

        if (pool == nullptr) {
            return nullptr;
        }

        return pool->storage().tryGet(entity);
    }

    template <ComponentType T> [[nodiscard]] const T *tryGet(EntityHandle entity) const {
        if (!isAlive(entity)) {
            return nullptr;
        }

        const auto *pool = findPool<T>();

        if (pool == nullptr) {
            return nullptr;
        }

        return pool->storage().tryGet(entity);
    }

    template <ComponentType T> [[nodiscard]] T &get(EntityHandle entity) {
        auto *component = tryGet<T>(entity);

        if (component == nullptr) {
            throw std::out_of_range("Entity is not alive or component not found");
        }

        return *component;
    }

    template <ComponentType T> [[nodiscard]] const T &get(EntityHandle entity) const {
        const auto *component = tryGet<T>(entity);

        if (component == nullptr) {
            throw std::out_of_range("Entity is not alive or component not found");
        }

        return *component;
    }

    template <ComponentType T> bool remove(EntityHandle entity) {
        requireStructuralMutationAllowed();

        if (!isAlive(entity)) {
            return false;
        }

        auto *pool = findPool<T>();

        if (pool == nullptr) {
            return false;
        }

        return pool->remove(entity);
    }

    template <ComponentType T> void reserveComponents(std::size_t capacity) {
        requireStructuralMutationAllowed();

        assurePool<T>().storage().reserve(capacity);
    }

    template <typename... Ts, typename Fn>
        requires QueryComponents<Ts...> && std::invocable<Fn &, EntityHandle, Ts &...>
    std::size_t each(Fn &&callback) {
        return executeQuery<Ts...>(*this, callback, false);
    }

    template <typename... Ts, typename Fn>
        requires QueryComponents<Ts...> && std::invocable<Fn &, EntityHandle, const Ts &...>
    std::size_t each(Fn &&callback) const {
        return executeQuery<Ts...>(*this, callback, false);
    }

    template <typename... Ts, typename Fn>
        requires QueryComponents<Ts...> && std::invocable<Fn &, EntityHandle, Ts &...>
    std::size_t eachOrdered(Fn &&callback) {
        return executeQuery<Ts...>(*this, callback, true);
    }

    template <typename... Ts, typename Fn>
        requires QueryComponents<Ts...> && std::invocable<Fn &, EntityHandle, const Ts &...>
    std::size_t eachOrdered(Fn &&callback) const {
        return executeQuery<Ts...>(*this, callback, true);
    }

  private:
    class IterationGuard final {
      public:
        explicit IterationGuard(const Registry &registry) noexcept : registry_(registry) {
            ++registry_.iterationDepth_;
        }

        ~IterationGuard() noexcept { --registry_.iterationDepth_; }

        IterationGuard(const IterationGuard &) = delete;

        IterationGuard &operator=(const IterationGuard &) = delete;

        IterationGuard(IterationGuard &&) = delete;

        IterationGuard &operator=(IterationGuard &&) = delete;

      private:
        const Registry &registry_;
    };

    void requireAlive(EntityHandle entity) const;

    void requireStructuralMutationAllowed() const;

    template <typename... Ts, typename RegistryType, typename Fn>
    static std::size_t executeQuery(RegistryType &registry, Fn &callback, bool ordered) {

        const auto pools = std::tuple{registry.template findPool<Ts>()...};

        const bool allPoolsExist =
            std::apply([](auto *...pool) { return ((pool != nullptr) && ...); }, pools);

        if (!allPoolsExist) {
            return 0;
        }

        const IterationGuard guard{registry};

        const auto entityViews = std::apply(
            [](auto *...pool) {
                return std::array<std::span<const EntityHandle>, sizeof...(Ts)>{
                    pool->storage().entities()...};
            },
            pools);

        const auto smallest =
            std::min_element(entityViews.begin(), entityViews.end(),
                             [](auto lhs, auto rhs) { return lhs.size() < rhs.size(); });

        const auto candidates = *smallest;

        if (candidates.empty()) {
            return 0;
        }

        std::size_t matchedCount = 0;

        const auto visit = [&](EntityHandle entity) {
            if (!registry.isAlive(entity)) {
                return;
            }

            const auto components = std::apply(
                [&](auto *...pool) { return std::tuple{pool->storage().tryGet(entity)...}; },
                pools);

            const bool matches = std::apply(
                [](auto *...component) { return ((component != nullptr) && ...); }, components);

            if (!matches) {
                return;
            }

            std::apply(
                [&](auto *...component) {
                    std::invoke(callback, EntityHandle{entity}, (*component)...);
                },
                components);

            ++matchedCount;
        };

        if (!ordered) {

            for (const auto entity : candidates) {
                visit(entity);
            }

            return matchedCount;
        }

        std::vector<EntityHandle> sortedCandidates(candidates.begin(), candidates.end());

        std::sort(sortedCandidates.begin(), sortedCandidates.end(),
                  [](EntityHandle lhs, EntityHandle rhs) {
                      if (lhs.index != rhs.index) {
                          return lhs.index < rhs.index;
                      }

                      return lhs.generation < rhs.generation;
                  });

        for (const auto entity : sortedCandidates) {
            visit(entity);
        }

        return matchedCount;
    }

    template <ComponentType T> [[nodiscard]] ComponentPool<T> *findPool() {
        const auto key = std::type_index(typeid(T));

        const auto it = poolLookup_.find(key);

        if (it == poolLookup_.end()) {
            return nullptr;
        }

        return static_cast<ComponentPool<T> *>(pools_[it->second].get());
    }

    template <ComponentType T> [[nodiscard]] const ComponentPool<T> *findPool() const {
        const auto key = std::type_index(typeid(T));

        const auto it = poolLookup_.find(key);

        if (it == poolLookup_.end()) {
            return nullptr;
        }

        return static_cast<const ComponentPool<T> *>(pools_[it->second].get());
    }

    template <ComponentType T> ComponentPool<T> &assurePool() {
        if (auto *existing = findPool<T>()) {
            return *existing;
        }

        auto pool = std::make_unique<ComponentPool<T>>();

        auto *result = pool.get();

        const auto index = pools_.size();

        pools_.push_back(std::move(pool));

        try {
            const auto insertion = poolLookup_.emplace(std::type_index(typeid(T)), index);

            if (!insertion.second) {
                throw std::logic_error("Component pool already registered");
            }
        } catch (...) {
            pools_.pop_back();

            throw;
        }

        return *result;
    }

    EntityManager entityManager_;

    std::vector<std::unique_ptr<IComponentPool>> pools_;

    std::unordered_map<std::type_index, std::size_t> poolLookup_;

    mutable std::size_t iterationDepth_{};
};

} // namespace thj::ecs
