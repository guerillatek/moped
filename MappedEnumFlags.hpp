#pragma once

#include "MappedEnum.hpp"

namespace moped {

template <typename T, std::size_t index = 0>
constexpr bool hasFlaggableEntries(auto checkValue = 0) {
  if constexpr (index >= std::tuple_size_v<T>) {
    return true; // No more entries to check
  } else {
    if (std::to_underlying(std::get<index>(T{}).value) == 0) {
      return false; // Found an entry with zero value
    }
    if (checkValue & std::to_underlying(std::get<index>(T{}).value)) {
      return false; // Found a entry with shared bits from previous entries
    }
    checkValue |=
        std::to_underlying(std::get<index>(T{}).value);   // Accumulate flags
    return hasFlaggableEntries<T, index + 1>(checkValue); // Check next entry
  }
}

template <is_mapped_enum MappedEnumT> class MappedEnumFlags {

public:
  using FlagValueT = std::underlying_type_t<typename MappedEnumT::EnumType>;
  using MappedEnumType = MappedEnumT;
  using EnumType = typename MappedEnumT::EnumType;

  static_assert(
      hasFlaggableEntries<typename MappedEnumT::EntryTupleType>(FlagValueT{0}),
      "MappedEnumFlags types require that the underlying enum have entry "
      "values that are not zero and with non intersecting bit values");

  FlagValueT _value{0};

  void forEachSetFlag(auto &&func) {
    return MappedEnumType::forEachMappedValue([&](auto &entry) {
      if (has_value(entry.value)) {
        func(entry);
      }
    });
  }

public:
  using value_type = std::string_view;

  MappedEnumFlags() : _value(0) {}

  explicit MappedEnumFlags(MappedEnumT value)
      : _value(std::to_underlying(value)) {}

  explicit MappedEnumFlags(FlagValueT value) : _value() {}

  MappedEnumFlags &operator=(EnumType value) {
    _value = static_cast<FlagValueT>(value);
    return *this;
  }

  MappedEnumFlags operator^(EnumType value) {
    return MappedEnumFlags{_value ^ std::to_underlying(value)};
  }

  MappedEnumFlags operator|(EnumType value) {
    return MappedEnumFlags{_value | std::to_underlying(value)};
  }

  MappedEnumFlags operator&(MappedEnumT value) {
    return MappedEnumFlags{_value &= std::to_underlying(value)};
  }

  MappedEnumFlags &operator^=(EnumType value) {
    _value ^= std::to_underlying(value);
    return *this;
  }

  MappedEnumFlags &operator|=(EnumType value) {
    _value |= std::to_underlying(value);
    return *this;
  }

  MappedEnumFlags &operator&=(MappedEnumT value) {
    _value &= std::to_underlying(value);
    return *this;
  }

  // Collection-like interface
  bool emplace_back(MappedEnumT value) {
    if (has_value(value.getEnumValue())) {
      return false; // Value aeady exists
    }
    _value |= std::to_underlying(value.getEnumValue());
    return true; // Value inserted successfully
  }

  bool emplace_back(std::string_view value) {
    return emplace_back(MappedEnumT{value});
  }

  bool erase(EnumType value) {
    if (!has_value(value)) {
      return false; // Value does not exist
    }
    _value ^= std::to_underlying(value);
    return true; // Value erased successfully
  }

  void clear() { _value = 0; }

  bool has_value(EnumType value) const {
    return (_value & std::to_underlying(value)) != 0;
  }

  bool empty() const { return _value == 0; }

  operator FlagValueT() const { return _value; }
};

} // namespace moped