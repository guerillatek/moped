#pragma once

#include "concepts.hpp"

#include "ScaledInteger.hpp"
#include <charconv>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace moped {

template <typename NumericT>
inline std::expected<NumericT, std::string> ston(std::string_view value) {
  NumericT result;
  auto [ptr, ec] =
      std::from_chars(value.data(), value.data() + value.size(), result);
  if (ec != std::errc{}) {
    return std::unexpected(
        std::format("Failed to convert '{}' to numeric", value));
  }
  return result;
}

template <typename TargetT, typename DecodingTraits>
std::expected<TargetT, std::string> getValueFor(std::string_view value) {
  if constexpr (std::is_same_v<TargetT, bool>) {
    if (value == "true" || value == "1") {
      return true;
    } else if (value == "false" || value == "0") {
      return false;
    } else {
      return std::unexpected("Invalid boolean value: " + std::string{value});
    }
  } else if constexpr (std::is_same_v<TargetT, TimePoint>) {
    auto epochTime = DecodingTraits::TimePointFormaterT::getTimeValue(value);
    if (!epochTime) {
      throw std::runtime_error(epochTime.error());
    }
    return epochTime.value();
  } else if constexpr (std::is_same_v<TargetT, std::string> ||
                       std::is_same_v<TargetT, std::string_view>) {
    return TargetT{value};
  } else if constexpr ((std::is_integral_v<TargetT>) ||
                       (std::is_floating_point_v<TargetT>)) {
    return ston<TargetT>(value);
  } else if constexpr (IsOptionalC<TargetT, DecodingTraits>) {
    auto result =
        getValueFor<typename TargetT::value_type, DecodingTraits>(value);
    if (!result) {
      return std::unexpected(result.error());
    }
    return result.value();
  } else if constexpr (is_scaled_int<TargetT>) {
    return TargetT{value};
  } else {
    return std::unexpected("Unsupported type for value conversion: " +
                           std::string(typeid(TargetT).name()));
  }
}

template <typename TargetT, typename DecodingTraits>
std::expected<bool, std::string> getValueFor(bool value) {
  return value;
}

template <typename MemberT, DecodingTraitsC DecodingTraits> struct Handler {
  using MemberType = MemberT;
  using MemberIdType = typename DecodingTraits::MemberIdType;
};

template <typename T, DecodingTraitsC DecodingTraits>
struct ValueHandlerBase {};

template <typename T, DecodingTraitsC DecodingTraits>
  requires(IsMOPEDCompositeC<T, DecodingTraits>)
struct ValueHandlerBase<T, DecodingTraits> {

  using ValueTypeHandlerT =
      std::decay_t<decltype(T::template getMOPEDHandler<DecodingTraits>())>;

  ValueTypeHandlerT _valueTypeHandler{
      getMOPEDHandlerForParser<T, DecodingTraits>()};
};

template <IsMOPEDContentCollectionC T, DecodingTraitsC DecodingTraits>
struct ValueHandlerBase<T, DecodingTraits> {
  using ValueTypeHandlerT = Handler<T, DecodingTraits>;
  ValueTypeHandlerT _valueTypeHandler{};
};

