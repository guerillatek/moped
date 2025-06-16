#include "moped/ScaledInteger.hpp"

#include <boost/test/unit_test.hpp>
#include <format>

using namespace moped;

struct TestFixture {};

BOOST_FIXTURE_TEST_SUITE(MakeDecimalTest, TestFixture);

namespace utf = boost::unit_test;

BOOST_AUTO_TEST_CASE(floatToScaledInt_test, *utf::tolerance(1e-10)) {

  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(1.0, 1), 10);
  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(1.0, 2), 100);
  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(1.0, 3), 1000);
  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(1.0, 9), 1000000000);

  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(1.1, 1), 11);
  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(1.1, 2), 110);
  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(1.1, 3), 1100);
  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(1.1, 9), 1100000000);

  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(1.001, 1), 10);
  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(1.001, 2), 100);
  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(1.001, 3), 1001);
  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(1.001, 9), 1001000000);

  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(0.001, 1), 0);
  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(0.001, 2), 0);
  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(0.001, 3), 1);
  BOOST_REQUIRE_EQUAL(floatToScaledInt<int64_t>(0.001, 9), 1000000);

  BOOST_REQUIRE_EQUAL(0, floatToScaledInt<int64_t>(0., 9));

  // TODO: figure out how to do parameterised unit tests in boost test
  {
    int64_t expectedScaledIntVal = 999999999000000000;
    double expectedDoubleVal = 999999999.;
    BOOST_REQUIRE_EQUAL(expectedScaledIntVal,
                        floatToScaledInt<int64_t>(expectedDoubleVal, 9));
    BOOST_REQUIRE_EQUAL(expectedDoubleVal,
                        scaledIntToFloat<double>(expectedScaledIntVal, 9));
  }

  {
    int64_t expectedScaledIntVal = -999999999000000000;
    double expectedDoubleVal = -999999999.;
    BOOST_REQUIRE_EQUAL(expectedScaledIntVal,
                        floatToScaledInt<int64_t>(expectedDoubleVal, 9));
    BOOST_REQUIRE_EQUAL(expectedDoubleVal,
                        scaledIntToFloat<double>(expectedScaledIntVal, 9));
  }

  {
    int64_t expectedScaledIntVal = 999999999;
    double expectedDoubleVal = .999999999;
    BOOST_REQUIRE_EQUAL(expectedScaledIntVal,
                        floatToScaledInt<int64_t>(expectedDoubleVal, 9));
    BOOST_REQUIRE_EQUAL(expectedDoubleVal,
                        scaledIntToFloat<double>(expectedScaledIntVal, 9));
  }

  {
    int64_t expectedScaledIntVal = -999999999;
    double expectedDoubleVal = -.999999999;
    BOOST_REQUIRE_EQUAL(expectedScaledIntVal,
                        floatToScaledInt<int64_t>(expectedDoubleVal, 9));
    BOOST_REQUIRE_EQUAL(expectedDoubleVal,
                        scaledIntToFloat<double>(expectedScaledIntVal, 9));
  }
}

BOOST_AUTO_TEST_CASE(parseToScaledInt_test) {

  BOOST_REQUIRE_EQUAL(parseToScaledInt<int64_t>(std::string{"123.456"}, 3),
                      123456);
  BOOST_REQUIRE_EQUAL(parseToScaledInt<int64_t>(std::string{"-123.456"}, 3),
                      -123456);
  BOOST_REQUIRE_EQUAL(parseToScaledInt<int64_t>(std::string{"123.456"}, 6),
                      123456000);
  BOOST_REQUIRE_EQUAL(parseToScaledInt<int64_t>(std::string{"-123.456"}, 6),
                      -123456000);
  BOOST_REQUIRE_EQUAL(parseToScaledInt<int64_t>(std::string{".1234567"}, 6),
                      123456);
  BOOST_REQUIRE_EQUAL(parseToScaledInt<int64_t>(std::string{"-.1234567"}, 6),
                      -123456);
}

