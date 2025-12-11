#pragma once

#include "moped/AutoTypeSelectingParserDispatcher.hpp"
#include "moped/AutoTypeSelectingParserHandler.hpp"
#include "moped/CollectionFunctionDispatcher.hpp"
#include "moped/CompositeParseEventDispatcher.hpp"
#include "moped/MappedEnum.hpp"
#include "moped/MappedObjectParseEncoderDispatcher.hpp"
#include "moped/PivotMemberTypeSelector.hpp"
#include "moped/ScaledInteger.hpp"
#include "moped/TimeFormatters.hpp"
#include "moped/concepts.hpp"

namespace moped {

template <typename TimePointFormatter = DurationSinceEpochFormatter<>,
          typename PivotValueType = std::string_view>
struct StringDecodingTraits {
  using MemberIdType = std::string_view;
  using PivotValueT = PivotValueType;
  static MemberIdType getMemberId(std::string_view name) { return name; }
  static std::string getDisplayName(MemberIdType memberId) {
    return std::string{memberId};
  }
  static constexpr std::string_view EmbeddingMemberId = "";
  using TimePointFormaterT = TimePointFormatter;
};

static_assert(DecodingTraitsC<StringDecodingTraits<>>,
              "StringDecodingTraits should satisfy DecodingTraitsC concept");

} // namespace moped