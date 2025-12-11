#pragma once

#include "concepts.hpp"
#include "moped/getValueFor.hpp"

#include <optional>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace moped {

template <auto value, typename T> struct ValueMapEntry {
  static constexpr auto type_select_value = value;
  using MappedType = T;
};

template <auto selecting_member, typename... MapEntries> struct PivotMap {
  using ValueToTypeMapT = std::tuple<MapEntries...>;
  static constexpr auto pivot_member = selecting_member;
};

inline constexpr char nullMember[] = "";

using EmptyPivotMapT = PivotMap<nullMember>;

template <typename ParseHandlerT, typename PivotMapT>
class PivotMemberTypeSelector {

public:
  using ValueToTypeMapT = typename PivotMapT::ValueToTypeMapT;
  using DecodingTraits = typename ParseHandlerT::DecodingTraits;
  using PivotValueT = typename DecodingTraits::PivotValueT;

  PivotMemberTypeSelector(ParseHandlerT &parserHandler)
      : _parserHandler(parserHandler) {}

  Expected onMember(std::string_view memberName) {
    if (memberName == PivotMapT::pivot_member) {
      _pivotOnNextValue = true;
    }
    return {};
  }

  Expected onStringValue(std::string_view value) {
    if (_pivotOnNextValue) {
      auto result = applyPivotValue(value);
      if (!result) {
        return result;
      }
    }
    return {};
  }

  Expected onNumericValue(std::string_view value) {
    if (_pivotOnNextValue) {
      auto result = applyPivotValue(value);
      if (!result) {
        return result;
      }
    }
    return {};
  }

  template <typename... ConstructorArgs> void reset(ConstructorArgs &&...args) {
    _pivotOnNextValue = false;
    _compositeSet = false;
  }

  auto compositeSet() const { return _compositeSet; }

  void reset() {
    _pivotOnNextValue = false;
    _compositeSet = false;
  }

private:
  Expected applyPivotValue(std::string_view value) {
    if constexpr (std::tuple_size_v<ValueToTypeMapT> == 0) {
      return std::unexpected("No pivot map entries defined");
    } else {
      auto result = getValueFor<PivotValueT, DecodingTraits>(value);
      if (!result) {
        return std::unexpected(result.error());
      }
      auto selectResult = selectTypeForAssignedValue(result.value());
      if (!selectResult) {
        return selectResult;
      }
      return selectTypeForAssignedValue(result.value());
    }
  }

  template <size_t I = 0> Expected selectTypeForAssignedValue(auto value) {
    if constexpr (I < std::tuple_size_v<ValueToTypeMapT>) {
      using CurrentEntry = std::tuple_element_t<I, ValueToTypeMapT>;
      if (PivotValueT{CurrentEntry::type_select_value} == value) {
        using MappedType = typename CurrentEntry::MappedType;
        _parserHandler.template setActiveComposite<MappedType>();
        _compositeSet = true;
        return {};
      } else {
        return selectTypeForAssignedValue<I + 1>(value);
      }
    }
    return std::unexpected(
        std::format("No type selected for member {} with value {}",
                    PivotMapT::pivot_member, value));
  }

  ParseHandlerT &_parserHandler;
  bool _pivotOnNextValue{false};
  bool _compositeSet{false};
};

} // namespace moped