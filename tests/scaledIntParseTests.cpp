#include "moped/ScaledInteger.hpp"
#include <catch2/catch.hpp>
#include <format>

#define ASSERT_EQ(A, B) REQUIRE(A == B)

using namespace moped;

TEST_CASE("floatToScaledInt tests", "[ScaledInteger]") {
  ASSERT_EQ(floatToScaledInt<int64_t>(1.0, 1), 10);
  ASSERT_EQ(floatToScaledInt<int64_t>(1.0, 2), 100);
  ASSERT_EQ(floatToScaledInt<int64_t>(1.0, 3), 1000);
  ASSERT_EQ(floatToScaledInt<int64_t>(1.0, 9), 1000000000);

  ASSERT_EQ(floatToScaledInt<int64_t>(1.1, 1), 11);
  ASSERT_EQ(floatToScaledInt<int64_t>(1.1, 2), 110);
  ASSERT_EQ(floatToScaledInt<int64_t>(1.1, 3), 1100);
  ASSERT_EQ(floatToScaledInt<int64_t>(1.1, 9), 1100000000);

  ASSERT_EQ(floatToScaledInt<int64_t>(1.001, 1), 10);
  ASSERT_EQ(floatToScaledInt<int64_t>(1.001, 2), 100);
  ASSERT_EQ(floatToScaledInt<int64_t>(1.001, 3), 1001);
  ASSERT_EQ(floatToScaledInt<int64_t>(1.001, 9), 1001000000);

  ASSERT_EQ(floatToScaledInt<int64_t>(0.001, 1), 0);
  ASSERT_EQ(floatToScaledInt<int64_t>(0.001, 2), 0);
  ASSERT_EQ(floatToScaledInt<int64_t>(0.001, 3), 1);
  ASSERT_EQ(floatToScaledInt<int64_t>(0.001, 9), 1000000);

  ASSERT_EQ(0, floatToScaledInt<int64_t>(0., 9));

  // TODO: figure out how to do parameterised unit tests in boost test
  {
    int64_t expectedScaledIntVal = 999999999000000000;
    double expectedDoubleVal = 999999999.;
    ASSERT_EQ(expectedScaledIntVal,
              floatToScaledInt<int64_t>(expectedDoubleVal, 9));
    ASSERT_EQ(expectedDoubleVal,
              scaledIntToFloat<double>(expectedScaledIntVal, 9));
  }
  {
    int64_t expectedScaledIntVal = -999999999000000000;
    double expectedDoubleVal = -999999999.;
    ASSERT_EQ(expectedScaledIntVal,
              floatToScaledInt<int64_t>(expectedDoubleVal, 9));
    ASSERT_EQ(expectedDoubleVal,
              scaledIntToFloat<double>(expectedScaledIntVal, 9));
  }
  {
    int64_t expectedScaledIntVal = 999999999;
    double expectedDoubleVal = .999999999;
    ASSERT_EQ(expectedScaledIntVal,
              floatToScaledInt<int64_t>(expectedDoubleVal, 9));
    ASSERT_EQ(expectedDoubleVal,
              scaledIntToFloat<double>(expectedScaledIntVal, 9));
  }
  {
    int64_t expectedScaledIntVal = -999999999;
    double expectedDoubleVal = -.999999999;
    ASSERT_EQ(expectedScaledIntVal,
              floatToScaledInt<int64_t>(expectedDoubleVal, 9));
    ASSERT_EQ(expectedDoubleVal,
              scaledIntToFloat<double>(expectedScaledIntVal, 9));
  }
}

TEST_CASE("parseToScaledInt tests", "[ScaledInteger]") {
  ASSERT_EQ(parseToScaledInt<int64_t>(std::string{"123.456"}, 3), 123456);
  ASSERT_EQ(parseToScaledInt<int64_t>(std::string{"-123.456"}, 3), -123456);
  ASSERT_EQ(parseToScaledInt<int64_t>(std::string{"123.456"}, 6), 123456000);
  ASSERT_EQ(parseToScaledInt<int64_t>(std::string{"-123.456"}, 6), -123456000);
  ASSERT_EQ(parseToScaledInt<int64_t>(std::string{".1234567"}, 6), 123456);
  ASSERT_EQ(parseToScaledInt<int64_t>(std::string{"-.1234567"}, 6), -123456);
}

