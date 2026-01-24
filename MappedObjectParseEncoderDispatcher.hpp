#pragma once

#include "moped/AutoTypeSelectingParserHandler.hpp"
#include "moped/concepts.hpp"
#include "moped/getValueFor.hpp"
#include <tuple>

namespace moped {

template <typename MemberT, DecodingTraitsC DecodingTraits> struct Handler {
  using MemberType = MemberT;
  using MemberIdType = typename DecodingTraits::MemberIdType;
};

template <typename T, DecodingTraitsC DecodingTraits>
struct ValueHandlerBase {};

template <typename T, DecodingTraitsC DecodingTraits>
  requires(IsMOPEDCompositeC<T, DecodingTraits>)
struct ValueHandlerBase<T, DecodingTraits> {

  using ValueTypeHandlerT = std::decay_t<
      decltype(MopedHandlerManager<T, DecodingTraits>::getHandler())>;

  ValueTypeHandlerT _valueTypeHandler{
      getMOPEDHandlerForParser<T, DecodingTraits>()};
};

template <IsMOPEDContentCollectionC T, DecodingTraitsC DecodingTraits>
struct ValueHandlerBase<T, DecodingTraits> {
  using ValueTypeHandlerT = Handler<T, DecodingTraits>;
  ValueTypeHandlerT _valueTypeHandler{};
};

template <is_variant T, DecodingTraitsC DecodingTraits>
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
      IsMOPEDCompositeC<typename MemberT::value_type, DecodingTraits> ||
      is_variant<typename MemberT::value_type>;
  using MOPEDHandlerStack = std::stack<IMOPEDHandler<DecodingTraits> *>;

  Expected onMember(MOPEDHandlerStack &, MemberIdType member) override {
    return std::unexpected{ParseError{
        "Parse error!!! onMember parse event not expected"
        " in collection handler. Failed to push value hander on parse stack",
        member}};
  }

  Expected onObjectStart(MOPEDHandlerStack &eventHandlerStack) override {
    if constexpr (HasCompositeValueType) {
      _targetCollection->emplace_back();
      this->_valueTypeHandler.setTargetMember(_targetCollection->back());
      return this->_valueTypeHandler.onObjectStart(eventHandlerStack);
    }
    return {};
  }

  Expected onObjectFinish(MOPEDHandlerStack &) override {
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

template <IsMOPEDInsertCollectionC MemberT, DecodingTraitsC DecodingTraits>
struct Handler<MemberT, DecodingTraits>
    : IMOPEDHandler<DecodingTraits>,
      ValueHandlerBase<typename MemberT::value_type, DecodingTraits> {
  using MemberIdType = typename DecodingTraits::MemberIdType;

  using MemberType = MemberT;
  using ValueType = typename MemberT::value_type;
  static constexpr bool HasCompositeValueType =
      IsMOPEDCompositeC<typename MemberT::value_type, DecodingTraits>;
  using MOPEDHandlerStack = std::stack<IMOPEDHandler<DecodingTraits> *>;

  Expected onMember(MOPEDHandlerStack &, MemberIdType memberId) override {
    return std::unexpected{ParseError{
        "Parse error!!! onMember parse event not expected"
        " in collection handler. Failed to push value hander on parse stack",
        memberId}};
  }

  Expected onObjectStart(MOPEDHandlerStack &eventHandlerStack) override {
    if constexpr (HasCompositeValueType) {
      std::destroy_at<ValueType>(&_reusableValue);
      std::construct_at<ValueType>(&_reusableValue);
      this->_valueTypeHandler.setTargetMember(_reusableValue);
      return this->_valueTypeHandler.onObjectStart(eventHandlerStack);
    }
    return {};
  }

  Expected onObjectFinish(MOPEDHandlerStack &) override {
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
    if constexpr (HasCompositeValueType) {
      _targetCollection->insert(std::move(_reusableValue));
    }
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
      _targetCollection->insert(result.value());
    }
    return {};
  }
  ValueType _reusableValue{};
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
      if (auto result = _dispatcher->dispatchLastCapture(); !result) {
        return result;
      }
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
        if (auto result = _dispatcher->dispatchLastCapture(); !result) {
          return result;
        }
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
    if (auto result = _dispatcher->dispatchLastCapture(); !result) {
      return result;
    }
    handlerStack.pop();
    _currentIndex = 0;
    return {};
  }