template <IsMOPEDPushCollectionC MemberT, DecodingTraitsC DecodingTraits>
struct Handler<MemberT, DecodingTraits>
    : IMOPEDHandler<DecodingTraits>,
      ValueHandlerBase<typename MemberT::value_type, DecodingTraits> {
  using MemberIdType = typename DecodingTraits::MemberIdType;

  using MemberType = MemberT;
  using ValueType = typename MemberT::value_type;
  static constexpr bool HasCompositeValueType =
      IsMOPEDCompositeC<typename MemberT::value_type, DecodingTraits>;
  using MOPEDHandlerStack = std::stack<IMOPEDHandler<DecodingTraits> *>;

  Expected onMember(MOPEDHandlerStack &eventHandlerStack,
                    MemberIdType memberId) override {
    return std::unexpected("Parse error!!! onObjectFinish event not expected"
                           " in collection handler");
  }

  Expected onObjectStart(MOPEDHandlerStack &eventHandlerStack) override {
    if constexpr (HasCompositeValueType) {
      _targetCollection->emplace_back();
      this->_valueTypeHandler.setTargetMember(_targetCollection->back());
      return this->_valueTypeHandler.onObjectStart(eventHandlerStack);
    }
    return {};
  }

  Expected onObjectFinish(MOPEDHandlerStack &handlerStack) override {
    return std::unexpected("Parse error!!! onObjectFinish event not expected"
                           " in collection handler");
  }

  Expected onArrayStart(MOPEDHandlerStack &handlerStack) override {
    if (handlerStack.empty()) {
      return std::unexpected("Parse error!!! onArrayStart event not expected"
                             " in collection handler");
    }
    if (handlerStack.top() == this) {
      if constexpr (is_array<ValueType> || IsMOPEDPushCollectionC<ValueType>) {
        this->_valueTypeHandler.setTargetMember(
            _targetCollection->emplace_back());
        return this->_valueTypeHandler.onArrayStart(handlerStack);
      } else {
        return std::unexpected(
            "Parse error!!! onArrayStart event not expected in collection "
            "with non collection value types");
      }
    }
    handlerStack.push(this);
    return {};
  }
  Expected onArrayFinish(MOPEDHandlerStack &handlerStack) override {
    handlerStack.pop();
    return {};
  }

  Expected onStringValue(std::string_view value) override {
    return HandleScalarValue(value);
  }

  Expected onNumericValue(std::string_view value) override {
    return HandleScalarValue(value);
  }

  void setTargetMember(MemberT &targetMember) {
    _targetCollection = &targetMember;
  }

  void applyEmitterContext(auto &emitterContext, MemberIdType memberId) {
    emitterContext.onArrayStart(memberId, _targetCollection->size());

    for (auto &item : *_targetCollection) {
      if constexpr (HasCompositeValueType) {
        this->_valueTypeHandler.setTargetMember(item);
        this->_valueTypeHandler.applyEmitterContext(emitterContext,
                                                    std::nullopt);
      } else {
        emitterContext.onArrayValueEntry(item);
      }
    }
    emitterContext.onArrayFinish();
  }

private:
  Expected HandleScalarValue(std::string_view value) {
    if constexpr (HasCompositeValueType) {
      return std::unexpected(
          "Parse error!!! No active composite handler to set value");
    } else {
      auto result = getValueFor<ValueType, DecodingTraits>(value);
      if (!result) {
        return std::unexpected(result.error());
      }
      _targetCollection->emplace_back(result.value());
    }
    return {};
  }

  MemberT *_targetCollection;
};

