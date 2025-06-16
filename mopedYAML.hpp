#pragma once
#include "moped/YAMLParser.hpp"
#include "moped/moped.hpp"

namespace moped {

template <typename CompositeT, typename... Args>
  requires IsMOPEDCompositeC<CompositeT, StringMemberIdTraits>
std::expected<CompositeT, std::string>
parseCompositeFromYAMLView(std::string_view yamlView, Args &&...args) {
  using DispatcherT =
      CompositeParserEventDispatcher<CompositeT, StringMemberIdTraits>;
  DispatcherT dispatcher{std::forward<Args>(args)...};
  YAMLParser<DispatcherT> parser{};
  if (auto result = parser.parseInputView(yamlView); !result) {
    return std::unexpected(result.error());
  }
  return parser.getDispatcher().getComposite();
}

} // namespace moped