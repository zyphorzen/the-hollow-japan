#include "core/math/FixedPoint.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits>

using thj::core::FixedPoint;
using thj::core::i64;

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