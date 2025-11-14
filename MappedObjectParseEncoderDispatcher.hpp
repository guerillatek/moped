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

template <typename NumericT> inline NumericT ston(std::string_view value) {
  NumericT result;
  auto [ptr, ec] =
      std::from_chars(value.data(), value.data() + value.size(), result);
  if (ec != std::errc{}) {
    throw std::runtime_error(
        std::format("Failed to convert '{}' to numeric", value));
  }
  return result;
}

template <typename TargetT, typename MemberIdTraits>
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
    auto epochTime = MemberIdTraits::TimePointFormaterT::getTimeValue(value);
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
  } else if constexpr (IsOptionalC<TargetT, MemberIdTraits>) {
    auto result = getValueFor<typename TargetT::value_type, MemberIdTraits>(value);
    if (!result) {
      return std::unexpected(result.error());
    }
    return result.value();
  } else if constexpr (is_scaled_int<TargetT>) {
    return TargetT{value};
  }
}

template <typename TargetT, typename MemberIdTraits>
std::expected<bool, std::string> getValueFor(bool value) {
  return value;
}

template <typename MemberT, MemberIdTraitsC MemberIdTraits> struct Handler {
  using MemberType = MemberT;
  using MemberIdType = typename MemberIdTraits::MemberIdType;
};

template <typename T, MemberIdTraitsC MemberIdTraits>
struct ValueHandlerBase {};

template <typename T, MemberIdTraitsC MemberIdTraits>
  requires(IsMOPEDCompositeC<T, MemberIdTraits>)
struct ValueHandlerBase<T, MemberIdTraits> {

  using ValueTypeHandlerT =
      std::decay_t<decltype(T::template getMOPEDHandler<MemberIdTraits>())>;

  ValueTypeHandlerT _valueTypeHandler{
      getMOPEDHandlerForParser<T, MemberIdTraits>()};
};

template <IsMOPEDPushCollectionC MemberT, MemberIdTraitsC MemberIdTraits>
struct Handler<MemberT, MemberIdTraits>
    : IMOPEDHandler<MemberIdTraits>,
      ValueHandlerBase<typename MemberT::value_type, MemberIdTraits> {
  using MemberIdType = typename MemberIdTraits::MemberIdType;

  using MemberType = MemberT;
  using ValueType = typename MemberT::value_type;
  static constexpr bool HasCompositeValueType =
      IsMOPEDCompositeC<typename MemberT::value_type, MemberIdTraits>;
  using MOPEDHandlerStack = std::stack<IMOPEDHandler<MemberIdTraits> *>;

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
      auto result = getValueFor<ValueType, MemberIdTraits>(value);
      if (!result) {
        return std::unexpected(result.error());
      }
      _targetCollection->emplace_back(result.value());
    }
    return {};
  }

  MemberT *_targetCollection;
};

template <IsMOPEDCompositeDispatcherC MemberT, MemberIdTraitsC MemberIdTraits>
struct Handler<MemberT, MemberIdTraits>
    : IMOPEDHandler<MemberIdTraits>,
      ValueHandlerBase<typename MemberT::value_type, MemberIdTraits> {
  using MemberIdType = typename MemberIdTraits::MemberIdType;

  using MemberType = MemberT;
  using ValueType = typename MemberT::value_type;
  static constexpr bool HasCompositeValueType =
      IsMOPEDCompositeC<typename MemberT::value_type, MemberIdTraits>;
  using MOPEDHandlerStack = std::stack<IMOPEDHandler<MemberIdTraits> *>;

  Expected onMember(MOPEDHandlerStack &eventHandlerStack,
                    MemberIdType memberId) override {
    return std::unexpected("Parse error!!! onObjectFinish event not expected"
                           " in dispatching handler");
  }

  Expected onObjectStart(MOPEDHandlerStack &eventHandlerStack) override {
    _dispatcher->dispatcherLastCapture();
    this->_valueTypeHandler.setTargetMember(_dispatcher->resetCapture());
    return this->_valueTypeHandler.onObjectStart(eventHandlerStack);
  }

  Expected onObjectFinish(MOPEDHandlerStack &handlerStack) override {
    return std::unexpected("Parse error!!! onObjectFinish event not expected"
                           " in dispatching handler");
  }

  Expected onArrayStart(MOPEDHandlerStack &handlerStack) override {
    handlerStack.push(this);
    return {};
  }

  Expected onArrayFinish(MOPEDHandlerStack &handlerStack) override {
    _dispatcher->dispatcherLastCapture();
    handlerStack.pop();
    return {};
  }

  Expected onStringValue(std::string_view value) override {
    HandleScalarValue(value);
    return {};
  }

  Expected onNumericValue(std::string_view value) override {
    HandleScalarValue(value);
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
      auto result = getValueFor<ValueType, MemberIdTraits>(value);
      if (!result) {
        return std::unexpected(result.error());
      }
      _dispatcher->setCurrentValue(result.value());
      _dispatcher->dispatcherLastCapture();
    }
    return {};
  }

  MemberT *_dispatcher;
};