template <IsMOPEDCompositeDispatcherC MemberT, DecodingTraitsC DecodingTraits>
struct Handler<MemberT, DecodingTraits>
    : IMOPEDHandler<DecodingTraits>,
      ValueHandlerBase<typename MemberT::value_type, DecodingTraits> {
  using MemberIdType = typename DecodingTraits::MemberIdType;

  using MemberType = MemberT;
  using ValueType = typename MemberT::value_type;
  static constexpr bool HasCompositeValueType =
      IsMOPEDCompositeC<typename MemberT::value_type, DecodingTraits>;
  using MOPEDHandlerStack = std::stack<IMOPEDHandler<DecodingTraits> *>;

  Expected onMember(MOPEDHandlerStack &, MemberIdType) override {
    return std::unexpected("Parse error!!! onObjectFinish event not expected"
                           " in dispatching handler");
  }

  Expected onObjectStart(MOPEDHandlerStack &eventHandlerStack) override {
    if (_currentIndex > 0) {
      _dispatcher->dispatchLastCapture();
    }

    if constexpr (HasCompositeValueType) {
      this->_valueTypeHandler.setTargetMember(_dispatcher->resetCapture());
      _currentIndex++;
      return this->_valueTypeHandler.onObjectStart(eventHandlerStack);
    }
    return {};
  }

  Expected onObjectFinish(MOPEDHandlerStack &) override {
    return std::unexpected("Parse error!!! onObjectFinish event not expected"
                           " in dispatching handler");
  }

  Expected onArrayStart(MOPEDHandlerStack &handlerStack) override {
    if (handlerStack.empty()) {
      return std::unexpected("Parse error!!! onArrayStart event not expected"
                             " in collection dispatch handler");
    }
    if (handlerStack.top() == this) {
      if (_currentIndex > 0) {
        _dispatcher->dispatchLastCapture();
      }
      if constexpr (is_array<ValueType> || IsMOPEDPushCollectionC<ValueType>) {
        this->_valueTypeHandler.setTargetMember(_dispatcher->resetCapture());
        ++_currentIndex;
        return this->_valueTypeHandler.onArrayStart(handlerStack);
      } else {
        return std::unexpected(
            "Parse error!!! onArrayStart event not expected in collection "
            "with non collection value types");
      }
    }
    handlerStack.push(this);
    return {};
  }

  Expected onArrayFinish(MOPEDHandlerStack &handlerStack) override {
    _dispatcher->dispatchLastCapture();
    handlerStack.pop();
    _currentIndex = 0;
    return {};
  }

  Expected onStringValue(std::string_view value) override {
    if (_currentIndex > 0) {
      _dispatcher->dispatchLastCapture();
    }
    HandleScalarValue(value);
    _currentIndex++;
    return {};
  }

  Expected onNumericValue(std::string_view value) override {
    if (_currentIndex > 0) {
      _dispatcher->dispatchLastCapture();
    }
    HandleScalarValue(value);
    _currentIndex++;
    return {};
  }

  void setTargetMember(MemberT &targetMember) { _dispatcher = &targetMember; }

  void applyEmitterContext(auto &emitterContext,
                           std::optional<MemberIdType> memberId) {
    return std::unexpected("Emission not supported for types that use function "
                           "dispatchers in lieu of collection types");
  }

private:
  Expected HandleScalarValue(std::string_view value) {
    if constexpr (HasCompositeValueType) {
      return std::unexpected(
          "Parse error!!! No active composite handler to set value");
    } else {
      auto result = getValueFor<ValueType, DecodingTraits>(value);
      if (!result) {
        return std::unexpected(result.error());
      }
      _dispatcher->setCurrentValue(result.value());
      _dispatcher->dispatchLastCapture();
    }
    return {};
  }

  size_t _currentIndex{0};

  MemberT *_dispatcher;
};

