#include "moped/TimeFormatters.hpp"
#include <catch2/catch.hpp>
#include <chrono>
#include <string>

using namespace moped;
using namespace std::chrono;

TEST_CASE("DurationSinceEpochFormatter - format",
          "[TimeFormatters][DurationSinceEpochFormatter]") {
  SECTION("Format milliseconds") {
    using Formatter = DurationSinceEpochFormatter<milliseconds>;

    // January 1, 2023 00:00:00 UTC = 1672531200000 milliseconds
    auto timePoint = system_clock::time_point(milliseconds(1672531200000));
    auto result = Formatter::format(timePoint);
    REQUIRE(result == "1672531200000");
  }

  SECTION("Format seconds") {
    using Formatter = DurationSinceEpochFormatter<seconds>;

    auto timePoint = system_clock::time_point(seconds(1672531200));
    auto result = Formatter::format(timePoint);
    REQUIRE(result == "1672531200");
  }

  SECTION("Format microseconds") {
    using Formatter = DurationSinceEpochFormatter<microseconds>;

    auto timePoint = system_clock::time_point(microseconds(1672531200123456));
    auto result = Formatter::format(timePoint);
    REQUIRE(result == "1672531200123456");
  }
}

TEST_CASE("DurationSinceEpochFormatter - getTimeValue from integer",
          "[TimeFormatters][DurationSinceEpochFormatter]") {
  SECTION("Parse integer milliseconds") {
    using Formatter = DurationSinceEpochFormatter<milliseconds>;

    auto result = Formatter::getTimeValue(1672531200000);
    REQUIRE(result.has_value());
    REQUIRE(result->time_since_epoch() == milliseconds(1672531200000));
  }

  SECTION("Parse integer seconds") {
    using Formatter = DurationSinceEpochFormatter<seconds>;

    auto result = Formatter::getTimeValue(1672531200);
    REQUIRE(result.has_value());
    REQUIRE(result->time_since_epoch() == seconds(1672531200));
  }
}

TEST_CASE("DurationSinceEpochFormatter - getTimeValue from string",
          "[TimeFormatters][DurationSinceEpochFormatter]") {
  SECTION("Parse valid numeric string") {
    using Formatter = DurationSinceEpochFormatter<milliseconds>;

    auto result = Formatter::getTimeValue(std::string("1672531200000"));
    REQUIRE(result.has_value());
    // Should interpret as milliseconds based on digit count (13 digits)
  }

  SECTION("Parse seconds (10 digits)") {
    using Formatter = DurationSinceEpochFormatter<milliseconds>;

    auto result = Formatter::getTimeValue(std::string("1672531200"));
    REQUIRE(result.has_value());
    // Should interpret as seconds
  }

  SECTION("Parse milliseconds (13 digits)") {
    using Formatter = DurationSinceEpochFormatter<milliseconds>;

    auto result = Formatter::getTimeValue(std::string("1672531200123"));
    REQUIRE(result.has_value());
    // Should interpret as milliseconds
  }

  SECTION("Invalid input - empty string") {
    using Formatter = DurationSinceEpochFormatter<milliseconds>;

    auto result = Formatter::getTimeValue(std::string(""));
    REQUIRE(!result.has_value());
    REQUIRE(result.error().find("Invalid input") != std::string::npos);
  }

  SECTION("Invalid input - non-numeric") {
    using Formatter = DurationSinceEpochFormatter<milliseconds>;

    auto result = Formatter::getTimeValue(std::string("abc123"));
    REQUIRE(!result.has_value());
    REQUIRE(result.error().find("Invalid input") != std::string::npos);
  }

  SECTION("Invalid input - too many digits") {
    using Formatter = DurationSinceEpochFormatter<milliseconds>;

    auto result =
        Formatter::getTimeValue(std::string("12345678901234567890123"));
    REQUIRE(!result.has_value());
    REQUIRE(result.error().find("too many digits") != std::string::npos);
  }
}

TEST_CASE("ISO8601Formatter - getTimeValue",
          "[TimeFormatters][ISO8601Formatter]") {
  SECTION("Parse basic ISO8601 without fractional seconds") {
    using Formatter = ISO8601Formatter<seconds>;

    auto result = Formatter::getTimeValue(std::string("2023-01-01T00:00:00Z"));
    REQUIRE(result.has_value());

    // Convert back to verify
    auto timeT = system_clock::to_time_t(*result);
    auto tm = *std::gmtime(&timeT);
    REQUIRE(tm.tm_year == 123); // years since 1900
    REQUIRE(tm.tm_mon == 0);    // January
    REQUIRE(tm.tm_mday == 1);
  }

  SECTION("Parse ISO8601 with milliseconds") {
    using Formatter = ISO8601Formatter<milliseconds>;

    auto result =
        Formatter::getTimeValue(std::string("2023-01-01T12:30:45.123Z"));
    REQUIRE(result.has_value());

    // Check fractional part
    auto ms =
        duration_cast<milliseconds>(result->time_since_epoch()).count() % 1000;
    REQUIRE(ms == 123);
  }

  SECTION("Parse ISO8601 with microseconds") {
    using Formatter = ISO8601Formatter<microseconds>;

    auto result =
        Formatter::getTimeValue(std::string("2023-01-01T12:30:45.123456Z"));
    REQUIRE(result.has_value());

    auto us = duration_cast<microseconds>(result->time_since_epoch()).count() %
              1000000;
    REQUIRE(us == 123456);
  }

  SECTION("Parse ISO8601 with timezone offset") {
    using Formatter = ISO8601Formatter<seconds>;

    auto result =
        Formatter::getTimeValue(std::string("2023-01-01T05:00:00-05:00"));
    REQUIRE(result.has_value());

    // Should be equivalent to 10:00:00 UTC
    auto timeT = system_clock::to_time_t(*result);
    auto tm = *std::gmtime(&timeT);
    REQUIRE(tm.tm_hour == 10);
  }

  SECTION("Invalid format") {
    using Formatter = ISO8601Formatter<milliseconds>;

    auto result = Formatter::getTimeValue(std::string("invalid-date"));
    REQUIRE(!result.has_value());
    REQUIRE(result.error().find("Invalid ISO8601") != std::string::npos);
  }

  SECTION("Too many fractional digits") {
    using Formatter = ISO8601Formatter<milliseconds>; // expects max 3 digits

    auto result =
        Formatter::getTimeValue(std::string("2023-01-01T12:30:45.123456Z"));
    REQUIRE(!result.has_value());
    REQUIRE(result.error().find("Too many fractional digits") !=
            std::string::npos);
  }

  SECTION("Fractional seconds not allowed for seconds precision") {
    using Formatter = ISO8601Formatter<seconds>; // expects 0 fractional digits

    auto result =
        Formatter::getTimeValue(std::string("2023-01-01T12:30:45.1Z"));
    REQUIRE(!result.has_value());
    REQUIRE(result.error().find("not allowed") != std::string::npos);
  }
}