TEST_CASE("scaledIntToString tests", "[ScaledInteger]") {
  char buffer[18] = {0};
  ASSERT_EQ(scaledIntToString(123456, 3, buffer, 6),
            std::string_view{"123.456"});
  ASSERT_EQ(scaledIntToString(-123456, 3, buffer, 6),
            std::string_view{"-123.456"});
  ASSERT_EQ(scaledIntToString(123456000, 6, buffer),
            std::string_view{"123.456"});
  ASSERT_EQ(scaledIntToString(-123456000, 6, buffer),
            std::string_view{"-123.456"});
  ASSERT_EQ(scaledIntToString(1234567, 6, buffer, 5),
            std::string_view{"1.23456"});
  ASSERT_EQ(scaledIntToString(-1234567, 6, buffer, 5),
            std::string_view{"-1.23456"});
  ASSERT_EQ(scaledIntToString(234567, 6, buffer), std::string_view{"0.234567"});
  ASSERT_EQ(scaledIntToString(-234567, 6, buffer),
            std::string_view{"-0.234567"});
  ASSERT_EQ(scaledIntToString(234567, 7, buffer),
            std::string_view{"0.0234567"});
  ASSERT_EQ(scaledIntToString(-234567, 7, buffer),
            std::string_view{"-0.0234567"});
  ASSERT_EQ(scaledIntToString(234567, 9, buffer),
            std::string_view{"0.000234567"});
  ASSERT_EQ(scaledIntToString(-234567, 9, buffer),
            std::string_view{"-0.000234567"});
  ASSERT_EQ(scaledIntToString(123456789, 6, buffer, 0),
            std::string_view{"123"});
  ASSERT_EQ(scaledIntToString(0, 9, buffer), std::string_view{"0"});
}

TEST_CASE("scaledIntToString failures/updates", "[ScaledInteger]") {
  char buffer[18] = {0};
  ASSERT_EQ(
      scaledIntToString(parseToScaledInt<int64_t>(std::string{"62000.0"}, 9), 9,
                        buffer, 8),
      std::string_view{"62000"});
  ASSERT_EQ(
      scaledIntToString(parseToScaledInt<int64_t>(std::string{"60000.0"}, 9), 9,
                        buffer, 8),
      std::string_view{"60000"});
  ASSERT_EQ(
      scaledIntToString(parseToScaledInt<int64_t>(std::string{"60000.01"}, 9),
                        9, buffer, 8),
      std::string_view{"60000.01"});
  ASSERT_EQ(
      scaledIntToString(parseToScaledInt<int64_t>(std::string{"0.001"}, 9), 9,
                        buffer, 3),
      std::string_view{"0.001"});
  ASSERT_EQ(
      scaledIntToString(parseToScaledInt<int64_t>(std::string{"0.00001"}, 9), 9,
                        buffer),
      std::string_view{"0.00001"});
  ASSERT_EQ(
      scaledIntToString(parseToScaledInt<int64_t>(std::string{"60000.1234"}, 9),
                        9, buffer, 3),
      std::string_view{"60000.123"});
  ASSERT_EQ(
      scaledIntToString(parseToScaledInt<int64_t>(std::string{"60000.1234"}, 9),
                        9, buffer, 4),
      std::string_view{"60000.1234"});
}

TEST_CASE("exponentNotationTests", "[ScaledInteger]") {
  char buffer[18] = {0};
  ASSERT_EQ(
      scaledIntToString(parseToScaledInt<int64_t>(std::string_view{"6.2e4"}, 9),
                        9, buffer, 8),
      std::string_view{"62000"});
  ASSERT_EQ(scaledIntToString(
                parseToScaledInt<int64_t>(std::string_view{"2.34567e-4"}, 9), 9,
                buffer, 9),
            std::string_view{"0.000234567"});

  ASSERT_EQ(parseToScaledInt<int64_t>(std::string{"6.2e4"}, 0), 62000);
  ASSERT_EQ(parseToScaledInt<int64_t>(std::string{"-6.2e4"}, 0), -62000);
  ASSERT_EQ(parseToScaledInt<int64_t>(std::string{"2.34567e-4"}, 9), 234567);
  ASSERT_EQ(parseToScaledInt<int64_t>(std::string{"-2.34567e-4"}, 9), -234567);
}