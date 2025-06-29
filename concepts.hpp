#pragma once

#include <chrono>
#include <concepts>
#include <expected>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace moped {

using Expected = std::expected<void, std::string>;
using TimePoint = std::chrono::system_clock::time_point;

template <typename T>
concept MemberIdTraitsC = requires(T t) {
  {
    T::getMemberId(std::string_view{})
  } -> std::same_as<typename T::MemberIdType>;

  {
    T::getDisplayName(typename T::MemberIdType{})
  } -> std::same_as<std::string>;
};

template <typename T>
concept PayloadHandlerC = requires(T t) {
  { t(std::string{}) } -> std::same_as<Expected>;
};

template <typename T>
concept IParserEventDispatchC = requires(T t) {
  { t.onMember(std::string_view{}) } -> std::same_as<void>;
  { t.onObjectStart() } -> std::same_as<void>;
  { t.onObjectFinish() } -> std::same_as<void>;
  { t.onArrayStart() } -> std::same_as<void>;
  { t.onArrayFinish() } -> std::same_as<void>;
  { t.onStringValue(std::string_view{}) } -> std::same_as<void>;
  { t.onBooleanValue(bool{}) } -> std::same_as<void>;
  { t.onNumericValue(std::string_view{}) } -> std::same_as<void>;
};

template <MemberIdTraitsC MemberIdTraits> struct IMOPEDHandler {
  using MOPEDHandlerStack = std::stack<IMOPEDHandler *>;
  using MemberIdT = typename MemberIdTraits::MemberIdType;

  virtual ~IMOPEDHandler() = default;
  // Used by the JSONValidator
  virtual void onMember(MOPEDHandlerStack &handlerStack, MemberIdT memberId) {
    throw std::runtime_error("Unhandled member event");
  }
  virtual void onObjectStart(MOPEDHandlerStack &handlerStack) {
    throw std::runtime_error("Unhandled object start event");
  }
  virtual void onObjectFinish(MOPEDHandlerStack &handlerStack) {
    throw std::runtime_error("Unhandled object finish event");
  }
  virtual void onArrayStart(MOPEDHandlerStack &handlerStack) {
    throw std::runtime_error("Unhandled array start event");
  }
  virtual void onArrayFinish(MOPEDHandlerStack &handlerStack) {
    throw std::runtime_error("Unhandled array finish event");
  }
  virtual void onStringValue(std::string_view value) {
    throw std::runtime_error("Unhandled string value event");
  }
  virtual void onNumericValue(std::string_view value) {
    throw std::runtime_error("Unhandled numeric value event");
  }
  virtual void onBooleanValue(bool value) {
    throw std::runtime_error("Unhandled boolean value event");
  }
};

template <typename T, typename MemberIdTraits>
concept IMOPEDHandlerC = std::derived_from<T, IMOPEDHandler<MemberIdTraits>>;

template <typename T>
concept is_scaled_int = requires(T t) {
  { T::Scale };
  { t.getRawIntegerValue() } -> std::same_as<typename T::IntegralT>;
};

template <typename T>
concept is_enum = std::is_enum_v<T>;

template <typename T>
concept is_mapped_enum = requires(T t) {
  { typename T::EnumType{} };
  { T{std::string_view{}} };
  { t = std::string_view{} };
  { t.getEnumValue() } -> is_enum;
};

template <typename T>
concept IsMOPEDValueC =
    std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view> ||
    std::is_same_v<T, TimePoint> || std::is_integral_v<T> ||
    std::is_floating_point_v<T> || is_scaled_int<T> || is_mapped_enum<T>;

template <typename T, typename MemberIdTraits>
concept HasEmbeddedHandler =
    IMOPEDHandlerC<decltype(T::template getMOPEDHandler<MemberIdTraits>()),
                   MemberIdTraits>;

template <typename T, typename MemberIdTraits> void getMOPEDHandler() {}

template <typename T, moped::MemberIdTraitsC MemberIdTraits>
static auto getMOPEDHandlerForParser() {
  if constexpr (HasEmbeddedHandler<T, MemberIdTraits>) {
    return T::template getMOPEDHandler<MemberIdTraits>();
  } else {
    return getMOPEDHandler<T, MemberIdTraits>();
  }
}

template <typename T>
concept NotVoidResult = !std::is_void_v<T>;

template <typename T, typename MemberIdTraits>
concept IsMOPEDCompositeC = requires() {
  { getMOPEDHandlerForParser<T, MemberIdTraits>() } -> NotVoidResult;
};

template <typename T, typename MemberIdTraits>
concept IsMOPEDContentC =
    IMOPEDHandlerC<T, MemberIdTraits> || IsMOPEDValueC<T> ||
    IsMOPEDCompositeC<T, MemberIdTraits>;

template <typename T>
concept IsMOPEDPushCollectionC = requires(T t) {
  { t.emplace_back(std::declval<typename T::value_type>()) };
};

template <typename T>
concept IsMOPEDMapCollectionC = requires(T t) {
  {
    t.operator[](std::declval<typename T::key_type>())
  } -> std::same_as<typename T::mapped_type &>;
};

template <typename T>
concept IsMOPEDContentCollectionC =
    IsMOPEDPushCollectionC<T> || std::is_array_v<T>;

template <typename T>
concept is_array = std::is_array_v<T>;

template <typename T>
concept is_optional = requires(T t) {
  { t.has_value() } -> std::same_as<bool>;
  { t.value() } -> std::same_as<typename T::value_type &>;
};

template <typename T, typename MemberIdTraits>
concept IsMOPEDTypeC = IsMOPEDContentC<T, MemberIdTraits> ||
                       IsMOPEDContentCollectionC<T> || is_optional<T>;

template <typename T>
concept IsSettable = IsMOPEDValueC<T> ||
                     (is_optional<T> && IsMOPEDValueC<typename T::value_type>);

template <typename T, typename MemberIdTraits>
concept IsCompositeCollectionC =
    IsMOPEDPushCollectionC<T> &&
    IsMOPEDCompositeC<typename T::value_type, MemberIdTraits>;

} // namespace moped