TEST_CASE("ISO8601GMTFormatter - format",
          "[TimeFormatters][ISO8601GMTFormatter]") {
  SECTION("Format with milliseconds") {
    using Formatter = ISO8601GMTFormatter<milliseconds>;

    // Create a specific time point
    auto tp = system_clock::time_point(seconds(1672531200) + milliseconds(123));
    auto result = Formatter::format(tp);

    REQUIRE(result.starts_with("2023-01-01T00:00:00"));
    REQUIRE(result.ends_with("123Z"));
    REQUIRE(result.find(".") != std::string::npos);
  }

  SECTION("Format with seconds") {
    using Formatter = ISO8601GMTFormatter<seconds>;

    auto tp = system_clock::time_point(seconds(1672531200));
    auto result = Formatter::format(tp);

    REQUIRE(result.starts_with("2023-01-01T00:00:00"));
    REQUIRE(result.ends_with("0Z"));
  }

  SECTION("Format with microseconds") {
    using Formatter = ISO8601GMTFormatter<microseconds>;

    auto tp =
        system_clock::time_point(seconds(1672531200) + microseconds(123456));
    auto result = Formatter::format(tp);

    REQUIRE(result.starts_with("2023-01-01T00:00:00"));
    REQUIRE(result.ends_with("123456Z"));
  }
}

TEST_CASE("ISO8601LocalFormatter - format",
          "[TimeFormatters][ISO8601LocalFormatter]") {
  SECTION("Format includes timezone offset") {
    using Formatter = ISO8601LocalFormatter<milliseconds>;

    auto tp = system_clock::time_point(seconds(1672531200) + milliseconds(123));
    auto result = Formatter::format(tp);

    REQUIRE(result.find(".123") != std::string::npos);
    REQUIRE(!result.ends_with("Z")); // Should not end with Z

    // Should contain timezone offset (+ or -)
    bool hasOffset = result.find('+') != std::string::npos ||
                     result.find('-') != std::string::npos;
    REQUIRE(hasOffset);

    // Should have colon in timezone offset
    size_t lastColon = result.rfind(':');
    REQUIRE(lastColon != std::string::npos);
    // The colon should be in the last 6 characters (for timezone offset)
    REQUIRE(lastColon >= result.length() - 6);
  }
}

TEST_CASE("Formatter inheritance and polymorphism", "[TimeFormatters]") {
  SECTION("ISO8601GMTFormatter inherits getTimeValue") {
    using Formatter = ISO8601GMTFormatter<milliseconds>;

    auto result =
        Formatter::getTimeValue(std::string("2023-01-01T12:30:45.123Z"));
    REQUIRE(result.has_value());

    // Should be able to format what it parsed
    auto formatted = Formatter::format(*result);
    REQUIRE(formatted.find("12:30:45") != std::string::npos);
    REQUIRE(formatted.find("123Z") != std::string::npos);
  }

  SECTION("ISO8601LocalFormatter inherits getTimeValue") {
    using Formatter = ISO8601LocalFormatter<milliseconds>;

    auto result =
        Formatter::getTimeValue(std::string("2023-01-01T12:30:45.123Z"));
    REQUIRE(result.has_value());

    // Should be able to format what it parsed (but with local timezone)
    auto formatted = Formatter::format(*result);
    REQUIRE(formatted.find("123") != std::string::npos);
    REQUIRE(!formatted.ends_with("Z"));
  }
}

TEST_CASE("Edge cases and error conditions", "[TimeFormatters]") {
  SECTION("DurationSinceEpochFormatter with zero") {
    using Formatter = DurationSinceEpochFormatter<milliseconds>;

    auto result = Formatter::getTimeValue(0);
    REQUIRE(result.has_value());
    REQUIRE(result->time_since_epoch() == milliseconds(0));
  }

  SECTION("ISO8601Formatter with leap year") {
    using Formatter = ISO8601Formatter<seconds>;

    auto result = Formatter::getTimeValue(std::string("2024-02-29T00:00:00Z"));
    REQUIRE(result.has_value());
  }

  SECTION("ISO8601Formatter with end of year") {
    using Formatter = ISO8601Formatter<milliseconds>;

    auto result =
        Formatter::getTimeValue(std::string("2023-12-31T23:59:59.999Z"));
    REQUIRE(result.has_value());

    auto ms =
        duration_cast<milliseconds>(result->time_since_epoch()).count() % 1000;
    REQUIRE(ms == 999);
  }
}