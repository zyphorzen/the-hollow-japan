
#pragma once

#include "ecs/ComponentPool.hpp"
#include "ecs/EntityManager.hpp"

#include <concepts>
#include <cstddef>
#include <memory>
#include <stdexcept>
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

    bool destroy(EntityHandle entity) noexcept;

    [[nodiscard]] bool isAlive(EntityHandle entity) const noexcept;

    [[nodiscard]] std::size_t aliveCount() const noexcept;

    [[nodiscard]] std::size_t slotCount() const noexcept;

    void reserveEntities(std::size_t capacity);

    [[nodiscard]] std::size_t poolCount() const noexcept;

    template <ComponentType T> [[nodiscard]] std::size_t componentCount() const {
        const auto *pool = findPool<T>();

        return pool != nullptr ? pool->size() : 0;
    }

    template <ComponentType T, typename... Args>
        requires std::constructible_from<T, Args...>
    T &emplace(EntityHandle entity, Args &&...args) {
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
        assurePool<T>().storage().reserve(capacity);
    }

  private:
    void requireAlive(EntityHandle entity) const;

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
};

} // namespace thj::ecs
