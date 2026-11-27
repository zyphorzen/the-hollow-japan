#pragma once

#include "core/math/FixedPoint.hpp"

namespace thj::core {

inline constexpr i64 kTileSizePixels = 16;
inline constexpr i64 kChunkSizeTiles = 32;

inline constexpr i64 kChunkSizePixels = kTileSizePixels * kChunkSizeTiles;

struct Position2 {
    FixedPoint x{};
    FixedPoint y{};

    constexpr bool operator==(const Position2 &) const noexcept = default;
};

struct TileCoord {
    i64 x{};
    i64 y{};

    constexpr bool operator==(const TileCoord &) const noexcept = default;
};

struct ChunkCoord {
    i64 x{};
    i64 y{};

    constexpr bool operator==(const ChunkCoord &) const noexcept = default;
};

struct LocalTileCoord {
    i64 x{};
    i64 y{};

    constexpr bool operator==(const LocalTileCoord &) const noexcept = default;
};

template <i64 Divisor> [[nodiscard]] constexpr i64 floorDiv(i64 value) noexcept {
    static_assert(Divisor > 0);

    return value / Divisor - (value % Divisor < 0 ? 1 : 0);
}

template <i64 Divisor> [[nodiscard]] constexpr i64 floorMod(i64 value) noexcept {
    static_assert(Divisor > 0);

    const i64 remainder = value % Divisor;

    return remainder < 0 ? remainder + Divisor : remainder;
}

[[nodiscard]] constexpr TileCoord toTile(Position2 position) noexcept {
    return {floorDiv<kTileSizePixels>(position.x.floorPixels()),
            floorDiv<kTileSizePixels>(position.y.floorPixels())};
}

[[nodiscard]] constexpr ChunkCoord toChunk(TileCoord tile) noexcept {
    return {floorDiv<kChunkSizeTiles>(tile.x), floorDiv<kChunkSizeTiles>(tile.y)};
}

[[nodiscard]] constexpr LocalTileCoord toLocalTile(TileCoord tile) noexcept {
    return {floorMod<kChunkSizeTiles>(tile.x), floorMod<kChunkSizeTiles>(tile.y)};
}

} // namespace thj::core
