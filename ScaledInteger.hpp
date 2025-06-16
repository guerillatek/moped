#pragma once
#include "concepts.hpp"
#include "scaledIntParseUtils.hpp"
#include <format>

namespace moped {


} // namespace moped

template <moped::is_scaled_int T> struct std::formatter<T> {

  constexpr auto parse(auto &ctx) { return ctx.begin(); }

  auto format(const T &scaledInt, auto &ctx) const {
    char buffer[20];
    moped::scaledIntToString(scaledInt.getRawIntegerValue(), T::Scale, buffer);
    return std::format_to(ctx.out(), "{}",buffer);
  }
};

namespace moped {

template <std::integral I, std::uint8_t Scale10V> class ScaledInteger {
public:
  using IntegralT = I;

  static constexpr auto Scale = Scale10V;
  static constexpr auto Divisor = scale10(Scale);


  ScaledInteger(std::string_view input)
      : _rawIntegerValue{parseToScaledInt<I>(input, Scale)} {}

  template <is_scaled_int S> explicit ScaledInteger(const S &src) {
    if (S::Scale > Scale) {
      _rawIntegerValue = src._rawIntegerValue / scale10(S::Scale - Scale);
    } else {
      _rawIntegerValue = src._rawIntegerValue * scale10(Scale - S::Scale);
    }
  }

  // Numeric conversions represent potential loss of information
  template <std::integral It> It toInteger() const {
    return static_cast<It>(_rawIntegerValue) / Scale;
  }

  template <std::floating_point Ft> Ft toFloat() const {
    return static_cast<Ft>(_rawIntegerValue) / scale10(Scale);
  }

  auto toString() const { return std::format("{}", *this); }

  auto getRawIntegerValue() const { return _rawIntegerValue; }

  bool operator>(const ScaledInteger &rhs) const {
    return _rawIntegerValue > rhs._rawIntegerValue;
  }

  bool operator<(const ScaledInteger &rhs) const {
    return _rawIntegerValue < rhs._rawIntegerValue;
  }

  bool operator==(const ScaledInteger &rhs) const {
    return _rawIntegerValue == rhs._rawIntegerValue;
  }

  ScaledInteger operator+(const ScaledInteger &rhs) const {
    return ScaledInteger{_rawIntegerValue + rhs._rawIntegerValue};
  }

  ScaledInteger operator-(const ScaledInteger &rhs) const {
    return ScaledInteger{_rawIntegerValue - rhs._rawIntegerValue};
  }

  ScaledInteger operator*(const ScaledInteger &rhs) const {
      return ScaledInteger{_rawIntegerValue * rhs.getRawIntegerValue()/scale10(Scale)};
  }

  ScaledInteger operator/(const ScaledInteger &rhs) const {
    // Avoids potential divide by zero errors
    return ScaledInteger{_rawIntegerValue * scale10(Scale) / rhs.getRawIntegerValue()};

  }


friend std::ostream &operator<<(std::ostream &os,
                                  const ScaledInteger &scaledInt) {
    char buffer[20];
    scaledIntToString(scaledInt._rawIntegerValue, Scale, buffer);
    return os << std::string_view{static_cast<const char*>(buffer)};
  }

  friend std::string to_string(const ScaledInteger &scaledInt) {
    return scaledInt.toString();
  }

private:
  ScaledInteger(I rawIntegerValue) : _rawIntegerValue{rawIntegerValue} {}

  IntegralT _rawIntegerValue;
};

} // namespace moped
