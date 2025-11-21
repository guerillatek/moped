#pragma once
#include "JSONStreamParser.hpp"
#include "JSONViewParser.hpp"
#include "moped.hpp"

namespace moped {

template <typename CompositeT, typename TFT, typename... Args>
  requires IsMOPEDCompositeC<CompositeT, StringDecodingTraits<TFT>>
std::expected<CompositeT, std::string>
parseCompositeFromJSONStream(TFT, std::istream &jsonStream, Args &&...args) {
  using DispatcherT =
      CompositeParserEventDispatcher<CompositeT, StringDecodingTraits<TFT>>;
  DispatcherT dispatcher{std::forward<Args>(args)...};
  JSONStreamParser<DispatcherT> parser{dispatcher};
  if (auto result = parser.parse(jsonStream); !result) {
    return std::unexpected(result.error());
  }
  return dispatcher.moveComposite();
}

template <typename CompositeT, typename TFT, typename... Args>
  requires IsMOPEDCompositeC<CompositeT, StringDecodingTraits<TFT>>
std::expected<CompositeT, std::string>
parseCompositeFromJSONStream(TFT, std::string_view jsonText, Args &&...args) {
  std::stringstream ss{std::string{jsonText}};
  return parseCompositeFromJSONStream<CompositeT, TFT, Args...>(
      TFT{}, ss, std::forward<Args>(args)...);
}

template <typename CompositeT, typename TFT, typename... Args>
  requires IsMOPEDCompositeC<CompositeT, StringDecodingTraits<TFT>>
std::expected<CompositeT, std::string>
parseCompositeFromJSONView(TFT, std::string_view jsonView, Args &&...args) {
  using DispatcherT =
      CompositeParserEventDispatcher<CompositeT, StringDecodingTraits<TFT>>;
  DispatcherT dispatcher{std::forward<Args>(args)...};
  JSONViewParser<DispatcherT> parser{dispatcher};
  if (auto result = parser.parse(jsonView); !result) {
    return std::unexpected(result.error());
  }
  return parser.getDispatcher().moveComposite();
}

} // namespace moped