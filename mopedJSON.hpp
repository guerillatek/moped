#pragma once
#include "JSONStreamParser.hpp"
#include "JSONViewParser.hpp"
#include "moped.hpp"

namespace moped {

template <typename CompositeT, typename TFT, typename... Args>
  requires IsMOPEDCompositeC<CompositeT, StringMemberIdTraits<TFT>>
std::expected<CompositeT, std::string>
parseCompositeFromJSONStream(TFT, std::istream &jsonStream, Args &&...args) {
  using DispatcherT =
      CompositeParserEventDispatcher<CompositeT, StringMemberIdTraits<TFT>>;
  JSONStreamParser<DispatcherT> parser{std::forward<Args>(args)...};
  if (auto result = parser.parseInputStream(jsonStream); !result) {
    return std::unexpected(result.error());
  }
  return parser.getDispatcher().moveComposite();
}

template <typename CompositeT, typename TFT, typename... Args>
  requires IsMOPEDCompositeC<CompositeT, StringMemberIdTraits<TFT>>
std::expected<CompositeT, std::string>
parseCompositeFromJSONStream(TFT, std::string_view jsonText, Args &&...args) {
  std::stringstream ss{std::string{jsonText}};
  return parseCompositeFromJSONStream<CompositeT, TFT, Args...>(
      TFT{}, ss, std::forward<Args>(args)...);
}

template <typename CompositeT, typename TFT, typename... Args>
  requires IsMOPEDCompositeC<CompositeT, StringMemberIdTraits<TFT>>
std::expected<CompositeT, std::string>
parseCompositeFromJSONView(TFT, std::string_view jsonView, Args &&...args) {
  using DispatcherT =
      CompositeParserEventDispatcher<CompositeT, StringMemberIdTraits<TFT>>;
  DispatcherT dispatcher{std::forward<Args>(args)...};
  JSONViewParser<DispatcherT> parser{std::forward<Args>(args)...};
  if (auto result = parser.parse(jsonView); !result) {
    return std::unexpected(result.error());
  }
  return parser.getDispatcher().moveComposite();
}

} // namespace moped