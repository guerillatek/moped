#pragma once
#include "concepts.hpp"
#include "moped/MappedObjectParseEncoderDispatcher.hpp"
#include "moped/ScaledInteger.hpp"
#include "moped/TimeFormatters.hpp"
#include "moped/concepts.hpp"
#include "moped/moped.hpp"
#include <chrono>
#include <concepts>
#include <cstdint>
#include <format>
#include <iostream>
#include <optional>
#include <ostream>
#include <stack>
#include <string>
#include <string_view>

namespace moped {

template <typename TimePointFormatter = DurationSinceEpochFormatter<>>
class JSONEmitterContext {
public:
  JSONEmitterContext(std::ostream &output) : _output(output) {}
  // Implementation for starting a JSON object
  void onObjectStart(std::optional<std::string_view> memberId = std::nullopt) {
    if (!_positionNested.empty() && (_positionNested.top() > 0)) {
      _output << ",";
    }
    if (memberId) {
      _output << "\"" << *memberId << "\": ";
    }
    _output << "{";
    _positionNested.push(0);
  }

  void onObjectFinish() {
    _output << "}";
    _positionNested.pop();
    if (!_positionNested.empty()) {
      _positionNested.top()++;
    }
  }

  // Implementation for starting a JSON array
  void onArrayStart(std::string_view memberId, size_t) {
    if (!_positionNested.empty() && (_positionNested.top() > 0)) {
      _output << ",";
    }
    _output << "\"" << memberId << "\": ";
    _output << "[";
    _positionNested.push(0);
  }

  void onArrayValueEntry(const auto &value) {
    if (_positionNested.top() > 0) {
      _output << ",";
    }
    _positionNested.top()++;
    writeJSONEncodedValue(value);
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
    writeJSONEncodedValue(value);
  }

  template <typename T>
  void onObjectValueEntry(const std::string_view memberId,
                          const std::optional<T> &value) {
    if (!value.has_value()) {
      return; // Skip null values
    }
    onObjectValueEntry(memberId, value.value());
  }

  void onObjectFinished() {
    _output << "}";
    _positionNested.pop();
  }

  // Implementation for a JSON member

private:
  void writeJSONEncodedValue(moped::TimePoint value) {
    _output << "\"" << TimePointFormatter::format(value) << "\"";
  }

  template <std::integral I, std::uint8_t Scale10V>
  void writeJSONEncodedValue(const ScaledInteger<I, Scale10V> &value) {
    _output << value.toString();
  }

  void writeJSONEncodedValue(const auto &value) {
    if constexpr (std::is_arithmetic_v<std::decay_t<decltype(value)>>) {
      _output << value;
    } else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, bool>) {
      _output << (value ? "true" : "false");
    } else {
      _output << "\"" << value << "\"";
    }
  }

  std::ostream &_output;
  std::stack<std::uint32_t> _positionNested;
};

template <typename T, typename... FormatArgs>
void encodeToJSONStream(const T &mopedObject, std::ostream &output,
                        FormatArgs... args) {
  JSONEmitterContext<FormatArgs...> context(output);
  auto handler =
      getMOPEDHandlerForParser<T, StringMemberIdTraits<FormatArgs...>>();
  handler.setTargetMember(const_cast<T &>(mopedObject));
  handler.applyEmitterContext(context);
}

template <typename T, typename... FormatArgs>
std::string encodeToJSONString(const T &mopedObject) {
  std::ostringstream oss;
  encodeToJSONStream<FormatArgs...>(mopedObject, oss);
  return oss.str();
}

} // namespace moped