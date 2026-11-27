#pragma once

#include "core/Types.hpp"

#include <limits>
#include <optional>

namespace thj::core {

class FixedPoint final {
  public:
    static constexpr i64 kScale = i64{1} << 16;

    constexpr FixedPoint() noexcept = default;

    [[nodiscard]] static constexpr FixedPoint fromRaw(i64 raw) noexcept { return FixedPoint(raw); }

    [[nodiscard]] static constexpr FixedPoint fromPixels(i32 pixels) noexcept {
        return fromRaw(static_cast<i64>(pixels) * kScale);
    }

    [[nodiscard]] constexpr i64 raw() const noexcept { return raw_; }

    [[nodiscard]] constexpr i64 floorPixels() const noexcept {
        return raw_ / kScale - (raw_ % kScale < 0 ? 1 : 0);
    }

    [[nodiscard]] constexpr double toDouble() const noexcept {
        return static_cast<double>(raw_) / static_cast<double>(kScale);
    }

    [[nodiscard]] constexpr std::optional<FixedPoint> checkedAdd(FixedPoint other) const noexcept {
        const i64 rhs = other.raw_;

        if ((rhs > 0 && raw_ > std::numeric_limits<i64>::max() - rhs) ||
            (rhs < 0 && raw_ < std::numeric_limits<i64>::min() - rhs)) {
            return std::nullopt;
        }

        return fromRaw(raw_ + rhs);
    }

    constexpr bool operator==(const FixedPoint &) const noexcept = default;

  private:
    explicit constexpr FixedPoint(i64 raw) noexcept : raw_(raw) {}

    i64 raw_{};
};

} // namespace thj::core
