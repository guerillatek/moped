#pragma once

#include "concepts.hpp"
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace moped {

template <const char *mappedName, auto enumVal> struct EnumValueEntry {
  static constexpr auto stringValue = mappedName;
  static constexpr auto value = enumVal;
};

template <const char *mappedName, auto enumVal, auto... Args>
constexpr auto enumEntryTuple() {
  if constexpr (sizeof...(Args) == 0) {
    return std::tuple(EnumValueEntry<mappedName, enumVal>{});
  } else {
    return std::tuple_cat(std::tuple(EnumValueEntry<mappedName, enumVal>{}),
                          enumEntryTuple<Args...>());
  }
};

template <const char *mappedName, auto enumVal, auto... Args> class MappedEnum {

  static constexpr auto entries =
      enumEntryTuple<mappedName, enumVal, Args...>();

public:
  using EnumT = decltype(enumVal);
  MappedEnum() = default;
  MappedEnum(EnumT value) : _value(value) {}
  MappedEnum(const std::string_view &str) {
    auto entry = findValueForString<0>(str);
    if (!entry) {
      throw std::invalid_argument("Invalid enum string: " + std::string(str));
    }
    _value = entry.value();
  }

  MappedEnum &operator=(EnumT value) {
    _value = value;
    return *this;
  }

  MappedEnum &operator=(std::string_view str) {
    auto entry = findValueForString<0>(str);
    if (!entry) {
      throw std::invalid_argument("Invalid enum string: " + std::string(str));
    }
    _value = entry.value();
    return *this;
  }

  using EnumType = EnumT;
  using EntryTupleType = decltype(entries);

  friend std::string to_string(const MappedEnum &mappedEnum) {
    auto entry = findStringForValue<0>(mappedEnum._value);
    if (!entry) {
      throw std::invalid_argument("Invalid enum value");
    }
    return std::string(entry.value());
  }

  friend bool operator==(const MappedEnum &lhs, const MappedEnum &rhs) {
    return lhs._value == rhs._value;
  }
  friend bool operator!=(const MappedEnum &lhs, const MappedEnum &rhs) {
    return !(lhs == rhs);
  }
  friend bool operator<(const MappedEnum &lhs, const MappedEnum &rhs) {
    return lhs._value < rhs._value;
  }
  friend bool operator>(const MappedEnum &lhs, const MappedEnum &rhs) {
    return lhs._value > rhs._value;
  }
  friend bool operator<=(const MappedEnum &lhs, const MappedEnum &rhs) {
    return !(lhs > rhs);
  }
  friend bool operator>=(const MappedEnum &lhs, const MappedEnum &rhs) {
    return !(lhs < rhs);
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const MappedEnum &mappedEnum) {
    auto entry = findStringForValue<0>(mappedEnum._value);
    if (!entry) {
      throw std::invalid_argument("Invalid enum value");
    }
    return os << std::string{entry.value()};
  }

  auto getEnumValue() const { return _value; }

private:
  template <std::size_t index = 0>
  static std::optional<std::string_view> findStringForValue(EnumT value) {
    if constexpr (index >= std::tuple_size_v<EntryTupleType>) {
      return std::nullopt; // No more entries to check
    } else {
      if (std::get<index>(entries).value == value) {
        return std::get<index>(entries).stringValue;
      }
      return findStringForValue<index + 1>(value);
    }
  }

  template <std::size_t index = 0>
  static std::optional<EnumT> findValueForString(std::string_view str) {
    if constexpr (index >= std::tuple_size_v<EntryTupleType>) {
      return std::nullopt; // No more entries to check
    } else {
      if (str == std::get<index>(entries).stringValue) {
        return std::get<index>(entries).value;
      }
      return findValueForString<index + 1>(str);
    }
  }

  EnumT _value{};
};

} // namespace moped