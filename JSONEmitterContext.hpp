#pragma once
#include "moped/concepts.hpp"

#include <concepts>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stack>
#include <string_view>

namespace moped {


template <typename TimePointFormatter = DefaultTimePointFormatter<>>
class JSONEmitterContext {
public:
  JSONEmitterContext(std::ostream &output) : _output(output) {}
  // Implementation for starting a JSON object
  void onObjectStart(std::optional<std::string_view> memberId = std::nullopt) {
    if (memberId) {
      _output << "\"" << *memberId << "\": ";
    }
    _output << "{";
    _positionNested.push(0);
  }

  void onObjectFinish() {
    _output << "}";
    _positionNested.pop();
  }

  // Implementation for starting a JSON array
  void onArrayStart(std::string_view memberId, size_t) {
    _output << "\"" << memberId << "\": ";
    _output << "[";
    _positionNested.push(0);
  }

  void onArrayValueEntry(const auto &value) {
    if (_positionNested.top() > 0) {
      _output << ",";
    }
    _positionNested.top()++;
    _output << getJSONEncodeValue(value);
  }

  void onArrayFinish() {
    _output << "]";
    _positionNested.pop();
  }

  void onObjectValueEntry(const std::string_view memberId, const auto &value) {
    if (_positionNested.top() > 0) {
      _output << ",";
    }
    _positionNested.top()++;
    _output << "\"" << memberId << "\": ";
    writeJSONEncodeValue(value);
  }

  void onObjectFinished() {
    _output << "}";
    _positionNested.pop();
  }

  // Implementation for a JSON member

private:
  void writeJSONEncodeValue(moped::TimePoint value) {
    _output << TimePointFormatter::format(value);
  }

  template <std::integral I, std::uint8_t Scale10V>
  void writeJSONEncodeValue(const ScaledInteger<I, Scale10V> &value) {
    _output << value.toString();
  }

  void writeJSONEncodeValue(const auto &value) {
    if constexpr (std::is_same_v<std::decay_t<decltype(value)>,
                                 std::string_view>) {
      _output << "\"" << valloc << "\"";
    } else if constexpr (std::is_arithmetic_v<std::decay_t<decltype(value)>>) {
      _output << value;
    } else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, bool>) {
      _output << (value ? "true" : "false");
    } else {
      static_assert(always_false<decltype(value)>,
                    "Unsupported type for JSON encoding");
    }
  }

  std::ostream &_output;
  std::stack<std::uint32_t> _positionNested;
};

} // namespace moped