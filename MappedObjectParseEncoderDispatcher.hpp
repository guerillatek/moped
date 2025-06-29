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

inline std::optional<TimePoint> getEpochTimeValue(std::string_view input) {
  // Make sure the input is numeric
  if (input.empty() || !std::all_of(input.begin(), input.end(),
                                    [](char c) { return std::isdigit(c); })) {
    return std::nullopt; // Invalid input
  }

  // Convert string to 64-bit integer
  auto durationValue =
      ScaledInteger<std::uint64_t, 0>{input}.getRawIntegerValue();
  size_t digits = input.size();
  using namespace std::chrono;

  if (digits <= 10) {
    return time_point<system_clock, nanoseconds>(seconds(durationValue));
  } else if (digits <= 13) {
    return time_point<system_clock, nanoseconds>(milliseconds(durationValue));
  } else if (digits <= 16) {
    return time_point<system_clock, nanoseconds>(microseconds(durationValue));
  } else if (digits <= 19) {
    return time_point<system_clock, nanoseconds>(nanoseconds(durationValue));
  } else {
    return std::nullopt; // Invalid input
  }
}

template <typename TargetT> auto getValueFor(std::string_view value) {
  if constexpr (std::is_same_v<TargetT, bool>) {
    if (value == "true" || value == "1") {
      return true;
    } else if (value == "false" || value == "0") {
      return false;
    } else {
      throw std::runtime_error("Invalid boolean value: " + std::string{value});
    }
  } else if constexpr (std::is_same_v<TargetT, TimePoint>) {
    auto epochTime = getEpochTimeValue(value);
    if (!epochTime) {
      throw std::runtime_error("Invalid epoch time value: " +
                               std::string{value});
    }
    return *epochTime;
  } else if constexpr (std::is_same_v<TargetT, std::string> ||
                       std::is_same_v<TargetT, std::string_view>) {
    return value;
  } else if constexpr ((std::is_integral_v<TargetT>) ||
                       (std::is_floating_point_v<TargetT>)) {
    return ston<TargetT>(value);
  } else if constexpr (is_optional<TargetT>) {
    return getValueFor<typename TargetT::value_type>(value);
  } else if constexpr (is_scaled_int<TargetT>) {
    return TargetT{value};
  }
}

template <typename TargetT> auto getValueFor(bool value) { return value; }

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

  void onMember(MOPEDHandlerStack &eventHandlerStack,
                MemberIdType memberId) override {
    throw std::runtime_error("Parse error!!! onObjectFinish event not expected"
                             " in collection handler");
  }

  void onObjectStart(MOPEDHandlerStack &eventHandlerStack) override {
    if constexpr (HasCompositeValueType) {
      _targetCollection->emplace_back();
      this->_valueTypeHandler.setTargetMember(_targetCollection->back());
      this->_valueTypeHandler.onObjectStart(eventHandlerStack);
    }
  }

  void onObjectFinish(MOPEDHandlerStack &handlerStack) override {
    throw std::runtime_error("Parse error!!! onObjectFinish event not expected"
                             " in collection handler");
  }

  void onArrayStart(MOPEDHandlerStack &handlerStack) override {
    handlerStack.push(this);
  }
  void onArrayFinish(MOPEDHandlerStack &handlerStack) override {
    handlerStack.pop();
  }

  void onStringValue(std::string_view value) override {
    HandleScalarValue(value);
  }

  void onNumericValue(std::string_view value) override {
    HandleScalarValue(value);
  }

  void setTargetMember(MemberT &targetMember) {
    _targetCollection = &targetMember;
  }

