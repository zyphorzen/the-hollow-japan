#pragma once

#include <cstdint>

namespace thj::core {

using i32 = std::int32_t;
using i64 = std::int64_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

struct EntityHandle {
    u32 index{};
    u32 generation{};

    [[nodiscard]] constexpr bool valid() const noexcept { return generation != 0; }
    [[nodiscard]] constexpr u64 packed() const noexcept {
        return (static_cast<u64>(generation) << 32) | index;
    }

    [[nodiscard]] static constexpr EntityHandle fromPacked(u64 value) noexcept {
        return {static_cast<u32>(value), static_cast<u32>(value >> 32)};
    }

    constexpr bool operator==(const EntityHandle &) const noexcept = default;
};

} // namespace thj::core
