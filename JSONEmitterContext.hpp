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
    if (_positionNested.size() > 0) {
      _output << "{";
    }
    _positionNested.push(0);
  }

  void onObjectFinish() {
    _positionNested.pop();
    if (_positionNested.empty() && _publishingRootArray) {
      _publishingRootArray = false;
      return;
    }
    _output << "}";
    if (!_positionNested.empty()) {
      _positionNested.top()++;
    }
  }

  // Implementation for starting a JSON array
  void onArrayStart(std::optional<std::string_view> memberId) {
    if ((_positionNested.size() == 1) && (_positionNested.top() == 0)) {
      if ((memberId.has_value()) && (*memberId == "")) {
        _publishingRootArray = true;
        _output << "[";
        _positionNested.push(0);
        return;
      } else {
        _output << "{";
      }
    }
    if (!_positionNested.empty() && (_positionNested.top() > 0)) {
      _output << ",";
    }
    if (memberId) {
      _output << "\"" << *memberId << "\": ";
    }
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
    if (!_positionNested.empty()) {
      _positionNested.top()++;
    }
  }

  void onObjectValueEntry(const std::string_view memberId, const auto &value) {
    if ((_positionNested.size() == 1) && (_positionNested.top() == 0)) {
      _output << "{";
    }
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
    if ((_positionNested.size() == 1) && (_positionNested.top() == 0)) {
      _output << "{";
    }
    if (!value.has_value()) {
      return; // Skip null values
    }
    onObjectValueEntry(memberId, value.value());
  }
  // Implementation for a JSON member

private:
  void writeJSONEncodedValue(moped::TimePoint value) {
    _output << "\"" << TimePointFormatter::format(value) << "\"";
  }

  template <is_allowed_itegral I, std::uint8_t Scale10V>
  void writeJSONEncodedValue(const ScaledInteger<I, Scale10V> &value) {
    _output << value.toString();
  }

  std::string escapeJSONString(std::string_view input) {
    std::string escaped;
    for (char c : input) {
      switch (c) {
      case '\"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\b':
        escaped += "\\b";
        break;
      case '\f':
        escaped += "\\f";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          escaped += "\\u" + std::to_string(static_cast<int>(c));
        } else {
          escaped += c;
        }
      }
    }
    return escaped;
  }

  template <typename T> void writeJSONEncodedValue(const T &value) {
    if constexpr (std::is_same_v<T, bool>) {
      _output << (value ? "true" : "false");
    } else if constexpr (std::is_arithmetic_v<T>) {
      _output << value;
    } else {
      if constexpr (std::convertible_to<T, std::string_view>) {
        _output << "\"" << escapeJSONString(value) << "\"";
      } else {
        _output << "\"" << value << "\"";
      }
    }
  }

  bool _publishingRootArray{false};
  std::ostream &_output;
  std::stack<std::uint32_t> _positionNested;
};

template <typename T, typename... FormatArgs>
void encodeToJSONStream(const T &mopedObject, std::ostream &output,
                        FormatArgs...) {
  JSONEmitterContext<FormatArgs...> context(output);
  auto handler =
      getMOPEDHandlerForParser<T, StringDecodingTraits<FormatArgs...>>();
  handler.setTargetMember(const_cast<T &>(mopedObject));
  handler.applyEmitterContext(context);
}

template <typename T, typename... FormatArgs>
std::string encodeToJSONString(const T &mopedObject, FormatArgs... args) {
  std::ostringstream oss;
  encodeToJSONStream(mopedObject, oss, std::forward<FormatArgs>(args)...);
  return oss.str();
}

} // namespace moped