private:
  void HandleScalarValue(std::string_view value) {
    if constexpr (HasCompositeValueType) {
      throw std::runtime_error(
          "Parse error!!! No active composite handler to set value");
    } else {
      _targetCollection->emplace_back(getValueFor<ValueType>(value));
    }
  }

  MemberT *_targetCollection;
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

  void onMember(MOPEDHandlerStack &eventHandlerStack,
                MemberIdT memberId) override {
    throw std::runtime_error("Parse error!!! onMember event not expected in "
                             "array handler");
  }

  void onObjectStart(MOPEDHandlerStack &eventHandlerStack) override {

    if constexpr (HasCompositeValueType) {
      if (_currentIndex >= std::size(_targetArray)) {
        throw std::runtime_error("Index out of bounds");
      }

      this->_valueTypeHandler.setTargetMember(_targetArray[_currentIndex]);
      this->_valueTypeHandler.onObjectStart(eventHandlerStack);
    } else {
      throw std::runtime_error(
          "Parse error!!! onObjectStart event not expected in array "
          "with scalar value types");
    }
    ++_currentIndex;
  }

  virtual void onObjectFinish(MOPEDHandlerStack &handlerStack) {
    throw std::runtime_error(
        "Parse error!!! onObjectFinish event not expected for array handler");
  }

  virtual void onArrayStart(MOPEDHandlerStack &handlerStack) {
    handlerStack.push(this);
    _currentIndex = 0;
  }

  virtual void onArrayFinish(MOPEDHandlerStack &handlerStack) {
    handlerStack.pop();
  }

  void onStringValue(std::string_view value) override {
    if (_currentIndex >= std::size(_targetArray)) {
      throw std::runtime_error("Index out of bounds");
    }
    _targetArray[_currentIndex] = getValueFor<MemberT>(value);
    ++_currentIndex;
  }

  void onNumericValue(std::string_view value) override {
    if (_currentIndex >= std::size(_targetArray)) {
      throw std::runtime_error("Index out of bounds");
    }
    _targetArray[_currentIndex] = getValueFor<MemberT>(value);
    ++_currentIndex;
  }

  void setTargetMember(MemberT &targetMember) { _targetArray = &targetMember; }

private:
  void HandleScalarValue(std::string_view value) {
    if constexpr (HasCompositeValueType) {
      throw std::runtime_error(
          "Parse error!!! No active composite handler to set value");
    } else {
      _targetArray[_currentIndex] = getValueFor<MemberT>(value);
      ++_currentIndex;
    }
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

  void onMember(MOPEDHandlerStack &eventHandlerStack,
                MemberIdType memberId) override {
    _currentKey = memberId;
  }

  void onObjectStart(MOPEDHandlerStack &eventHandlerStack) override {
    if (!_addingContent) {
      _addingContent = true;
      eventHandlerStack.push(this);
      return;
    }
    if constexpr (HasCompositeMappedType) {
      this->_valueTypeHandler.setTargetMember(
          (*_targetCollection)[_currentKey]);
      this->_valueTypeHandler.onObjectStart(eventHandlerStack);
    } else {
      throw std::runtime_error(
          "Parse error!!! onObjectStart event not expected in collection "
          "with scalar value types");
    }
  }

  void onObjectFinish(MOPEDHandlerStack &handlerStack) override {
    handlerStack.pop();
    _addingContent = false;
  }

  void onArrayStart(MOPEDHandlerStack &handlerStack) override {
    handlerStack.push(this);
  }
  void onArrayFinish(MOPEDHandlerStack &handlerStack) override {
    handlerStack.pop();
    _addingContent = false;
  }

  void onStringValue(std::string_view value) override {
    HandleScalarValue(value);
  }

  void onNumericValue(std::string_view value) override {
    HandleScalarValue(value);
  }

  void setTargetMember(MemberT &targetMember) {
    _targetCollection = &targetMember;
  }

private:
  void HandleScalarValue(std::string_view value) {
    if constexpr (HasCompositeMappedType) {
      throw std::runtime_error(
          "Parse error!!! No active composite handler to set value");
    } else {
      (*_targetCollection)[_currentKey] = getValueFor<MappedType>(value);
    }
  }

  MemberT *_targetCollection;
  KeyType _currentKey;
  bool _addingContent{false};
};

template <is_optional MemberT, MemberIdTraitsC MemberIdTraits>
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

template <typename CaptureT, typename MemberPtrT,
          MemberIdTraitsC MemberIdTraits>