template <is_array MemberT, DecodingTraitsC DecodingTraits>
struct Handler<MemberT, DecodingTraits>
    : IMOPEDHandler<DecodingTraits>,
      ValueHandlerBase<typename MemberT::value_type, DecodingTraits> {

  using MemberType = MemberT;
  using ValueType = typename MemberT::value_type;
  static constexpr bool HasCompositeValueType =
      IsMOPEDCompositeC<typename MemberT::value_type, DecodingTraits>;

  using MemberIdT = typename DecodingTraits::MemberIdType;
  using MOPEDHandlerStack = std::stack<IMOPEDHandler<DecodingTraits> *>;

  Expected onMember(MOPEDHandlerStack &, MemberIdT) override {
    return std::unexpected("Parse error!!! onMember event not expected in "
                           "array handler");
  }

  Expected onObjectStart(MOPEDHandlerStack &eventHandlerStack) override {

    if constexpr (HasCompositeValueType) {
      if (_currentIndex >= std::size(*_targetArray)) {
        return std::unexpected("Index out of bounds");
      }

      this->_valueTypeHandler.setTargetMember(*_targetArray[_currentIndex++]);
      return this->_valueTypeHandler.onObjectStart(eventHandlerStack);
    } else {
      return std::unexpected(
          "Parse error!!! onObjectStart event not expected in array "
          "with non composite value types");
    }
  }

  virtual Expected onObjectFinish(MOPEDHandlerStack &) {
    return std::unexpected(
        "Parse error!!! onObjectFinish event not expected for array handler");
  }

  virtual Expected onArrayStart(MOPEDHandlerStack &handlerStack) {
    if (handlerStack.empty()) {
      return std::unexpected("Parse error!!! onArrayStart event not expected"
                             " in array handler");
    }
    if (handlerStack.top() == this) {
      if constexpr (is_array<ValueType> || IsMOPEDPushCollectionC<ValueType>) {
        if (_currentIndex >= sizeof(*_targetArray)) {
          return std::unexpected("Index out of bounds");
        }

        this->_valueTypeHandler.setTargetMember(*_targetArray[_currentIndex++]);
        return this->_valueTypeHandler.onArrayStart(handlerStack);
      } else {
        return std::unexpected(
            "Parse error!!! onArrayStart event not expected in array "
            "with non collection value types");
      }
    }
    _currentIndex = 0;
    handlerStack.push(this);
    return {};
  }

  virtual Expected onArrayFinish(MOPEDHandlerStack &handlerStack) {
    handlerStack.pop();
    return {};
  }

  Expected onStringValue(std::string_view value) override {
    if (_currentIndex >= sizeof(*_targetArray)) {
      return std::unexpected("Index out of bounds");
    }
    auto result = getValueFor<ValueType, DecodingTraits>(value);
    if (!result) {
      return std::unexpected(result.error());
    }
    (*_targetArray)[_currentIndex] = result.value();
    ++_currentIndex;
    return {};
  }

  Expected onNumericValue(std::string_view value) override {
    if (_currentIndex >= sizeof(*_targetArray)) {
      return std::unexpected("Index out of bounds");
    }
    auto result = getValueFor<ValueType, DecodingTraits>(value);
    if (!result) {
      return std::unexpected(result.error());
    }
    (*_targetArray)[_currentIndex] = result.value();
    ++_currentIndex;
    return {};
  }

  void setTargetMember(MemberT &targetMember) { _targetArray = &targetMember; }

  void applyEmitterContext(auto &emitterContext,
                           std::optional<MemberIdT> memberId) {
    emitterContext.onArrayStart(memberId, sizeof(*_targetArray));

    for (auto &item : *_targetArray) {
      if constexpr (HasCompositeValueType) {
        this->_valueTypeHandler.setTargetMember(item);
        this->_valueTypeHandler.applyEmitterContext(emitterContext,
                                                    std::nullopt);
      } else {
        emitterContext.onArrayValueEntry(item);
      }
    }
    emitterContext.onArrayFinish();
  }

private:
  Expected HandleScalarValue(std::string_view value) {
    if constexpr (HasCompositeValueType) {
      return std::unexpected(
          "Parse error!!! No active composite handler to set value");
    } else {
      auto result = getValueFor<ValueType, DecodingTraits>(value);
      if (!result) {
        return std::unexpected(result.error());
      }
      _targetArray[_currentIndex] = result.value();
      ++_currentIndex;
    }
    return {};
  }

  MemberT *_targetArray;
  size_t _currentIndex = 0;
};

