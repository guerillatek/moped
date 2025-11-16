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
  { t.onMember(std::string_view{}) } -> std::same_as<Expected>;
  { t.onObjectStart() } -> std::same_as<Expected>;
  { t.onObjectFinish() } -> std::same_as<Expected>;
  { t.onArrayStart() } -> std::same_as<Expected>;
  { t.onArrayFinish() } -> std::same_as<Expected>;
  { t.onStringValue(std::string_view{}) } -> std::same_as<Expected>;
  { t.onBooleanValue(bool{}) } -> std::same_as<Expected>;
  { t.onNumericValue(std::string_view{}) } -> std::same_as<Expected>;
};

template <MemberIdTraitsC MemberIdTraits> struct IMOPEDHandler {
  using MOPEDHandlerStack = std::stack<IMOPEDHandler *>;
  using MemberIdT = typename MemberIdTraits::MemberIdType;

  virtual ~IMOPEDHandler() = default;

  virtual Expected onMember(MOPEDHandlerStack &, MemberIdT) {
    return std::unexpected("Unhandled member event");
  }

  virtual Expected onObjectStart(MOPEDHandlerStack &) {
    return std::unexpected("Unhandled object start event");
  }

  virtual Expected onObjectFinish(MOPEDHandlerStack &) {
    return std::unexpected("Unhandled object finish event");
  }

  virtual Expected onArrayStart(MOPEDHandlerStack &) {
    return std::unexpected("Unhandled array start event");
  }

  virtual Expected onArrayFinish(MOPEDHandlerStack &) {
    return std::unexpected("Unhandled array finish event");
  }

  virtual Expected onStringValue(std::string_view) {
    return std::unexpected("Unhandled string value event");
  }

  virtual Expected onNumericValue(std::string_view) {
    return std::unexpected("Unhandled numeric value event");
  }

  virtual Expected onBooleanValue(bool) {
    return std::unexpected("Unhandled boolean value event");
  }

  virtual Expected onRawBinary(void *) {
    return std::unexpected("Unhandled binary value event");
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

template <typename T> struct is_variant_t : std::false_type {};

template <typename... Types>
struct is_variant_t<std::variant<Types...>> : std::true_type {};

template <typename T>
inline constexpr bool is_variant_v = is_variant_t<T>::value;

template <typename T>
concept is_variant = is_variant_v<std::remove_cvref_t<T>>;

template <typename T>
concept is_integral = std::is_integral_v<T>;

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
concept IsMOPEDCompositeDispatcherC = requires(T t) {
  { t.setCurrentValue(std::declval<const typename T::value_type &>()) };
  { t.dispatchLastCapture() };
  { t.resetCapture() } -> std::same_as<typename T::value_type &>;
};

template <typename T>
concept IsMOPEDMapCollectionC = requires(T t) {
  {
    t.operator[](std::declval<typename T::key_type>())
  } -> std::same_as<typename T::mapped_type &>;
};


template <typename T> struct is_std_array : std::false_type {};

template <typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

template <typename T>
inline constexpr bool is_std_array_v =
    is_std_array<std::remove_cvref_t<T>>::value;

template <typename T>
concept is_array = std::is_array_v<T> || is_std_array_v<T>;


template <typename T>
concept IsMOPEDContentCollectionC =
    IsMOPEDPushCollectionC<T> || is_array<T>;



template <typename T, typename MemberIdTraits>
concept IsOptionalC = requires(T t) {
  { t.has_value() } -> std::same_as<bool>;
  { t.value() } -> std::same_as<typename T::value_type &>;
} && IsMOPEDContentC<typename T::value_type, MemberIdTraits>;

template <typename T, typename MemberIdTraits>
concept IsMOPEDTypeC =
    IsMOPEDContentC<T, MemberIdTraits> || IsMOPEDContentCollectionC<T> ||
    IsOptionalC<T, MemberIdTraits> || IsMOPEDCompositeDispatcherC<T>;

template <typename T>
concept IsSettableVariantC = ISettableVariant(T{});

template <typename T, typename MemberIdTraits>
concept IsSettableC =
    IsMOPEDValueC<T> ||
    (IsOptionalC<T, MemberIdTraits> && IsMOPEDValueC<typename T::value_type>);

template <typename T, typename MemberIdTraits>
concept IsCompositeCollectionC =
    IsMOPEDPushCollectionC<T> &&
    IsMOPEDCompositeC<typename T::value_type, MemberIdTraits>;

} // namespace moped