struct MemberIdHandlerPair {
  using MemberIdT = typename MemberIdTraits::MemberIdType;
  MemberIdT memberId;
  MemberPtrT memberPtr;
  using MemberT = std::decay_t<decltype(std::declval<CaptureT>().*
                                        std::declval<MemberPtrT>())>;

  void setValue(CaptureT &setTarget, auto value) {
    if constexpr (IsSettable<MemberT> && requires() {
                    setTarget.*memberPtr = getValueFor<MemberT>(value);
                  }) {

      setTarget.*memberPtr = getValueFor<MemberT>(value);
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

  void onObjectStart(MOPEDHandlerStack &eventHandlerStack) override {
    if (!_activeMemberOffset.has_value()) {
      eventHandlerStack.push(this);
      return;
    }

    dispatchForActiveMember<0>([&eventHandlerStack](auto &handler) {
      handler.onObjectStart(eventHandlerStack);
    });
  }

  void onObjectFinish(MOPEDHandlerStack &eventHandlerStack) override {
    _activeMemberOffset.reset();
    eventHandlerStack.pop();
  }

  void onArrayStart(MOPEDHandlerStack &eventHandlerStack) override {
    if (!_activeMemberOffset.has_value()) {
      eventHandlerStack.push(this);
      return;
    }

    dispatchForActiveMember<0>([&eventHandlerStack](auto &handler) {
      handler.onArrayStart(eventHandlerStack);
    });
  }

  void onMember(MOPEDHandlerStack &eventHandlerStack,
                MemberIdT memberId) override {
    setActiveMember<0>(memberId, eventHandlerStack);
  }

  void onStringValue(std::string_view value) override {
    setActiveMemberValue<0>(value);
  }

  void onNumericValue(std::string_view value) {
    setActiveMemberValue<0>(value);
  }

  void onBooleanValue(bool value) override { setActiveMemberValue<0>(value); }

  void setTargetMember(CaptureType &targetMember) {
    _captureObject = &targetMember;
  }

  void onArrayFinish(MOPEDHandlerStack &handlerStack) {}

private:
  template <size_t MemberIndex>
  void setActiveMember(MemberIdT memberId,
                       MOPEDHandlerStack &eventHandlerStack) {
    if constexpr (MemberIndex == std::tuple_size_v<MemberEventHandlerTuple>) {
      throw std::runtime_error(
          std::format("Member, '{}' not found in MOPED handler", memberId));
    } else {
      auto &nameHandler = std::get<MemberIndex>(_handlerTuple);
      if (nameHandler.memberId == memberId) {
        _activeMemberOffset = MemberIndex;
        return;
      }
      return setActiveMember<MemberIndex + 1>(memberId, eventHandlerStack);
    }
  }

  template <size_t MemberIndex> void setActiveMemberValue(auto value) {
    if constexpr (MemberIndex == std::tuple_size_v<MemberEventHandlerTuple>) {
      throw std::runtime_error("Parse error ... no active member available");
    } else {
      if (!_activeMemberOffset.has_value()) {
        throw std::runtime_error(std::format(
            "Parse error ... no active member available for current value {}",
            value));
      }
      if (MemberIndex == *_activeMemberOffset) {
        std::get<MemberIndex>(_handlerTuple).setValue(*_captureObject, value);
        return;
      }
      return setActiveMemberValue<MemberIndex + 1>(value);
    }
  }

  template <size_t MemberIndex, typename H>
  void dispatchForActiveMember(H &&handlerActionFunction) {
    if constexpr (MemberIndex == std::tuple_size_v<MemberEventHandlerTuple>) {
      throw std::runtime_error("Parse error ... no active member available");
    } else {
      if (!_activeMemberOffset.has_value()) {
        throw std::runtime_error(
            "Parse error ... no active member available for current object");
      }
      auto &nameHandler = std::get<MemberIndex>(_handlerTuple);
      if constexpr (IMOPEDHandlerC<std::decay_t<decltype(nameHandler.handler)>,
                                   MemberIdTraits>) {
        if (MemberIndex == *_activeMemberOffset) {
          nameHandler.handler.setTargetMember(
              _captureObject->*(nameHandler.memberPtr));
          handlerActionFunction(nameHandler.handler);
          return;
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