template <IsMOPEDMapCollectionC MemberT, DecodingTraitsC DecodingTraits>
struct Handler<MemberT, DecodingTraits>
    : IMOPEDHandler<DecodingTraits>,
      ValueHandlerBase<typename MemberT::mapped_type, DecodingTraits> {
  using MemberIdType = typename DecodingTraits::MemberIdType;

  using MemberType = MemberT;
  using KeyType = typename MemberT::key_type;
  using MappedType = typename MemberT::value_type;
  static constexpr bool HasCompositeMappedType =
      IsMOPEDCompositeC<typename MemberT::mapped_type, DecodingTraits>;
  using MOPEDHandlerStack = std::stack<IMOPEDHandler<DecodingTraits> *>;

  Expected onMember(MOPEDHandlerStack &eventHandlerStack,
                    MemberIdType memberId) override {
    _currentKey = memberId;
    return {};
  }

  Expected onObjectStart(MOPEDHandlerStack &eventHandlerStack) override {
    if (!_addingContent) {
      _addingContent = true;
      eventHandlerStack.push(this);
      return {};
    }
    if constexpr (HasCompositeMappedType) {
      this->_valueTypeHandler.setTargetMember(
          (*_targetCollection)[_currentKey]);
      return this->_valueTypeHandler.onObjectStart(eventHandlerStack);
    } else {
      return std::unexpected(
          "Parse error!!! onObjectStart event not expected in collection "
          "with scalar value types");
    }
    return {};
  }

  Expected onObjectFinish(MOPEDHandlerStack &handlerStack) override {
    handlerStack.pop();
    _addingContent = false;
    return {};
  }

  Expected onArrayStart(MOPEDHandlerStack &handlerStack) override {
    handlerStack.push(this);
    return {};
  }
  Expected onArrayFinish(MOPEDHandlerStack &handlerStack) override {
    handlerStack.pop();
    _addingContent = false;
    return {};
  }

  Expected onStringValue(std::string_view value) override {
    return HandleScalarValue(value);
  }

  Expected onNumericValue(std::string_view value) override {
    return HandleScalarValue(value);
  }

  void setTargetMember(MemberT &targetMember) {
    _targetCollection = &targetMember;
  }

  void applyEmitterContext(auto &emitterContext,
                           std::optional<MemberIdType> memberId) {
    emitterContext.onObjectStart(memberId);

    for (const auto &[key, value] : *_targetCollection) {
      if constexpr (HasCompositeMappedType) {
        this->_valueTypeHandler.setTargetMember(value);
        this->_valueTypeHandler.applyEmitterContext(emitterContext, key);
      } else {
        emitterContext.onObjectValueEntry(key, value);
      }
    }
    emitterContext.onObjectFinish();
  }

private:
  Expected HandleScalarValue(std::string_view value) {
    if constexpr (HasCompositeMappedType) {
      return std::unexpected(
          "Parse error!!! No active composite handler to set value");
    } else {
      auto result = getValueFor<MappedType, DecodingTraits>(value);
      if (!result) {
        return std::unexpected(result.error());
      }
      (*_targetCollection)[_currentKey] = result.value();
    }
    return {};
  }

  MemberT *_targetCollection;
  KeyType _currentKey;
  bool _addingContent{false};
};

template <typename MemberT, DecodingTraitsC DecodingTraits>
  requires(IsOptionalC<MemberT, DecodingTraits>)
struct Handler<MemberT, DecodingTraits>
    : Handler<typename MemberT::value_type, DecodingTraits> {

  using MemberType = MemberT;
  using MemberIdT = typename DecodingTraits::MemberIdType;

  void setTargetMember(MemberT &targetMember) {
    targetMember = MemberT{};
    Handler<typename MemberT::value_type, DecodingTraits>::setTargetMember(
        targetMember.value());
  }
};

template <typename MemberT, DecodingTraitsC DecodingTraits>
  requires(IsMOPEDCompositeC<MemberT, DecodingTraits>)
struct Handler<MemberT, DecodingTraits>
    : std::decay_t<
          decltype(getMOPEDHandlerForParser<MemberT, DecodingTraits>())> {
  using HandlerBase = std::decay_t<
      decltype(getMOPEDHandlerForParser<MemberT, DecodingTraits>())>;
  using MemberType = MemberT;
  using MemberIdT = typename DecodingTraits::MemberIdType;

  Handler()
      : HandlerBase(MemberT::template getMOPEDHandler<DecodingTraits>()) {}
};

template <DecodingTraitsC DecodingTraits, typename T>
struct FinalMultiMopedHandlerHarness {
  using HandlerT =
      std::decay_t<decltype(getMOPEDHandlerForParser<DecodingTraits, T>())>;
  HandlerT handler;
  template <typename TargetType> auto &getHandlerForType() {
    if constexpr (std::is_same_v<TargetType, T>) {
      return handler;
    } else {
      static_assert(false, "Type not found in MultiMopedHandlerHarness");
    }
  }
};

