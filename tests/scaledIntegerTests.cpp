#include "moped/ScaledInteger.hpp"
#include <catch2/catch.hpp>
#include <cstdint>
#include <format>

using S6Int = moped::ScaledInteger<std::int64_t, 6>;
using S9Int = moped::ScaledInteger<std::int64_t, 9>;
static_assert(moped::is_scaled_int<S6Int>);

TEST_CASE("ScaledInteger class member tests", "[ScaledInteger]") {
  REQUIRE(S6Int{"-.1234567"}.getRawIntegerValue() == -123456);
  REQUIRE(S9Int{"60000.1234"}.getRawIntegerValue() == 60000123400000);
  REQUIRE(S9Int{"2.34567e-4"}.toString() == "0.000234567");
  REQUIRE((S9Int{"3.00300"} / S9Int{"0.0003"}) == S9Int{"10010.0"});
  REQUIRE((S9Int{"3.00300"} * S9Int{"0.0003"}) == S9Int{"0.0009009"});
  REQUIRE((S9Int{"3.00300"} + S9Int{"0.0003"}) == S9Int{"3.0033"});
  REQUIRE((S9Int{"3.00300"} - S9Int{"0.0003"}) == S9Int{"3.0027"});
}