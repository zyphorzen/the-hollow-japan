#include "core/world/Coordinates.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace thj::core;

TEST_CASE("World position converts to tile coordinates", "[unit][world]") {
    constexpr Position2 origin{FixedPoint::fromPixels(0), FixedPoint::fromPixels(0)};
    constexpr Position2 boundary{FixedPoint::fromPixels(16), FixedPoint::fromPixels(32)};
    constexpr Position2 insideTile{FixedPoint::fromPixels(15), FixedPoint::fromPixels(31)};

    CHECK(toTile(origin) == (TileCoord{0, 0}));
    CHECK(toTile(boundary) == (TileCoord{1, 2}));
    CHECK(toTile(insideTile) == (TileCoord{0, 1}));
}

TEST_CASE("Tile coordinates convert to chunks", "[unit][world]") {
    CHECK(kTileSizePixels == 16);
    CHECK(kChunkSizeTiles == 32);
    CHECK(kChunkSizePixels == 512);

    CHECK(toChunk({0, 0}) == (ChunkCoord{0, 0}));
    CHECK(toChunk({31, 31}) == (ChunkCoord{0, 0}));
    CHECK(toChunk({32, 32}) == (ChunkCoord{1, 1}));
    CHECK(toChunk({63, 64}) == (ChunkCoord{1, 2}));

    CHECK(toChunk({4095, 4095}) == (ChunkCoord{127, 127}));

    CHECK(toLocalTile({4095, 4095}) == (LocalTileCoord{31, 31}));
}

TEST_CASE("Negative coordinates use floor division", "[unit][world]") {
    CHECK(floorDiv<32>(-1) == -1);
    CHECK(floorDiv<32>(-32) == -1);
    CHECK(floorDiv<32>(-33) == -2);

    CHECK(floorMod<32>(-1) == 31);
    CHECK(floorMod<32>(-32) == 0);
    CHECK(floorMod<32>(-33) == 31);

    CHECK(toChunk({-1, -33}) == (ChunkCoord{-1, -2}));

    CHECK(toLocalTile({-1, -33}) == (LocalTileCoord{31, 31}));

    constexpr Position2 negative{FixedPoint::fromRaw(-1), FixedPoint::fromRaw(-32768)};

    CHECK(toTile(negative) == (TileCoord{-1, -1}));

    for (i64 value = -1024; value <= 1024; ++value) {
        const auto chunk = floorDiv<32>(value);
        const auto local = floorMod<32>(value);

        REQUIRE(local >= 0);
        REQUIRE(local < 32);

        CHECK(chunk * 32 + local == value);
    }
}