template <DecodingTraitsC DecodingTraits, typename T, typename... MOPEDTypes>
struct MultiMopedHandlerHarness;

template <DecodingTraitsC DecodingTraits, typename T, typename... MOPEDTypes>
constexpr auto getMultiMopedHandlerHarness() {
  if constexpr (sizeof...(MOPEDTypes) == 1) {
    return FinalMultiMopedHandlerHarness<DecodingTraits, T>{};
  } else {
    return MultiMopedHandlerHarness<DecodingTraits, T, MOPEDTypes...>{};
  }
};

template <DecodingTraitsC DecodingTraits, typename T, typename... MOPEDTypes>
struct MultiMopedHandlerHarness {
  using HandlerT =
      std::decay_t<decltype(getMOPEDHandlerForParser<DecodingTraits, T>())>;
  using TailHandlersT = std::decay_t<
      decltype(getMultiMopedHandlerHarness<DecodingTraits, MOPEDTypes...>())>;
  HandlerT handler;
  TailHandlersT tailHandlers;

  template <typename TargetType> auto &getHandlerForType() {
    if constexpr (std::is_same_v<TargetType, T>) {
      return handler;
    } else {
      return tailHandlers.template getHandlerForType<TargetType>();
    }
  }
};

template <DecodingTraitsC DecodingTraits, typename... MOPEDTypes>
constexpr auto
getMultiMopedHandlerHarnessFromVariant(std::variant<MOPEDTypes...>) {
  return getMultiMopedHandlerHarness<DecodingTraits, MOPEDTypes...>();
};

template <is_variant VariantT, DecodingTraitsC DecodingTraits>
struct Handler<VariantT, DecodingTraits> : IMOPEDHandler<DecodingTraits> {

  using MemberType = VariantT;
  using MemberIdT = typename DecodingTraits::MemberIdType;
  using MultiMopedHandlerHarness =
      std::decay_t<decltype(getMultiMopedHandlerHarnessFromVariant<
                            DecodingTraits>(VariantT{}))>;

  using MOPEDHandlerStack = std::stack<IMOPEDHandler<DecodingTraits> *>;
  Expected onObjectStart(MOPEDHandlerStack &eventHandlerStack) override {
    return std::visit(
        [this, &eventHandlerStack](auto &selected) {
          using SelectedT = std::decay_t<decltype(selected)>;
          auto &handler =
              _handlerHarness.template getHandlerForType<SelectedT>();
          handler.setTargetMember(selected);
          return handler.onObjectStart(eventHandlerStack);
        },
        *_captureVariant);
  }

  void applyEmitterContext(auto &emitterContext, MemberIdT memberId) {
    std::visit(
        [this, &emitterContext, &memberId](auto &selected) {
          using SelectedT = std::decay_t<decltype(selected)>;
          auto &handler =
              _handlerHarness.template getHandlerForType<SelectedT>();
          handler.setTargetMember(selected);
          handler.applyEmitterContext(emitterContext, memberId);
        },
        *_captureVariant);
  }

  void setTargetMember(MemberType &targetMember) {
    _captureVariant = &targetMember;
  }

private:
  MultiMopedHandlerHarness _handlerHarness;
  VariantT *_captureVariant;
};

template <typename CaptureT, typename MemberPtrT,
          DecodingTraitsC DecodingTraits>
struct MemberIdHandlerPair {
  using MemberIdT = typename DecodingTraits::MemberIdType;
  MemberIdT memberId;
  MemberPtrT memberPtr;
  using MemberT = std::decay_t<decltype(std::declval<CaptureT>().*
                                        std::declval<MemberPtrT>())>;

  Expected setValue(CaptureT &setTarget, auto value) {
    if constexpr (IsSettableC<MemberT, DecodingTraits> && requires() {
                    setTarget.*memberPtr =
                        getValueFor<MemberT, DecodingTraits>(value).value();
                  }) {
      auto result = getValueFor<MemberT, DecodingTraits>(value);
      if (!result) {
        return std::unexpected(result.error());
      }
      setTarget.*memberPtr = result.value();
    }
    return {};
  }

