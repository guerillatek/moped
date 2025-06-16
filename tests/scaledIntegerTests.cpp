#include "moped/ScaledInteger.hpp"
#include <boost/test/unit_test.hpp>
#include <cstdint>
#include <format>

struct TestFixture {};

using S6Int = moped::ScaledInteger<std::int64_t, 6>;
using S9Int = moped::ScaledInteger<std::int64_t, 9>;
static_assert(moped::is_scaled_int<S6Int>);

BOOST_FIXTURE_TEST_SUITE(ScaledIntegertests, TestFixture);

BOOST_AUTO_TEST_CASE(ClassMemberTests) {
  BOOST_REQUIRE_EQUAL(S6Int{"-.1234567"}.getRawIntegerValue(), -123456);
  BOOST_REQUIRE_EQUAL(S9Int{"60000.1234"}.getRawIntegerValue(), 60000123400000);
  BOOST_REQUIRE_EQUAL(S9Int{"2.34567e-4"}.toString(), "0.000234567");
  BOOST_REQUIRE_EQUAL((S9Int{"3.00300"} / S9Int{"0.0003"}), S9Int{"10010.0"});
  BOOST_REQUIRE_EQUAL((S9Int{"3.00300"} * S9Int{"0.0003"}), S9Int{"0.0009009"});
  BOOST_REQUIRE_EQUAL((S9Int{"3.00300"} + S9Int{"0.0003"}), S9Int{"3.0033"});
  BOOST_REQUIRE_EQUAL((S9Int{"3.00300"} - S9Int{"0.0003"}), S9Int{"3.0027"});
}

BOOST_AUTO_TEST_SUITE_END();