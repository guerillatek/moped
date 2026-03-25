#pragma once


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
using int128_t = __int128;
using uint128_t = unsigned __int128;
#pragma GCC diagnostic pop

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

namespace moped {

template <typename F>
concept FloatType = std::is_floating_point_v<F>;

constexpr int64_t smallScale10(std::uint32_t n) {
  constexpr size_t NUM_SCALE_FACTORS = 19uz;
  static constexpr std::array<std::int64_t, NUM_SCALE_FACTORS> values = {
      1,
      10,
      100,
      1000,
      10000,
      100000,
      1000000,
      10000000,
      100000000,
      1000000000,
      10000000000,
      100000000000,
      1000000000000,
      10000000000000,
      100000000000000,
      1000000000000000,
      10000000000000000,
      100000000000000000,
      1000000000000000000};

  return values[n];
}

constexpr std::array<uint128_t, 36uz> makeBigScale10Values() {
  std::array<uint128_t, 36uz> values{};
  values[0] = 1;
  for (std::size_t i = 1; i < values.size(); ++i) {
    values[i] = (values[i - 1] << 3) + (values[i - 1] << 1);
  }
  return values;
}

constexpr uint128_t bigScale10(std::uint32_t n) {
  constexpr uint128_t BIG_NUM_SCALE_FACTORS = 36uz;
  static constexpr std::array<uint128_t, BIG_NUM_SCALE_FACTORS>
      big_values = makeBigScale10Values();

  return big_values[n];
}

template <typename T> constexpr T scale10(T n) {
  if constexpr (std::is_same_v<T, int128_t> ||
                std::is_same_v<T, uint128_t>) {
    return bigScale10(n);
  } else {
    return smallScale10(n);
  }
}

template <typename T> inline T floatToScaledInt(FloatType auto v, int scale) {
  v = v * scale10<T>(scale);
  return T(std::round(v));
}

template <typename T> void fromChars(auto start, auto finish, T &value) {
  value = 0;
  while ((start != finish) && (*start >= '0') && (*start <= '9')) {
    value *= 10;
    value += *start - '0';
    ++start;
  }
}

template <typename T>
inline T parseToScaledInt(const auto &floatString, int scale) {
  auto begin = std::begin(floatString);
  auto end = std::end(floatString);
  auto decimalPos = std::find(begin, end, '.');

  T result = 0;
  bool negative{false};
  if (*begin == '-') {
    negative = true;
    ++begin;
  }

  auto cr = begin;
  while ((cr != end) && (*cr != 'e')) {
    if (*cr == 0) {
      end = cr; // true end if input was std::array
      break;
    }
    if ((*cr == '.') || (*cr == '-')) {
      ++cr;
      continue;
    }

    result += *cr - '0';
    result *= 10;
    ++cr;
  }
  if (*cr == 'e') {
    // Check for numbers with exponential notations
    auto newEndP = cr;
    ++cr;
    int expVal;
    if (*cr == '-') {
      ++cr;
      fromChars(cr, end, expVal);
      scale -= expVal;

    } else {
      fromChars(cr, end, expVal);
      result *= scale10<T>(expVal);
    }
    end = newEndP;
  }
  if (decimalPos != end) {
    scale -= (end - decimalPos - 1);
  }

  result /= 10;
  if (scale < 0) {
    return result / scale10<T>(abs(scale)) * ((negative) ? -1 : 1);
  }
  return result * scale10<T>(scale) * ((negative) ? -1 : 1);
}

template <typename T>
inline std::tuple<T, int> parseToIntFindScale(const auto &floatString) {
  int scale = 0;
  auto begin = std::begin(floatString);
  auto end = std::end(floatString);
  auto decimalPos = std::find(begin, end, '.');

  T result = 0;
  bool negative{false};
  if (*begin == '-') {
    negative = true;
    ++begin;
  }

  auto cr = begin;
  while ((cr != end) && (*cr != 'e')) {
    if (*cr == 0) {
      end = cr; // true end if input was std::array
      break;
    }
    if ((*cr == '.') || (*cr == '-')) {
      ++cr;
      continue;
    }

    result += *cr - '0';
    result *= 10;
    ++cr;
  }
  if (*cr == 'e') {
    // Check for numbers with exponential notations
    auto newEndP = cr;
    ++cr;
    int expVal;
    if (*cr == '-') {
      ++cr;
      fromChars(cr, end, expVal);
      scale += expVal;

    } else {
      fromChars(cr, end, expVal);
      result *= scale10<T>(expVal);
    }
    end = newEndP;
  }
  if (decimalPos != end) {
    scale += (end - decimalPos - 1);
  }

  result /= 10;
  return {result * scale10<T>(scale) * ((negative) ? -1 : 1), scale};
}

template <typename T>
inline T parseToScaledInt(const char *floatString, int scale) {
  return parseToScaledInt<T>(std::string_view{floatString}, scale);
}

template <typename T>
inline T parseToIntFindScale(const auto &floatString, int &scale) {
  auto begin = std::begin(floatString);
  auto end = std::end(floatString);
  auto decimalPos = std::find(begin, end, '.');
  if (decimalPos = end) {
    // Not a floating point number
    scale = 0;
    return parseToScaledInt(floatString, 0);
  }
}

inline auto &
scaledIntToString(auto value, int valueScale, auto &fixedLenBuffer,
                  int decimalPlaces = std::numeric_limits<int>::min()) {

  using T = std::decay_t<decltype(value)>;
  if (value == std::numeric_limits<int>::min()) {
    static auto fastZero = std::string_view{"0.0"};
    std::copy(fastZero.begin(), fastZero.end() + 1, std::begin(fixedLenBuffer));
    return fixedLenBuffer;
  }
  if (decimalPlaces == std::numeric_limits<int>::min()) {
    decimalPlaces = valueScale;
  }
  int scale;
  if constexpr (std::is_same_v<T, int128_t> || std::is_same_v<T, uint128_t>) {
    scale = 35;
  } else {
    scale = 18;
  }
  bool decimalSet = false;
  auto writePos = std::begin(fixedLenBuffer);
  auto endWrite = std::prev(std::end(fixedLenBuffer));
  bool positive = value >= 0;
  if (!positive) {
    *writePos = '-';
    ++writePos;
  }

  if (!(value / scale10<T>(valueScale))) {
    *writePos++ = '0';
  }

  bool numStarted = false;
  while ((writePos != endWrite) && (scale > -1)) {
    if ((!decimalSet) && (scale < valueScale)) {
      if (value == 0 || decimalPlaces <= 0) {
        break; // terminates write before decimal values if none exists
      }
      *writePos = '.';
      ++writePos;
      numStarted = true;
      decimalSet = true;
    }
    auto digitVal = value / scale10<T>(scale);

    value -= digitVal * scale10<T>(scale);

    if (digitVal != 0) {
      numStarted = true;
    }
    if (numStarted) {
      *writePos = '0' + abs(static_cast<int>(digitVal));
      ++writePos;
      if (decimalSet) {
        if (--decimalPlaces <= 0) {
          break;
        }
      }
    }
    --scale;
    if (decimalSet && (value == 0)) {
      break; // avoids trailing zeros
    }
  }
  *writePos = 0;
  return fixedLenBuffer;
}

template <typename F> inline F scaledIntToFloat(auto value, int valueScale) {
  // Note: floating point errors can arise here!!
  using T = std::decay_t<decltype(value)>;
  return static_cast<F>(value) / scale10<T>(valueScale);
}

template <typename T> inline T intScaled(T value, int valueScale) {
  if (valueScale == 0) {
    return value;
  }

  if (valueScale < 0) {
    if (valueScale < -15) {
      return 0;
    }
    const auto DIV = scale10<T>(abs(valueScale));
    const auto idiv = std::div(value, DIV);
    if (idiv.rem < DIV / 2) {
      return idiv.quot;
    }
    return idiv.quot + 1;
  }

  return value * scale10<T>(valueScale);
}

} // namespace moped