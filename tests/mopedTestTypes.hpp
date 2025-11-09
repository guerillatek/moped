#include "moped/CollectionFunctionDispatcher.hpp"
#include "moped/MappedObjectParseEncoderDispatcher.hpp"
#include "moped/ScaledInteger.hpp"
#include "moped/mopedJSON.hpp"

#include <optional>

namespace moped::tests {
using S9Int = moped::ScaledInteger<std::int64_t, 9>;

namespace view {
struct RateLimit {
  std::string_view rateLimitType;
  std::string_view interval;
  int intervalNum;
  int limit;

  // moped serialization
  template <moped::MemberIdTraitsC MemberIdTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<MemberIdTraits, RateLimit>(
        "rateLimitType", &RateLimit::rateLimitType, "interval",
        &RateLimit::interval, "intervalNum", &RateLimit::intervalNum, "limit",
        &RateLimit::limit);
  }
};

// Struct for filters
struct Filter {
  std::string_view filterType;
  std::optional<S9Int> bidMultiplierUp;
  std::optional<S9Int> bidMultiplierDown;
  std::optional<S9Int> askMultiplierUp;
  std::optional<S9Int> askMultiplierDown;
  std::optional<S9Int> minPrice;
  std::optional<S9Int> maxPrice;
  std::optional<S9Int> tickSize;
  std::optional<S9Int> minQty;
  std::optional<S9Int> maxQty;
  std::optional<S9Int> stepSize;
  std::optional<S9Int> limit;
  std::optional<S9Int> minNotional;
  std::optional<S9Int> multiplierUp;
  std::optional<S9Int> multiplierDown;
  std::optional<bool> applyToMarket;
  std::optional<bool> applyMinToMarket;
  std::optional<S9Int> maxNotional;
  std::optional<bool> applyMaxToMarket;
  std::optional<S9Int> avgPriceMins;
  std::optional<S9Int> minTrailingAboveDelta;
  std::optional<S9Int> maxTrailingAboveDelta;
  std::optional<S9Int> minTrailingBelowDelta;
  std::optional<S9Int> maxTrailingBelowDelta;
  std::optional<int> maxNumOrders;
  std::optional<int> maxNumAlgoOrders;

  template <moped::MemberIdTraitsC MemberIdTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<MemberIdTraits, Filter>(
        "filterType", &Filter::filterType, "minPrice", &Filter::minPrice,
        "maxPrice", &Filter::maxPrice, "tickSize", &Filter::tickSize, "minQty",
        &Filter::minQty, "maxQty", &Filter::maxQty, "stepSize",
        &Filter::stepSize, "limit", &Filter::limit, "minNotional",
        &Filter::minNotional, "multiplierUp", &Filter::multiplierUp,
        "multiplierDown", &Filter::multiplierDown, "applyToMarket",
        &Filter::applyToMarket, "applyMinToMarket", &Filter::applyMinToMarket,
        "maxNotional", &Filter::maxNotional, "applyMaxToMarket",
        &Filter::applyMaxToMarket, "avgPriceMins", &Filter::avgPriceMins,
        "minTrailingAboveDelta", &Filter::minTrailingAboveDelta,
        "maxTrailingAboveDelta", &Filter::maxTrailingAboveDelta,
        "minTrailingBelowDelta", &Filter::minTrailingBelowDelta,
        "maxTrailingBelowDelta", &Filter::maxTrailingBelowDelta, "maxNumOrders",
        &Filter::maxNumOrders, "maxNumAlgoOrders", &Filter::maxNumAlgoOrders,
        "bidMultiplierUp", &Filter::bidMultiplierUp, "bidMultiplierDown",
        &Filter::bidMultiplierDown, "askMultiplierUp", &Filter::askMultiplierUp,
        "askMultiplierDown", &Filter::askMultiplierDown);
  }
};

// Struct for symbols
struct Symbol {
  std::string_view symbol;
  std::string_view status;
  std::string_view baseAsset;
  int baseAssetPrecision;
  std::string_view quoteAsset;
  int quotePrecision;
  int quoteAssetPrecision;
  int baseCommissionPrecision;
  int quoteCommissionPrecision;
  std::vector<std::string_view> orderTypes;
  bool icebergAllowed;
  bool ocoAllowed;
  bool otoAllowed;
  bool quoteOrderQtyMarketAllowed;
  bool allowTrailingStop;
  bool cancelReplaceAllowed;
  bool amendAllowed;
  bool isSpotTradingAllowed;
  bool isMarginTradingAllowed;
  std::vector<Filter> filters;
  std::vector<std::string_view> permissions;
  std::string_view defaultSelfTradePreventionMode;
  std::vector<std::string_view> allowedSelfTradePreventionModes;