  void applyEmitterContext(auto &emitterContext, CaptureT &captureTarget) {
    auto &memberValue = captureTarget.*memberPtr;
    if constexpr (IMOPEDHandlerC<decltype(handler), DecodingTraits>) {
      handler.setTargetMember(memberValue);
      handler.applyEmitterContext(emitterContext, memberId);
    } else {
      emitterContext.onObjectValueEntry(memberId, memberValue);
    }
  }

  auto &getMember(CaptureT &getTarget) { return getTarget.*memberPtr; }

  Handler<MemberT, DecodingTraits> handler{};
};

template <typename CaptureType, typename MemberEventHandlerTuple,
          DecodingTraitsC DecodingTraits>
struct MappedObjectParserEncoderDispatcher : IMOPEDHandler<DecodingTraits> {

  using MOPEDHandlerStack = std::stack<IMOPEDHandler<DecodingTraits> *>;
  using MemberIdT = typename DecodingTraits::MemberIdType;

  MappedObjectParserEncoderDispatcher(MemberEventHandlerTuple &&handlerTuple)
      : _handlerTuple{std::move(handlerTuple)} {}

  Expected onObjectStart(MOPEDHandlerStack &eventHandlerStack) override {
    if (!_activeMemberOffset.has_value()) {
      eventHandlerStack.push(this);
      return {};
    }

    return dispatchForActiveMember<0>([&eventHandlerStack](auto &handler) {
      return handler.onObjectStart(eventHandlerStack);
    });
  }

  Expected onObjectFinish(MOPEDHandlerStack &eventHandlerStack) override {
    _activeMemberOffset.reset();
    eventHandlerStack.pop();
    return {};
  }

  Expected onArrayStart(MOPEDHandlerStack &eventHandlerStack) override {
    if (!_activeMemberOffset.has_value()) {
      return std::unexpected("Parse error!!! onArrayStart event not expected "
                             "in object handler with a preceding member event");
    }

    return dispatchForActiveMember<0>([&eventHandlerStack](auto &handler) {
      return handler.onArrayStart(eventHandlerStack);
    });
  }

  Expected onMember(MOPEDHandlerStack &eventHandlerStack,
                    MemberIdT memberId) override {
    return setActiveMember<0>(memberId, eventHandlerStack);
  }

  Expected onStringValue(std::string_view value) override {
    return setActiveMemberValue<0>(value);
  }

  Expected onNumericValue(std::string_view value) {
    return setActiveMemberValue<0>(value);
  }

  Expected onBooleanValue(bool value) override {
    return setActiveMemberValue<0>(value);
  }

  void setTargetMember(CaptureType &targetMember) {
    _captureObject = &targetMember;
  }

  Expected onArrayFinish(MOPEDHandlerStack &) { return {}; }

  template <size_t MemberIndex = 0>
  void applyEmitterContext(auto &emitterContext,
                           std::optional<MemberIdT> memberId = std::nullopt) {
    if constexpr (MemberIndex == 0) {
      emitterContext.onObjectStart(memberId);
    }

    if constexpr (MemberIndex == std::tuple_size_v<MemberEventHandlerTuple>) {
      return; // No more members to process
    } else {
      std::get<MemberIndex>(_handlerTuple)
          .applyEmitterContext(emitterContext, *_captureObject);
      // Recursively apply emitter context for all composite members
      applyEmitterContext<MemberIndex + 1>(emitterContext, memberId);
    }

    if constexpr (MemberIndex == 0) {
      emitterContext.onObjectFinish();
    }
  }

private:
  std::string_view _activeMemberName;
  template <size_t MemberIndex>
  Expected setActiveMember(MemberIdT memberId,
                           MOPEDHandlerStack &eventHandlerStack) {
    if constexpr (MemberIndex == std::tuple_size_v<MemberEventHandlerTuple>) {
      return std::unexpected(
          std::format("Member, '{}' not found in MOPED handler", memberId));
    } else {
      auto &nameHandler = std::get<MemberIndex>(_handlerTuple);
      if (nameHandler.memberId == memberId) {
        _activeMemberOffset = MemberIndex;
        return {};
      } else if (nameHandler.memberId == DecodingTraits::EmbeddingMemberId) {
        if constexpr (IMOPEDHandlerC<
                          std::decay_t<decltype(nameHandler.handler)>,
                          DecodingTraits>) {
          // If we haven't found the target member before the embedding member,
          // we need to set the embedding handler as the active handler and hand
          // off the current event to it.
          nameHandler.handler.setTargetMember(
              _captureObject->*(nameHandler.memberPtr));
          eventHandlerStack.pop();
          Expected result =
              nameHandler.handler.onObjectStart(eventHandlerStack);
          if (!result) {
            return result;
          }
          return nameHandler.handler.onMember(eventHandlerStack, memberId);
        } else {
          return std::unexpected("Embedded members must be a composite type");
        }
      }

      return setActiveMember<MemberIndex + 1>(memberId, eventHandlerStack);
    }
  }

