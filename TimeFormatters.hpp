#pragma once
#include "concepts.hpp"
#include "moped/ScaledInteger.hpp"
#include <chrono>
#include <cstring>
#include <expected>
#include <format>
#include <string>
#include <time.h>
#include <type_traits>

namespace moped {

template <typename FractionalDurationT = std::chrono::milliseconds>
class DurationSinceEpochFormatter {
public:
  static std::string format(moped::TimePoint value) {
    auto fractionalUnits = std::chrono::duration_cast<FractionalDurationT>(
        value.time_since_epoch());
    return std::to_string(fractionalUnits.count());
  }

  static std::expected<TimePoint, ParseError>
  getTimeValue(is_integral auto input) {
    using namespace std::chrono;
    return TimePoint{FractionalDurationT{input}};
  }

  static std::expected<TimePoint, ParseError> getTimeValue(auto src) {
    auto input = std::string_view(std::begin(src), std::end(src));
    // Make sure the input is numeric
    if (input.empty() || !std::all_of(input.begin(), input.end(),
                                      [](char c) { return std::isdigit(c); })) {
      return std::unexpected("Invalid input: expected a numeric string "
                             "representing time units since epoch");
    }

    // Convert string to 64-bit integer
    auto durationValue =
        ScaledInteger<std::uint64_t, 0>{input}.getRawIntegerValue();
    size_t digits = input.size();
    using namespace std::chrono;

    if (digits <= 10) {
      return time_point<system_clock, nanoseconds>(seconds(durationValue));
    } else if (digits <= 13) {
      return time_point<system_clock, nanoseconds>(milliseconds(durationValue));
    } else if (digits <= 16) {
      return time_point<system_clock, nanoseconds>(microseconds(durationValue));
    } else if (digits <= 19) {
      return time_point<system_clock, nanoseconds>(nanoseconds(durationValue));
    }
    return std::unexpected(
        "Invalid input: too many digits for a valid time point");
  }
};

template <typename FractionalDurationT = std::chrono::milliseconds>
class ISO8601Formatter {
public:
  static std::expected<TimePoint, ParseError> getTimeValue(auto src) {
    auto input = std::string_view{src};
    std::tm tm = {};
    // Parse date and time (YYYY-MM-DDTHH:MM:SS)
    char *fractAndOffsetStart =
        strptime(input.data(), "%Y-%m-%dT%H:%M:%S", &tm);
    if (fractAndOffsetStart == nullptr) {
      return std::unexpected("Invalid ISO8601 time format");
    }

    uint64_t fractionalUnits = 0;
    // Calculate expected fractional digits based on FractionalDurationT
    constexpr int expectedFractionalDigits = []() {
      constexpr auto den = FractionalDurationT::period::den;
      // Count trailing zeros in powers of 10
      // 1 = 0 digits, 1000 = 3 digits, 1000000 = 6 digits, etc.
      int digits = 0;
      auto temp = den;
      while (temp > 1 && temp % 10 == 0) {
        temp /= 10;
        digits++;
      }
      // Validate that it's a clean power of 10
      return (temp == 1) ? digits : -1; // -1 for unsupported denominators
    }();

    static_assert(expectedFractionalDigits >= 0,
                  "Unsupported FractionalDurationT - must be seconds, "
                  "milliseconds, microseconds, or nanoseconds");

    // Move to fractional part if exists
    const char *frac_start = strchr(fractAndOffsetStart, '.');
    if (frac_start) {
      frac_start++; // Skip the dot
      int frac_len = 0;

      // Count actual fractional digits in input
      while (frac_start[frac_len] >= '0' && frac_start[frac_len] <= '9') {
        frac_len++;
      }

      // Validate fractional digit count
      if constexpr (expectedFractionalDigits == 0) {
        if (frac_len > 0) {
          return std::unexpected(
              "Fractional seconds not allowed for seconds precision");
        }
      } else {
        if (frac_len > expectedFractionalDigits) {
          return std::unexpected(
              ParseError{"Too many fractional digits",
                         std::format(" expected max {}, got {}",
                                     expectedFractionalDigits, frac_len)});
        }
      }

      // Parse fractional digits
      frac_len = 0;
      while (frac_len < expectedFractionalDigits &&
             frac_start[frac_len] >= '0' && frac_start[frac_len] <= '9') {
        fractionalUnits = fractionalUnits * 10 + (frac_start[frac_len] - '0');
        frac_len++;
      }

      // Pad with zeros if fewer digits than expected
      while (frac_len < expectedFractionalDigits) {
        fractionalUnits *= 10;
        frac_len++;
      }
    }

    // Parse timezone offset if exists
    int tz_offset = 0;
    const char *tz_start =
        frac_start ? frac_start + strcspn(frac_start, "+-Z")
                   : fractAndOffsetStart + strcspn(fractAndOffsetStart, "+-Z");
    if (*tz_start == '+' || *tz_start == '-') {
      int sign = (*tz_start == '-') ? -1 : 1;
      if (strlen(tz_start) >= 6) {
        int hours = (tz_start[1] - '0') * 10 + (tz_start[2] - '0');
        int minutes = (tz_start[4] - '0') * 10 + (tz_start[5] - '0');
        tz_offset = sign * (hours * 3600 + minutes * 60);
      }
    }

    // Calculate epoch time using constexpr denominator
    constexpr auto den = FractionalDurationT::period::den;
    uint64_t seconds_since_epoch =
        static_cast<uint64_t>(timegm(&tm)) - tz_offset;
    uint64_t total_units = seconds_since_epoch * den + fractionalUnits;

    return std::chrono::system_clock::time_point(
        FractionalDurationT(total_units));
  }
};

template <typename FractionalDurationT = std::chrono::milliseconds>
class ISO8601GMTFormatter : public ISO8601Formatter<FractionalDurationT> {
public:
  static std::string format(moped::TimePoint value) {
    std::time_t timeT = std::chrono::system_clock::to_time_t(value);
    std::tm tm = *std::gmtime(&timeT);
    char buffer[30];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &tm);
    auto fractionalUnits = std::chrono::duration_cast<FractionalDurationT>(
                               value.time_since_epoch())
                               .count() %
                           FractionalDurationT::period::den;
    return std::format("{}.{}Z", buffer, fractionalUnits);
  }
};

template <typename FractionalDurationT = std::chrono::milliseconds>
class ISO8601LocalFormatter : public ISO8601Formatter<FractionalDurationT> {
public:
  static std::string format(moped::TimePoint value) {
    std::time_t timeT = std::chrono::system_clock::to_time_t(value);
    std::tm tm = *std::localtime(&timeT);
    char buffer[30];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &tm);

    auto fractionalUnits = std::chrono::duration_cast<FractionalDurationT>(
                               value.time_since_epoch())
                               .count() %
                           FractionalDurationT::period::den;

    // Get timezone offset
    char tz_buffer[10];
    std::strftime(tz_buffer, sizeof(tz_buffer), "%z", &tm);

    // Format timezone offset with colon (e.g., +05:00 instead of +0500)
    std::string tz_offset(tz_buffer);
    if (tz_offset.length() == 5 &&
        (tz_offset[0] == '+' || tz_offset[0] == '-')) {
      tz_offset.insert(3, ":");
    }

    return std::format("{}.{}{}", buffer, fractionalUnits, tz_offset);
  }
};

} // namespace moped