  template <moped::MemberIdTraitsC MemberIdTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<MemberIdTraits, Symbol>(
        "symbol", &Symbol::symbol, "status", &Symbol::status, "baseAsset",
        &Symbol::baseAsset, "baseAssetPrecision", &Symbol::baseAssetPrecision,
        "quoteAsset", &Symbol::quoteAsset, "quotePrecision",
        &Symbol::quotePrecision, "quoteAssetPrecision",
        &Symbol::quoteAssetPrecision, "baseCommissionPrecision",
        &Symbol::baseCommissionPrecision, "quoteCommissionPrecision",
        &Symbol::quoteCommissionPrecision, "orderTypes", &Symbol::orderTypes,
        "icebergAllowed", &Symbol::icebergAllowed, "ocoAllowed",
        &Symbol::ocoAllowed, "otoAllowed", &Symbol::otoAllowed,
        "quoteOrderQtyMarketAllowed", &Symbol::quoteOrderQtyMarketAllowed,
        "allowTrailingStop", &Symbol::allowTrailingStop, "cancelReplaceAllowed",
        &Symbol::cancelReplaceAllowed, "amendAllowed", &Symbol::amendAllowed,
        "isSpotTradingAllowed", &Symbol::isSpotTradingAllowed,
        "isMarginTradingAllowed", &Symbol::isMarginTradingAllowed, "filters",
        &Symbol::filters, "permissions", &Symbol::permissions,
        "defaultSelfTradePreventionMode",
        &Symbol::defaultSelfTradePreventionMode,
        "allowedSelfTradePreventionModes",
        &Symbol::allowedSelfTradePreventionModes);
  }
};

// Root struct for the API response
struct ExchangeInfo {
  std::string timezone;
  moped::TimePoint serverTime;
  std::vector<RateLimit> rateLimits;
  std::vector<std::string> exchangeFilters;
  std::vector<Symbol> symbols;

  template <moped::MemberIdTraitsC MemberIdTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<MemberIdTraits, ExchangeInfo>(
        "timezone", &ExchangeInfo::timezone, "serverTime",
        &ExchangeInfo::serverTime, "rateLimits", &ExchangeInfo::rateLimits,
        "exchangeFilters", &ExchangeInfo::exchangeFilters, "symbols",
        &ExchangeInfo::symbols);
  }
};

} // namespace view

namespace stream {

struct RateLimit {
  std::string rateLimitType;
  std::string interval;
  int intervalNum;
  int limit;

  // moped serialization
  template <moped::MemberIdTraitsC MemberIdTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<MemberIdTraits, RateLimit>(
        "rateLimitType", &RateLimit::rateLimitType, "interval",
        &RateLimit::interval, "intervalNum", &RateLimit::intervalNum, "limit",
        &RateLimit::limit);
  }
};

// Struct for filters
struct Filter {
  std::string filterType;
  std::optional<S9Int> bidMultiplierUp;
  std::optional<S9Int> bidMultiplierDown;
  std::optional<S9Int> askMultiplierUp;
  std::optional<S9Int> askMultiplierDown;
  std::optional<S9Int> minPrice;
  std::optional<S9Int> maxPrice;
  std::optional<S9Int> tickSize;
  std::optional<S9Int> minQty;
  std::optional<S9Int> maxQty;
  std::optional<S9Int> stepSize;
  std::optional<S9Int> limit;
  std::optional<S9Int> minNotional;
  std::optional<S9Int> multiplierUp;
  std::optional<S9Int> multiplierDown;
  std::optional<bool> applyToMarket;
  std::optional<bool> applyMinToMarket;
  std::optional<S9Int> maxNotional;
  std::optional<bool> applyMaxToMarket;
  std::optional<S9Int> avgPriceMins;
  std::optional<S9Int> minTrailingAboveDelta;
  std::optional<S9Int> maxTrailingAboveDelta;
  std::optional<S9Int> minTrailingBelowDelta;
  std::optional<S9Int> maxTrailingBelowDelta;
  std::optional<int> maxNumOrders;
  std::optional<int> maxNumAlgoOrders;

  template <moped::MemberIdTraitsC MemberIdTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<MemberIdTraits, Filter>(
        "filterType", &Filter::filterType, "minPrice", &Filter::minPrice,
        "maxPrice", &Filter::maxPrice, "tickSize", &Filter::tickSize, "minQty",
        &Filter::minQty, "maxQty", &Filter::maxQty, "stepSize",
        &Filter::stepSize, "limit", &Filter::limit, "minNotional",
        &Filter::minNotional, "multiplierUp", &Filter::multiplierUp,
        "multiplierDown", &Filter::multiplierDown, "applyToMarket",
        &Filter::applyToMarket, "applyMinToMarket", &Filter::applyMinToMarket,
        "maxNotional", &Filter::maxNotional, "applyMaxToMarket",
        &Filter::applyMaxToMarket, "avgPriceMins", &Filter::avgPriceMins,
        "minTrailingAboveDelta", &Filter::minTrailingAboveDelta,
        "maxTrailingAboveDelta", &Filter::maxTrailingAboveDelta,
        "minTrailingBelowDelta", &Filter::minTrailingBelowDelta,
        "maxTrailingBelowDelta", &Filter::maxTrailingBelowDelta, "maxNumOrders",
        &Filter::maxNumOrders, "maxNumAlgoOrders", &Filter::maxNumAlgoOrders,
        "bidMultiplierUp", &Filter::bidMultiplierUp, "bidMultiplierDown",
        &Filter::bidMultiplierDown, "askMultiplierUp", &Filter::askMultiplierUp,
        "askMultiplierDown", &Filter::askMultiplierDown);
  }
};

