#include "moped/ScaledInteger.hpp"
#include <catch2/catch_all.hpp>
#include <cstdint>
#include <format>

using S6Int = moped::ScaledInteger<std::int64_t, 6>;
using S8Int = moped::ScaledInteger<std::int64_t, 8>;
static_assert(moped::is_scaled_int<S6Int>);
TEST_CASE("ScaledInteger basic tests", "[ScaledInteger]") {
  REQUIRE(S8Int{"886110000000.00"} == S8Int{"886110000000"});
}

TEST_CASE("ScaledInteger Arithmetic tests", "[ScaledInteger]") {
  REQUIRE(S6Int{"-.1234567"}.getRawIntegerValue() == -123456);
  REQUIRE(S8Int{"60000.1234"}.getRawIntegerValue() == 6000012340000);
  REQUIRE(S8Int{"2.34567e-4"}.toString() == "0.00023456");
  REQUIRE((S8Int{"3.00300"} / S8Int{"0.0003"}) == S8Int{"10010.0"});
  REQUIRE((S8Int{"3.00300"} * S8Int{"0.0003"}) == S8Int{"0.0009009"});
  REQUIRE((S8Int{"3.00300"} + S8Int{"0.0003"}) == S8Int{"3.0033"});
  REQUIRE((S8Int{"3.00300"} - S8Int{"0.0003"}) == S8Int{"3.0027"});
}