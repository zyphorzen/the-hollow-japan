
#include "core/math/FixedPoint.hpp"

#include <catch2/catch_test_macros.hpp>

#include <limits>

using thj::core::FixedPoint;
using thj::core::i32;
using thj::core::i64;

TEST_CASE("Fixed point construction and conversion", "[unit][math]") {
    STATIC_REQUIRE(FixedPoint::kScale == 65536);

    constexpr FixedPoint zero{};

    STATIC_REQUIRE(zero.raw() == 0);

    constexpr auto positive = FixedPoint::fromPixels(10);
    constexpr auto negative = FixedPoint::fromPixels(-10);

    STATIC_REQUIRE(positive.raw() == 655360);
    STATIC_REQUIRE(negative.raw() == -655360);

    constexpr auto fractional = FixedPoint::fromRaw(32768);

    STATIC_REQUIRE(fractional.raw() == 32768);

    CHECK(fractional.toDouble() == 0.5);
    CHECK(positive.toDouble() == 10.0);
    CHECK(negative.toDouble() == -10.0);
    CHECK(zero.toDouble() == 0.0);

    constexpr auto maximum = std::numeric_limits<i32>::max();

    constexpr auto minimum = std::numeric_limits<i32>::min();

    CHECK(FixedPoint::fromPixels(maximum).raw() == static_cast<i64>(maximum) * FixedPoint::kScale);

    CHECK(FixedPoint::fromPixels(minimum).raw() == static_cast<i64>(minimum) * FixedPoint::kScale);

    CHECK(FixedPoint::fromPixels(1) == FixedPoint::fromRaw(65536));

    CHECK(FixedPoint::fromPixels(1) != FixedPoint::fromPixels(2));
}

TEST_CASE("Fixed point floor conversion handles signed coordinates", "[unit][math]") {

    CHECK(FixedPoint{}.floorPixels() == 0);
    CHECK(FixedPoint::fromPixels(1).floorPixels() == 1);
    CHECK(FixedPoint::fromPixels(10).floorPixels() == 10);

    CHECK(FixedPoint::fromRaw(1).floorPixels() == 0);
    CHECK(FixedPoint::fromRaw(32768).floorPixels() == 0);
    CHECK(FixedPoint::fromRaw(65535).floorPixels() == 0);
    CHECK(FixedPoint::fromRaw(65537).floorPixels() == 1);
    CHECK(FixedPoint::fromRaw(98304).floorPixels() == 1);

    CHECK(FixedPoint::fromPixels(-1).floorPixels() == -1);
    CHECK(FixedPoint::fromPixels(-10).floorPixels() == -10);

    CHECK(FixedPoint::fromRaw(-1).floorPixels() == -1);
    CHECK(FixedPoint::fromRaw(-32768).floorPixels() == -1);
    CHECK(FixedPoint::fromRaw(-65535).floorPixels() == -1);
    CHECK(FixedPoint::fromRaw(-65537).floorPixels() == -2);
    CHECK(FixedPoint::fromRaw(-98304).floorPixels() == -2);

    constexpr auto maximum = std::numeric_limits<i64>::max();

    constexpr auto minimum = std::numeric_limits<i64>::min();

    CHECK(FixedPoint::fromRaw(maximum).floorPixels() == maximum / FixedPoint::kScale);

    CHECK(FixedPoint::fromRaw(minimum).floorPixels() == minimum / FixedPoint::kScale);
}

TEST_CASE("Fixed point addition rejects overflow", "[unit][math]") {
    const auto sum = FixedPoint::fromPixels(2).checkedAdd(FixedPoint::fromRaw(32768));

    REQUIRE(sum.has_value());

    CHECK(sum->raw() == 163840);

    const auto maximum = FixedPoint::fromRaw(std::numeric_limits<i64>::max());

    const auto minimum = FixedPoint::fromRaw(std::numeric_limits<i64>::min());

    CHECK_FALSE(maximum.checkedAdd(FixedPoint::fromRaw(1)).has_value());

    CHECK_FALSE(minimum.checkedAdd(FixedPoint::fromRaw(-1)).has_value());

    CHECK(maximum.checkedAdd(FixedPoint::fromRaw(0)).has_value());

    CHECK(minimum.checkedAdd(FixedPoint::fromRaw(1)).has_value());
}