  template <size_t MemberIndex> Expected setActiveMemberValue(auto value) {
    if constexpr (MemberIndex == std::tuple_size_v<MemberEventHandlerTuple>) {
      return std::unexpected("Parse error ... no active member available");
    } else {
      if (!_activeMemberOffset.has_value()) {
        return std::unexpected(std::format(
            "Parse error ... no active member available for current value {}",
            value));
      }
      if (MemberIndex == *_activeMemberOffset) {
        return std::get<MemberIndex>(_handlerTuple)
            .setValue(*_captureObject, value);
      }
      return setActiveMemberValue<MemberIndex + 1>(value);
    }
  }

  template <size_t MemberIndex, typename H>
  Expected dispatchForActiveMember(H &&handlerActionFunction) {
    if constexpr (MemberIndex == std::tuple_size_v<MemberEventHandlerTuple>) {
      return std::unexpected("Parse error ... no active member available");
    } else {
      if (!_activeMemberOffset.has_value()) {
        return std::unexpected(
            "Parse error ... no active member available for current object");
      }
      auto &nameHandler = std::get<MemberIndex>(_handlerTuple);
      if constexpr (IMOPEDHandlerC<std::decay_t<decltype(nameHandler.handler)>,
                                   DecodingTraits>) {
        if (MemberIndex == *_activeMemberOffset) {
          nameHandler.handler.setTargetMember(
              _captureObject->*(nameHandler.memberPtr));
          return handlerActionFunction(nameHandler.handler);
        }
      }
      return dispatchForActiveMember<MemberIndex + 1>(
          std::forward<H>(handlerActionFunction));
    }
  }

  CaptureType *_captureObject;
  MemberEventHandlerTuple _handlerTuple;
  std::optional<std::uint32_t> _activeMemberOffset;
};

template <DecodingTraitsC DecodingTraits, typename CaptureT>
constexpr auto mopedTuple() {
  return std::tuple<>{};
}

template <DecodingTraitsC DecodingTraits, typename CaptureT, typename... Args>
constexpr auto mopedTuple(std::string_view name, auto memberPtr,
                          Args &&...args) {
  using MemberPtrT = decltype(memberPtr);
  auto handlerTuple =
      std::make_tuple(MemberIdHandlerPair<CaptureT, MemberPtrT, DecodingTraits>{
          DecodingTraits::getMemberId(name), memberPtr});
  return std::tuple_cat(handlerTuple, mopedTuple<DecodingTraits, CaptureT>(
                                          std::forward<Args>(args)...));
}

template <DecodingTraitsC DecodingTraits, typename CaptureT, typename... Args>
auto mopedHandler(Args &&...args) {
  auto handlerTuple =
      mopedTuple<DecodingTraits, CaptureT>(std::forward<Args>(args)...);
  using HandlerTupleT = decltype(handlerTuple);
  return MappedObjectParserEncoderDispatcher<CaptureT, HandlerTupleT,
                                             DecodingTraits>{
      std::move(handlerTuple)};
}

} // namespace moped