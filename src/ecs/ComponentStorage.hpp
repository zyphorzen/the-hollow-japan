
#pragma once

#include "ecs/SparseSet.hpp"

#include <concepts>
#include <cstddef>
#include <memory>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace thj::ecs {

template <typename T>
concept ComponentType =
    std::is_object_v<T> && std::same_as<T, std::remove_cv_t<T>> && (!std::is_array_v<T>) &&
    (!std::same_as<T, bool>) && std::is_nothrow_move_constructible_v<T> &&
    std::is_nothrow_move_assignable_v<T> && std::is_nothrow_destructible_v<T>;

template <ComponentType T> class ComponentStorage final {
  public:
    using Component = T;

    ComponentStorage() = default;
    ~ComponentStorage() = default;

    ComponentStorage(const ComponentStorage &) = delete;
    ComponentStorage &operator=(const ComponentStorage &) = delete;

    ComponentStorage(ComponentStorage &&) = delete;
    ComponentStorage &operator=(ComponentStorage &&) = delete;

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    T &emplace(EntityHandle entity, Args &&...args) {

        if (!entities_.insert(entity)) {
            throw std::invalid_argument("Invalid entity or conflicting component membership");
        }

        try {

            return components_.emplace_back(std::forward<Args>(args)...);
        } catch (...) {

            entities_.remove(entity);

            throw;
        }
    }

    bool remove(EntityHandle entity) noexcept {
        const auto foundIndex = entities_.indexOf(entity);

        if (!foundIndex.has_value()) {
            return false;
        }

        const auto index = *foundIndex;
        const auto lastIndex = components_.size() - 1;

        if (index != lastIndex) {
            components_[index] = std::move(components_[lastIndex]);
        }

        components_.pop_back();

        entities_.remove(entity);

        return true;
    }

    [[nodiscard]] bool contains(EntityHandle entity) const noexcept {
        return entities_.contains(entity);
    }

    [[nodiscard]] T *tryGet(EntityHandle entity) noexcept {
        const auto index = entities_.indexOf(entity);

        if (!index.has_value()) {
            return nullptr;
        }

        return std::addressof(components_[*index]);
    }

    [[nodiscard]] const T *tryGet(EntityHandle entity) const noexcept {
        const auto index = entities_.indexOf(entity);

        if (!index.has_value()) {
            return nullptr;
        }

        return &components_[*index];
    }

    [[nodiscard]] T &get(EntityHandle entity) {
        auto *component = tryGet(entity);

        if (component == nullptr) {
            throw std::out_of_range("Component not found");
        }

        return *component;
    }

    [[nodiscard]] const T &get(EntityHandle entity) const {
        const auto *component = tryGet(entity);

        if (component == nullptr) {
            throw std::out_of_range("Component not found");
        }

        return *component;
    }

    [[nodiscard]] std::span<const EntityHandle> entities() const noexcept {
        return entities_.entities();
    }

    [[nodiscard]] std::span<T> components() noexcept { return std::span<T>{components_}; }

    [[nodiscard]] std::span<const T> components() const noexcept {
        return std::span<const T>{components_};
    }

    [[nodiscard]] std::size_t size() const noexcept { return components_.size(); }

    [[nodiscard]] bool empty() const noexcept { return components_.empty(); }

    void reserve(std::size_t capacity) {
        entities_.reserve(capacity);

        components_.reserve(capacity);
    }

    void clear() noexcept {
        components_.clear();

        entities_.clear();
    }

  private:
    SparseSet entities_;

    std::vector<T> components_;
};

} // namespace thj::ecs
