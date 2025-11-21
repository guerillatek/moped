#pragma once

#include "concepts.hpp"

#include <optional>

namespace moped {

// These templates are used as alternatives to a c++ vector, list, dequeue ...
// in class definitions targeted for moped mapping. When parsing types with
// large collections with a value_type of 'ValueT' this template allows the
// parsing code dispatch each member of the collection to a handler function
// when it's parsing is complete versus allocating and storing the entire
// collection in memory minimizing memory footprint of the parsing operations.

template <typename ValueT> class CollectionFunctionDispatcher {

public:
  using value_type = ValueT;
  using HandlerT = std::function<void(const ValueT &)>;

  ValueT &resetCapture() {
    _captureValue = ValueT{};
    return *_captureValue;
  }

  CollectionFunctionDispatcher() = default;
  CollectionFunctionDispatcher(auto &&handler) : _handler(std::move(handler)) {}

  void setCurrentValue(const ValueT &value) { _captureValue = value; }

  void dispatchLastCapture() {
    if (!_captureValue.has_value()) {
      return;
    }
    _handler(*_captureValue);
    _captureValue.reset();
  }

  void setHandler(auto &&handler) { _handler = std::move(handler); }

private:
  std::optional<ValueT> _captureValue{};
  HandlerT _handler{};
};

} // namespace moped