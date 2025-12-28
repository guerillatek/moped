#pragma once

#include "moped/concepts.hpp"

#include <optional>

namespace moped {

// These templates are used as alternatives to a c++ vector, list, dequeue ...
// in class definitions targeted for moped mapping. When parsing types with
// large collections with a value_type of 'ValueT' this template allows the
// parsing code dispatch each member of the collection to a handler function
// when it's parsing is complete versus allocating and storing the entire
// collection in memory minimizing memory footprint of the parsing operations.

template <typename MapType, typename ValueT> class CollectionFunctionDispatcher {

public:
  using value_type = ValueT;
  using HandlerT = std::function<Expected(const ValueT &)>;

  ValueT &resetValueCapture() {
    _captureValue = ValueT{};
    return *_captureValue;
  }

  MappedCollectionFunctionDispatcher() = default;
  MappedCollectionFunctionDispatcher(auto &&handler) : _handler(std::move(handler)) {}

  void setCurrentValue(const ValueT &value) { _captureValue = value; }

  Expected dispatchLastCapture() {
    if (!_handler) {
      return std::unexpected("No function assigned to collection dispatcher");
    }
    if (!_captureValue) {
      return {};
    }
    auto result = _handler(*_captureValue);
    _captureValue.reset();
    return result;
  }

  void setHandler(auto &&handler) { _handler = std::move(handler); }

private:
  std::optional<ValueT> _captureValue{};
  HandlerT _handler{};
};

} // namespace moped