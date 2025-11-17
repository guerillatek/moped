#pragma once
#include "moped/YAMLParser.hpp"
#include "moped/moped.hpp"

namespace moped {

template <typename CompositeT, typename TimeFormatterT, typename... Args>
  requires IsMOPEDCompositeC<CompositeT, StringDecodingTraits<TimeFormatterT>>
std::expected<CompositeT, std::string>
parseCompositeFromYAMLView(TimeFormatterT, std::string_view yamlView,
                           Args &&...args) {
  using DispatcherT =
      CompositeParserEventDispatcher<CompositeT,
                                     StringDecodingTraits<TimeFormatterT>>;
  DispatcherT dispatcher{std::forward<Args>(args)...};
  YAMLParser<DispatcherT> parser{};
  if (auto result = parser.parseInputView(yamlView); !result) {
    return std::unexpected(result.error());
  }
  return parser.getDispatcher().moveComposite();
}

} // namespace moped