// Struct for symbols
struct Symbol {
  std::string symbol;
  std::string status;
  std::string baseAsset;
  int baseAssetPrecision;
  std::string quoteAsset;
  int quotePrecision;
  int quoteAssetPrecision;
  int baseCommissionPrecision;
  int quoteCommissionPrecision;
  std::vector<std::string> orderTypes;
  bool icebergAllowed;
  bool ocoAllowed;
  bool otoAllowed;
  bool quoteOrderQtyMarketAllowed;
  bool allowTrailingStop;
  bool cancelReplaceAllowed;
  bool amendAllowed;
  bool isSpotTradingAllowed;
  bool isMarginTradingAllowed;
  std::vector<Filter> filters;
  std::vector<std::string> permissions;
  std::string defaultSelfTradePreventionMode;
  std::vector<std::string> allowedSelfTradePreventionModes;

  template <moped::MemberIdTraitsC MemberIdTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<MemberIdTraits, Symbol>(
        "symbol", &Symbol::symbol, "status", &Symbol::status, "baseAsset",
        &Symbol::baseAsset, "baseAssetPrecision", &Symbol::baseAssetPrecision,
        "quoteAsset", &Symbol::quoteAsset, "quotePrecision",
        &Symbol::quotePrecision, "quoteAssetPrecision",
        &Symbol::quoteAssetPrecision, "baseCommissionPrecision",
        &Symbol::baseCommissionPrecision, "quoteCommissionPrecision",
        &Symbol::quoteCommissionPrecision, "orderTypes", &Symbol::orderTypes,
        "icebergAllowed", &Symbol::icebergAllowed, "ocoAllowed",
        &Symbol::ocoAllowed, "otoAllowed", &Symbol::otoAllowed,
        "quoteOrderQtyMarketAllowed", &Symbol::quoteOrderQtyMarketAllowed,
        "allowTrailingStop", &Symbol::allowTrailingStop, "cancelReplaceAllowed",
        &Symbol::cancelReplaceAllowed, "amendAllowed", &Symbol::amendAllowed,
        "isSpotTradingAllowed", &Symbol::isSpotTradingAllowed,
        "isMarginTradingAllowed", &Symbol::isMarginTradingAllowed, "filters",
        &Symbol::filters, "permissions", &Symbol::permissions,
        "defaultSelfTradePreventionMode",
        &Symbol::defaultSelfTradePreventionMode,
        "allowedSelfTradePreventionModes",
        &Symbol::allowedSelfTradePreventionModes);
  }
};

// Root struct for the API response
struct ExchangeInfo {
  std::string timezone;
  moped::TimePoint serverTime;
  std::vector<RateLimit> rateLimits;
  std::vector<std::string> exchangeFilters;
  std::vector<Symbol> symbols;

  template <moped::MemberIdTraitsC MemberIdTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<MemberIdTraits, ExchangeInfo>(
        "timezone", &ExchangeInfo::timezone, "serverTime",
        &ExchangeInfo::serverTime, "rateLimits", &ExchangeInfo::rateLimits,
        "exchangeFilters", &ExchangeInfo::exchangeFilters, "symbols",
        &ExchangeInfo::symbols);
  }
};

struct ExchangeInfoWithSymbolDispatch {
  std::string timezone;
  moped::TimePoint serverTime;
  std::vector<RateLimit> rateLimits;
  std::vector<std::string> exchangeFilters;
  moped::CollectionFunctionDispatcher<Symbol> symbols;

  ExchangeInfoWithSymbolDispatch(auto &&handler)
      : symbols(std::move(handler)) {}

  ExchangeInfoWithSymbolDispatch()
      : symbols([](const Symbol &) {}) {} // Default constructor

  template <moped::MemberIdTraitsC MemberIdTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<MemberIdTraits, ExchangeInfoWithSymbolDispatch>(
        "timezone", &ExchangeInfoWithSymbolDispatch::timezone, "serverTime",
        &ExchangeInfoWithSymbolDispatch::serverTime, "rateLimits",
        &ExchangeInfoWithSymbolDispatch::rateLimits, "exchangeFilters",
        &ExchangeInfoWithSymbolDispatch::exchangeFilters, "symbols",
        &ExchangeInfoWithSymbolDispatch::symbols);
  }
};
} // namespace stream

} // namespace moped::tests