  Expected onStringValue(std::string_view value) override {
    if (_currentIndex > 0) {
      if (auto result = _dispatcher->dispatchLastCapture(); !result) {
        return result;
      }
    }
    HandleScalarValue(value);
    _currentIndex++;
    return {};
  }

  Expected onNumericValue(std::string_view value) override {
    if (_currentIndex > 0) {
      if (auto result = _dispatcher->dispatchLastCapture(); !result) {
        return result;
      }
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
      if (auto result = _dispatcher->dispatchLastCapture(); !result) {
        return result;
      }
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

      auto &currentElement = (*_targetArray)[_currentIndex++];
      this->_valueTypeHandler.setTargetMember(currentElement);
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

        this->_valueTypeHandler.setTargetMember(
            (*_targetArray)[_currentIndex++]);
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
  using MappedType = typename MemberT::mapped_type;
  static constexpr bool HasCompositeMappedType =
      IsMOPEDCompositeC<MappedType, DecodingTraits> || is_variant<MappedType>;

  using MOPEDHandlerStack = std::stack<IMOPEDHandler<DecodingTraits> *>;

  Expected onMember(MOPEDHandlerStack &, MemberIdType memberId) override {
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
      auto entryResult = _targetCollection->emplace(_currentKey, MappedType{});

      if constexpr (requires {
                      this->_valueTypeHandler.setTargetMember(
                          entryResult.first->second);
                    }) {
        this->_valueTypeHandler.setTargetMember(entryResult.first->second);
      } else {
        this->_valueTypeHandler.setTargetMember(entryResult->second);
      }
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
      static_assert(!is_variant<MappedType>,
                    "Variant mapped types require composite handler support");
      auto result = getValueFor<MappedType, DecodingTraits>(value);
      if (!result) {
        return std::unexpected(result.error());
      }
      _targetCollection->emplace(_currentKey, result.value());
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
    targetMember = typename MemberT::value_type{};
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
      : HandlerBase{
            MopedHandlerManager<MemberT, DecodingTraits>::getHandler()} {}
};

template <DecodingTraitsC DecodingTraits, typename... MOPEDTypes>
constexpr auto
getMultiTypeHandlerHarnessFromVariant(std::variant<MOPEDTypes...>) {
  return AutoTypeSelectingParserHandler<
      CandidateTypeHarness<DecodingTraits, MOPEDTypes...>>{};
};

template <is_variant VariantT, DecodingTraitsC DecodingTraits>
struct Handler<VariantT, DecodingTraits> : IMOPEDHandler<DecodingTraits> {

  using MemberType = VariantT;
  using MemberIdT = typename DecodingTraits::MemberIdType;
  using MultiMopedHandlerHarness =
      std::decay_t<decltype(getMultiTypeHandlerHarnessFromVariant<
                            DecodingTraits>(VariantT{}))>;

  using MOPEDHandlerStack = std::stack<IMOPEDHandler<DecodingTraits> *>;

  Expected onObjectStart(MOPEDHandlerStack &eventHandlerStack) override {
    if (_nestingLevel == 0) {
      eventHandlerStack.push(this);
    }
    ++_nestingLevel;
    return _handlerHarness.onObjectStart();
  }

  Expected onObjectFinish(MOPEDHandlerStack &handlerStack) override {
    --_nestingLevel;
    auto result = _handlerHarness.onObjectFinish();
    if (!result) {
      _handlerHarness.reset();
      return result;
    }
    if (_nestingLevel == 0) {
      handlerStack.pop();
      return _handlerHarness.applyHandler([this](auto &selected) {
        *_captureVariant = selected;
        _handlerHarness.reset();
        return Expected{};
      });
    }
    return Expected{};
  }

  Expected onMember(MOPEDHandlerStack &, MemberIdT memberId) override {
    return _handlerHarness.onMember(memberId);
  }

  Expected onArrayStart(MOPEDHandlerStack &) {
    return _handlerHarness.onArrayStart();
  }

  Expected onArrayFinish(MOPEDHandlerStack &) {
    return _handlerHarness.onArrayFinish();
  }

  Expected onStringValue(std::string_view value) {
    return _handlerHarness.onStringValue(value);
  }

  Expected onNumericValue(std::string_view value) {
    return _handlerHarness.onNumericValue(value);
  }

  Expected onBooleanValue(bool value) {
    return _handlerHarness.onBooleanValue(value);
  }

  Expected onNullValue() { return _handlerHarness.onNullValue(); }

  Expected onRawBinary(void *) { return _handlerHarness.onNullValue(); }

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

  void setTargetMember(VariantT &targetMember) {
    _captureVariant = &targetMember;
  }

private:
  int _nestingLevel{0};
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

  Expected onNullValue() override { return applyNullValueToActiveMember<0>(); }

  void setTargetMember(CaptureType &targetMember) {
    _captureObject = &targetMember;
    _activeMemberOffset.reset();
  }

  void reset() { _activeMemberOffset.reset(); }

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

  template <typename BaseCaptureType, typename BaseMemberEventHandlerTuple>
  auto operator+(MappedObjectParserEncoderDispatcher<
                 BaseCaptureType, BaseMemberEventHandlerTuple, DecodingTraits>
                     &&baseObjectParserEncoderDispatcher) {
    static_assert(std::is_base_of_v<BaseCaptureType, CaptureType>,
                  "Base capture type must be a base class of the derived "
                  "capture type.");

    using NewMemberEventHandlerTuple =
        decltype(std::tuple_cat(std::declval<BaseMemberEventHandlerTuple>(),
                                std::declval<MemberEventHandlerTuple>()));
    return MappedObjectParserEncoderDispatcher<
        CaptureType, NewMemberEventHandlerTuple, DecodingTraits>{std::tuple_cat(
        baseObjectParserEncoderDispatcher.getHandlerTuple(), _handlerTuple)};
  }

  auto getHandlerTuple() { return _handlerTuple; }

private:
  std::string_view _activeMemberName;
  template <size_t MemberIndex>
  Expected setActiveMember(MemberIdT memberId,
                           MOPEDHandlerStack &eventHandlerStack) {
    if constexpr (MemberIndex == std::tuple_size_v<MemberEventHandlerTuple>) {
      // Failed parse ... no member found
      _activeMemberOffset.reset();
      return std::unexpected(
          ParseError{"Member not found in MOPED handler", memberId});
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
        return std::unexpected(
            ParseError{"No active member available for current value",
                       std::format("{}", value)});
      }
      if (MemberIndex == *_activeMemberOffset) {
        return std::get<MemberIndex>(_handlerTuple)
            .setValue(*_captureObject, value);
      }
      return setActiveMemberValue<MemberIndex + 1>(value);
    }
  }

  template <size_t MemberIndex> Expected applyNullValueToActiveMember() {
    if constexpr (MemberIndex == std::tuple_size_v<MemberEventHandlerTuple>) {
      return std::unexpected("Parse error ... no active member available");
    } else {
      if (!_activeMemberOffset.has_value()) {
        return std::unexpected("Parse error ... no active member available for "
                               "current value null");
      }
      if (MemberIndex == *_activeMemberOffset) {
        auto &memberValue =
            std::get<MemberIndex>(_handlerTuple).getMember(*_captureObject);
        if constexpr (IsOptionalC<std::decay_t<decltype(memberValue)>,
                                  DecodingTraits>) {
          memberValue.reset();
          return {};
        } else {
          return std::unexpected("Parse error ... null value not applicable "
                                 "for non-optional member");
        }
      }
      return applyNullValueToActiveMember<MemberIndex + 1>();
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
      if (MemberIndex == *_activeMemberOffset) {
        auto &nameHandler = std::get<MemberIndex>(_handlerTuple);
        if constexpr (IMOPEDHandlerC<
                          std::decay_t<decltype(nameHandler.handler)>,
                          DecodingTraits>) {

          nameHandler.handler.setTargetMember(
              _captureObject->*(nameHandler.memberPtr));
          return handlerActionFunction(nameHandler.handler);
        } else {
          return std::unexpected(ParseError{
              "Expected composite handler for member:", nameHandler.memberId});
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