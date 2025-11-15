#pragma once

#include "concepts.hpp"
#include <optional>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace moped {

template <auto ksv, typename T> struct MappedTypeEntry {
  static constexpr auto key_select_value = ksv;
  using MappedType = T;
};

template <auto key_select_value, typename T, auto... Args>
constexpr auto selectableTypeTuple() {
  if constexpr (sizeof...(Args) == 0) {
    return std::tuple(MappedTypeEntry<key_select_value, T>{});
  } else {
    return std::tuple_cat(std::tuple(MappedTypeEntry<key_select_value, T>{}),
                          selectableTypeTuple<Args...>());
  }
};

template <typename... Types>
constexpr auto getVariantFromTuple(const std::tuple<Types...> &) {
  if constexpr (sizeof...(Types) == 0) {
    return std::monostate{};
  } else {
    return std::variant<Types...>{};
  }
}

template <auto key_select_value, typename T, auto... Args>
constexpr auto getTypeTupleFromParams() {
  if constexpr (sizeof...(Args) == 0) {
    return std::tuple(T{});
  } else {
    return std::tuple_cat(T{}, getTypeTupleFromParams<Args...>());
  }
};

template <auto key_select_value, typename T, auto... Args>
constexpr auto getTargetSetVariantFromParams() {
  return getVariantFromTuple(
      std::tuple_cat(std::monostate{},
                     getTypeTupleFromParams<key_select_value, T, Args...>()));
}

template <auto key_select_value, typename T, auto... Args>
class ParsingPivotMember {

  using SelectableTypesLookup =
      decltype(selectableTypeTuple<key_select_value, T, Args...>());

public:
  using SelectorValueT = decltype(key_select_value);
  using TargetSetVariantT =
      decltype(getTargetSetVariantFromParams<key_select_value, T, Args...>());

  ParsingPivotMember(TargetSetVariantT &memberVariant)
      : _targetSetVariant{memberVariant} {}

  ParsingPivotMember &operator=(const SelectorValueT &value) {
    _selectorValue = value;
    return *this;
  }

  operator bool() const {
    return _selectorValue.has_value() && (_targetSetVariant.index() != 0);
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const ParsingPivotMember &csv) {
    if (!csv._selectorValue.has_value()) {
      return os;
    }
    return os << *(csv._selectorValue);
  }

private:
  TargetSetVariantT
      &_targetSetVariant; // This can refer to mapped member in the
      
                          // containing class or an embedded member.
  std::optional<SelectorValueT> _selectorValue{};

  template <size_t I = 0> void selectTypeForAssignedValue() {
    if constexpr (I < std::tuple_size_v<SelectableTypesLookup>) {
      using CurrentEntry = std::tuple_element_t<I, SelectableTypesLookup>;
      if (CurrentEntry::key_select_value == *_selectorValue) {
        using MappedType = typename CurrentEntry::MappedType;
        _targetSetVariant =
            MappedType{}; // The target variant is now loaded with the
        // appropriate type to capture the future parse values
        // implied by the selector value.
      } else {
        selectTypeForAssignedValue<I + 1>();
      }
    }
  }
};

} // namespace moped