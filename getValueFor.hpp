#pragma once

#include "ScaledInteger.hpp"
#include "moped/concepts.hpp"
#include <charconv>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace moped {

template <typename NumericT>
inline std::expected<NumericT, ParseError> ston(std::string_view value) {
  NumericT result;
  auto [ptr, ec] =
      std::from_chars(value.data(), value.data() + value.size(), result);
  if (ec != std::errc{}) {
    return std::unexpected{ParseError{"Failed to convert value to numeric type",
                                      std::format("{}", value)}};
  }
  return result;
}

template <typename TargetT, typename DecodingTraits>
std::expected<TargetT, ParseError> getValueFor(std::string_view value) {
  if constexpr (std::is_same_v<TargetT, bool>) {
    if (value == "true" || value == "1") {
      return true;
    } else if (value == "false" || value == "0") {
      return false;
    } else {
      return std::unexpected(ParseError{"Invalid boolean value", value});
    }
  } else if constexpr (std::is_same_v<TargetT, TimePoint>) {
    auto epochTime = DecodingTraits::TimePointFormaterT::getTimeValue(value);
    if (!epochTime) {
      return std::unexpected{epochTime.error()};
    }
    return epochTime.value();
  } else if constexpr (std::is_same_v<TargetT, std::string> ||
                       std::is_same_v<TargetT, std::string_view>) {
    return TargetT{value};
  } else if constexpr (std::is_same_v<TargetT, const char *>) {
    return value.data();
  } else if constexpr ((std::is_integral_v<TargetT>) ||
                       (std::is_floating_point_v<TargetT>)) {
    return ston<TargetT>(value);
  } else if constexpr (IsOptionalC<TargetT, DecodingTraits>) {
    auto result =
        getValueFor<typename TargetT::value_type, DecodingTraits>(value);
    if (!result) {
      return std::unexpected(result.error());
    }
    return result.value();
  } else if constexpr (is_scaled_int<TargetT>) {
    return TargetT{value};
  } else if constexpr (is_mapped_enum<TargetT>) {
    try {
      return TargetT{value};
    } catch (const std::exception &e) {
      return std::unexpected{ParseError{
          "Failed conversion ",
          std::format("from '{}' to mapped enum: {}", value, e.what())}};
    }
  } else {
    return std::unexpected(ParseError{"Unsupported type for value conversion",
                                      std::string(typeid(TargetT).name())});
  }
}

template <typename TargetT, typename DecodingTraits>
std::expected<bool, ParseError> getValueFor(bool value) {
  return value;
}

} // namespace moped