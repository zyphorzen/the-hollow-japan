#pragma once

#include "ecs/ComponentStorage.hpp"

#include <cstddef>

namespace thj::ecs {

class IComponentPool {
  public:
    virtual ~IComponentPool() = default;

    virtual bool remove(EntityHandle entity) noexcept = 0;

    [[nodiscard]] virtual bool contains(EntityHandle entity) const noexcept = 0;

    [[nodiscard]] virtual std::size_t size() const noexcept = 0;
};

template <ComponentType T> class ComponentPool final : public IComponentPool {
  public:
    ComponentPool() = default;

    ~ComponentPool() override = default;

    bool remove(EntityHandle entity) noexcept override { return storage_.remove(entity); }

    [[nodiscard]] bool contains(EntityHandle entity) const noexcept override {
        return storage_.contains(entity);
    }

    [[nodiscard]] std::size_t size() const noexcept override { return storage_.size(); }

    [[nodiscard]] ComponentStorage<T> &storage() noexcept { return storage_; }

    [[nodiscard]] const ComponentStorage<T> &storage() const noexcept { return storage_; }

  private:
    ComponentStorage<T> storage_;
};

} // namespace thj::ecs