BOOST_AUTO_TEST_CASE(scaledIntToString_test) {
  char buffer[18] = {0};

  BOOST_REQUIRE_EQUAL(scaledIntToString(123456, 3, buffer, 6), "123.456");
  BOOST_REQUIRE_EQUAL(scaledIntToString(-123456, 3, buffer, 6), "-123.456");
  BOOST_REQUIRE_EQUAL(scaledIntToString(123456000, 6, buffer), "123.456");
  BOOST_REQUIRE_EQUAL(scaledIntToString(-123456000, 6, buffer), "-123.456");
  BOOST_REQUIRE_EQUAL(scaledIntToString(1234567, 6, buffer, 5), "1.23456");
  BOOST_REQUIRE_EQUAL(scaledIntToString(-1234567, 6, buffer, 5), "-1.23456");

  //
  BOOST_REQUIRE_EQUAL(scaledIntToString(234567, 6, buffer), "0.234567");
  BOOST_REQUIRE_EQUAL(scaledIntToString(-234567, 6, buffer), "-0.234567");

  BOOST_REQUIRE_EQUAL(scaledIntToString(234567, 7, buffer), "0.0234567");
  BOOST_REQUIRE_EQUAL(scaledIntToString(-234567, 7, buffer), "-0.0234567");

  BOOST_REQUIRE_EQUAL(scaledIntToString(234567, 9, buffer), "0.000234567");
  BOOST_REQUIRE_EQUAL(scaledIntToString(-234567, 9, buffer), "-0.000234567");
  BOOST_REQUIRE_EQUAL(scaledIntToString(123456789, 6, buffer, 0), "123");
  BOOST_REQUIRE_EQUAL(scaledIntToString(0, 9, buffer), "0");
}

BOOST_AUTO_TEST_CASE(scaledIntToString_test_failures_updates) {
  char buffer[18] = {0};
  BOOST_REQUIRE_EQUAL(
      scaledIntToString(parseToScaledInt<int64_t>(std::string{"62000.0"}, 9), 9,
                        buffer, 8),
      "62000");
  BOOST_REQUIRE_EQUAL(
      scaledIntToString(parseToScaledInt<int64_t>(std::string{"60000.0"}, 9), 9,
                        buffer, 8),
      "60000");
  BOOST_REQUIRE_EQUAL(
      scaledIntToString(parseToScaledInt<int64_t>(std::string{"60000.01"}, 9),
                        9, buffer, 8),
      "60000.01");
  BOOST_REQUIRE_EQUAL(
      scaledIntToString(parseToScaledInt<int64_t>(std::string{"0.001"}, 9), 9,
                        buffer, 3),
      "0.001");
  BOOST_REQUIRE_EQUAL(
      scaledIntToString(parseToScaledInt<int64_t>(std::string{"0.00001"}, 9), 9,
                        buffer),
      "0.00001");
  BOOST_REQUIRE_EQUAL(
      scaledIntToString(parseToScaledInt<int64_t>(std::string{"60000.1234"}, 9),
                        9, buffer, 3),
      "60000.123");
  BOOST_REQUIRE_EQUAL(
      scaledIntToString(parseToScaledInt<int64_t>(std::string{"60000.1234"}, 9),
                        9, buffer, 4),
      "60000.1234");
}

BOOST_AUTO_TEST_CASE(exponentNotationTests) {
  char buffer[18] = {0};
  BOOST_REQUIRE_EQUAL(
      scaledIntToString(parseToScaledInt<int64_t>(std::string_view{"6.2e4"}, 9),
                        9, buffer, 8),
      "62000");
  BOOST_REQUIRE_EQUAL(scaledIntToString(parseToScaledInt<int64_t>(
                                            std::string_view{"2.34567e-4"}, 9),
                                        9, buffer, 9),
                      "0.000234567");
}

BOOST_AUTO_TEST_SUITE_END();





