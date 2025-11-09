#pragma once
#include "JSONStreamParser.hpp"
#include "JSONViewParser.hpp"
#include "moped.hpp"

namespace moped {

template <typename CompositeT, typename... Args>
  requires IsMOPEDCompositeC<CompositeT, StringMemberIdTraits>
std::expected<CompositeT, std::string>
parseCompositeFromJSONStream(std::istream &jsonStream, Args &&...args) {
  using DispatcherT =
      CompositeParserEventDispatcher<CompositeT, StringMemberIdTraits>;
  JSONStreamParser<DispatcherT> parser{std::forward<Args>(args)...};
  if (auto result = parser.parseInputStream(jsonStream); !result) {
    return std::unexpected(result.error());
  }
  return parser.getDispatcher().getComposite();
}

template <typename CompositeT, typename... Args>
  requires IsMOPEDCompositeC<CompositeT, StringMemberIdTraits>
std::expected<CompositeT, std::string>
parseCompositeFromJSONStream(std::string_view jsonText, Args &&...args) {
  std::stringstream ss{std::string{jsonText}};
  return parseCompositeFromJSONStream<CompositeT, Args...>(
      ss, std::forward<Args>(args)...);
}

template <typename CompositeT, typename... Args>
  requires IsMOPEDCompositeC<CompositeT, StringMemberIdTraits>
std::expected<CompositeT, std::string>
parseCompositeFromJSONView(std::string_view jsonView, Args &&...args) {
  using DispatcherT =
      CompositeParserEventDispatcher<CompositeT, StringMemberIdTraits>;
  DispatcherT dispatcher{std::forward<Args>(args)...};
  JSONViewParser<DispatcherT> parser{std::forward<Args>(args)...};
  if (auto result = parser.parse(jsonView); !result) {
    return std::unexpected(result.error());
  }
  return parser.getDispatcher().getComposite();
}

} // namespace moped