#pragma once
#include "BoostInfoTreeParser.hpp"
#include "moped.hpp"

namespace moped {

template <typename CompositeT, typename... Args>
  requires IsMOPEDCompositeC<CompositeT, StringMemberIdTraits>
std::expected<CompositeT, std::string>
parseCompositeFromBoostInfoFile(std::string filePath, Args &&...args) {
  using DispatcherT =
      CompositeParserEventDispatcher<CompositeT, StringMemberIdTraits>;
  DispatcherT dispatcher{std::forward<Args>(args)...};
  BoostInfoTreeParser<DispatcherT> parser{};
  if (auto result = parser.parse(jsonView); !result) {
    return std::unexpected(result.error());
  }
  return parser.getDispatcher().moveComposite();
}

} // namespace moped