template <is_array MemberT, MemberIdTraitsC MemberIdTraits>
struct Handler<MemberT, MemberIdTraits>
    : IMOPEDHandler<MemberIdTraits>,
      ValueHandlerBase<typename MemberT::value_type, MemberIdTraits> {

  using MemberType = MemberT;
  using ValueType = typename MemberT::value_type;
  static constexpr bool HasCompositeValueType =
      IsMOPEDCompositeC<typename MemberT::value_type, MemberIdTraits>;

  using MemberIdT = typename MemberIdTraits::MemberIdType;
  using MOPEDHandlerStack = std::stack<IMOPEDHandler<MemberIdTraits> *>;

  Expected onMember(MOPEDHandlerStack &eventHandlerStack,
                    MemberIdT memberId) override {
    return std::unexpected("Parse error!!! onMember event not expected in "
                           "array handler");
  }

  Expected onObjectStart(MOPEDHandlerStack &eventHandlerStack) override {

    if constexpr (HasCompositeValueType) {
      if (_currentIndex >= std::size(_targetArray)) {
        return std::unexpected("Index out of bounds");
      }

      this->_valueTypeHandler.setTargetMember(_targetArray[_currentIndex]);
      auto result = this->_valueTypeHandler.onObjectStart(eventHandlerStack);
      if (!result) {
        return result;
      }
    } else {
      return std::unexpected(
          "Parse error!!! onObjectStart event not expected in array "
          "with scalar value types");
    }
    ++_currentIndex;
  }

  virtual Expected onObjectFinish(MOPEDHandlerStack &handlerStack) {
    return std::unexpected(
        "Parse error!!! onObjectFinish event not expected for array handler");
  }

  virtual Expected onArrayStart(MOPEDHandlerStack &handlerStack) {
    handlerStack.push(this);
    _currentIndex = 0;
    return {};
  }

  virtual Expected onArrayFinish(MOPEDHandlerStack &handlerStack) {
    handlerStack.pop();
    return {};
  }

  Expected onStringValue(std::string_view value) override {
    if (_currentIndex >= std::size(_targetArray)) {
      return std::unexpected("Index out of bounds");
    }
    auto result = getValueFor<ValueType, MemberIdTraits>(value);
    if (!result) {
      return std::unexpected(result.error());
    }
    _targetArray[_currentIndex] = result.value();
    ++_currentIndex;
    return {};
  }

  Expected onNumericValue(std::string_view value) override {
    if (_currentIndex >= std::size(_targetArray)) {
      return std::unexpected("Index out of bounds");
    }
    auto result = getValueFor<MemberT, MemberIdTraits>(value);
    if (!result) {
      return std::unexpected(result.error());
    }
    _targetArray[_currentIndex] = result.value();
    ++_currentIndex;
    return {};
  }

  void setTargetMember(MemberT &targetMember) { _targetArray = &targetMember; }

  void applyEmitterContext(auto &emitterContext,
                           std::optional<MemberIdT> memberId) {
    emitterContext.onArrayStart(memberId, _targetArray->size());

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
      auto result = getValueFor<ValueType, MemberIdTraits>(value);
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

template <IsMOPEDMapCollectionC MemberT, MemberIdTraitsC MemberIdTraits>
struct Handler<MemberT, MemberIdTraits>
    : IMOPEDHandler<MemberIdTraits>,
      ValueHandlerBase<typename MemberT::mapped_type, MemberIdTraits> {
  using MemberIdType = typename MemberIdTraits::MemberIdType;

  using MemberType = MemberT;
  using KeyType = typename MemberT::key_type;
  using MappedType = typename MemberT::value_type;
  static constexpr bool HasCompositeMappedType =
      IsMOPEDCompositeC<typename MemberT::mapped_type, MemberIdTraits>;
  using MOPEDHandlerStack = std::stack<IMOPEDHandler<MemberIdTraits> *>;

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
      auto result = getValueFor<MappedType, MemberIdTraits>(value);
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

template <typename MemberT, MemberIdTraitsC MemberIdTraits>
  requires(IsOptionalC<MemberT, MemberIdTraits>)
struct Handler<MemberT, MemberIdTraits>
    : Handler<typename MemberT::value_type, MemberIdTraits> {

  using MemberType = MemberT;
  using MemberIdT = typename MemberIdTraits::MemberIdType;

  void setTargetMember(MemberT &targetMember) {
    targetMember = MemberT{};
    Handler<typename MemberT::value_type, MemberIdTraits>::setTargetMember(
        targetMember.value());
  }
};

template <typename MemberT, MemberIdTraitsC MemberIdTraits>
  requires(IsMOPEDCompositeC<MemberT, MemberIdTraits>)
struct Handler<MemberT, MemberIdTraits>
    : std::decay_t<
          decltype(getMOPEDHandlerForParser<MemberT, MemberIdTraits>())> {
  using HandlerBase = std::decay_t<
      decltype(getMOPEDHandlerForParser<MemberT, MemberIdTraits>())>;
  using MemberType = MemberT;
  using MemberIdT = typename MemberIdTraits::MemberIdType;

  Handler()
      : HandlerBase(MemberT::template getMOPEDHandler<MemberIdTraits>()) {}
};

template <MemberIdTraitsC MemberIdTraits, typename T>
struct FinalMultiMopedHandlerHarness {
  using HandlerT =
      std::decay_t<decltype(getMOPEDHandlerForParser<MemberIdTraits, T>())>;
  HandlerT handler;
  template <typename TargetType> auto &getHandlerForType() {
    if constexpr (std::is_same_v<TargetType, T>) {
      return handler;
    } else {
      static_assert(false, "Type not found in MultiMopedHandlerHarness");
    }
  }
};

template <MemberIdTraitsC MemberIdTraits, typename T, typename... MOPEDTypes>
struct MultiMopedHandlerHarness;

template <MemberIdTraitsC MemberIdTraits, typename T, typename... MOPEDTypes>
constexpr auto getMultiMopedHandlerHarness() {
  if constexpr (sizeof...(MOPEDTypes) == 1) {
    return FinalMultiMopedHandlerHarness<MemberIdTraits, T>{};
  } else {
    return MultiMopedHandlerHarness<MemberIdTraits, T, MOPEDTypes...>{};
  }
};

template <MemberIdTraitsC MemberIdTraits, typename T, typename... MOPEDTypes>
struct MultiMopedHandlerHarness {
  using HandlerT =
      std::decay_t<decltype(getMOPEDHandlerForParser<MemberIdTraits, T>())>;
  using TailHandlersT = std::decay_t<
      decltype(getMultiMopedHandlerHarness<MemberIdTraits, MOPEDTypes...>())>;
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

template <MemberIdTraitsC MemberIdTraits, typename... MOPEDTypes>
constexpr auto
getMultiMopedHandlerHarnessFromVariant(std::variant<MOPEDTypes...>) {
  return getMultiMopedHandlerHarness<MemberIdTraits, MOPEDTypes...>();
};

template <is_variant VariantT, MemberIdTraitsC MemberIdTraits>
struct Handler<VariantT, MemberIdTraits> : IMOPEDHandler<MemberIdTraits> {

  using MemberType = VariantT;
  using MemberIdT = typename MemberIdTraits::MemberIdType;
  using MultiMopedHandlerHarness =
      std::decay_t<decltype(getMultiMopedHandlerHarnessFromVariant<
                            MemberIdTraits>(VariantT{}))>;

  using MOPEDHandlerStack = std::stack<IMOPEDHandler<MemberIdTraits> *>;
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
          MemberIdTraitsC MemberIdTraits>
struct MemberIdHandlerPair {
  using MemberIdT = typename MemberIdTraits::MemberIdType;
  MemberIdT memberId;
  MemberPtrT memberPtr;
  using MemberT = std::decay_t<decltype(std::declval<CaptureT>().*
                                        std::declval<MemberPtrT>())>;

  Expected setValue(CaptureT &setTarget, auto value) {
    if constexpr (IsSettableC<MemberT, MemberIdTraits> && requires() {
                    setTarget.*memberPtr = getValueFor<MemberT, MemberIdTraits>(value).value();
                  }) {
      auto result = getValueFor<MemberT, MemberIdTraits>(value);
      if (!result) {
        return std::unexpected(result.error());
      }
      setTarget.*memberPtr = result.value();
    }
    return {};
  }

  void applyEmitterContext(auto &emitterContext, CaptureT &captureTarget) {
    auto &memberValue = captureTarget.*memberPtr;
    if constexpr (IMOPEDHandlerC<decltype(handler), MemberIdTraits>) {
      handler.setTargetMember(memberValue);
      handler.applyEmitterContext(emitterContext, memberId);
    } else {
      emitterContext.onObjectValueEntry(memberId, memberValue);
    }
  }

  auto &getMember(CaptureT &getTarget) { return getTarget.*memberPtr; }

  Handler<MemberT, MemberIdTraits> handler;
};

template <typename CaptureType, typename MemberEventHandlerTuple,
          MemberIdTraitsC MemberIdTraits>
struct MappedObjectParserEncoderDispatcher : IMOPEDHandler<MemberIdTraits> {

  using MOPEDHandlerStack = std::stack<IMOPEDHandler<MemberIdTraits> *>;
  using MemberIdT = typename MemberIdTraits::MemberIdType;

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
      eventHandlerStack.push(this);
      return {};
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

  Expected onArrayFinish(MOPEDHandlerStack &handlerStack) { return {}; }

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
      } else if (nameHandler.memberId == MemberIdTraits::EmbeddingMemberId) {
        if constexpr (IMOPEDHandlerC<
                          std::decay_t<decltype(nameHandler.handler)>,
                          MemberIdTraits>) {
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
        std::get<MemberIndex>(_handlerTuple).setValue(*_captureObject, value);
        return {};
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
                                   MemberIdTraits>) {
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

template <MemberIdTraitsC MemberIdTraits, typename CaptureT>
constexpr auto mopedTuple() {
  return std::tuple<>{};
}

template <MemberIdTraitsC MemberIdTraits, typename CaptureT, typename... Args>
constexpr auto mopedTuple(std::string_view name, auto memberPtr,
                          Args &&...args) {
  using MemberIdT = typename MemberIdTraits::MemberIdType;

  using MemberPtrT = decltype(memberPtr);
  auto handlerTuple =
      std::make_tuple(MemberIdHandlerPair<CaptureT, MemberPtrT, MemberIdTraits>{
          MemberIdTraits::getMemberId(name), memberPtr});
  return std::tuple_cat(handlerTuple, mopedTuple<MemberIdTraits, CaptureT>(
                                          std::forward<Args>(args)...));
}

template <MemberIdTraitsC MemberIdTraits, typename CaptureT, typename... Args>
auto mopedHandler(Args &&...args) {
  using MemberIdT = typename MemberIdTraits::MemberIdType;
  auto handlerTuple =
      mopedTuple<MemberIdTraits, CaptureT>(std::forward<Args>(args)...);
  using HandlerTupleT = decltype(handlerTuple);
  return MappedObjectParserEncoderDispatcher<CaptureT, HandlerTupleT,
                                             MemberIdTraits>{
      std::move(handlerTuple)};
}

} // namespace moped