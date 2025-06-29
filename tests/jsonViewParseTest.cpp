#include <catch2/catch.hpp>

#include "moped/MappedObjectParseEncoderDispatcher.hpp"
#include "moped/ScaledInteger.hpp"
#include "moped/mopedJSON.hpp"

#include <optional>

std::string_view instrumentData_view =
    R"({"timezone":"UTC","serverTime":1748461268460,"rateLimits":[{"rateLimitType":"REQUEST_WEIGHT","interval":"MINUTE","intervalNum":1,"limit":1200},{"rateLimitType":"ORDERS","interval":"SECOND","intervalNum":10,"limit":100},{"rateLimitType":"ORDERS","interval":"DAY","intervalNum":1,"limit":200000},{"rateLimitType":"RAW_REQUESTS","interval":"MINUTE","intervalNum":5,"limit":6100}],"exchangeFilters":[],"symbols":[{"symbol":"BTCUSD4","status":"BREAK","baseAsset":"BTC","baseAssetPrecision":8,"quoteAsset":"USD4","quotePrecision":4,"quoteAssetPrecision":4,"baseCommissionPrecision":8,"quoteCommissionPrecision":2,"orderTypes":["LIMIT","LIMIT_MAKER","MARKET","STOP_LOSS_LIMIT","TAKE_PROFIT_LIMIT"],"icebergAllowed":true,"ocoAllowed":true,"quoteOrderQtyMarketAllowed":true,"allowTrailingStop":true,"cancelReplaceAllowed":true,"isSpotTradingAllowed":true,"isMarginTradingAllowed":false,"filters":[{"filterType":"PRICE_FILTER","minPrice":"0.0100","maxPrice":"100000.0000","tickSize":"0.0100"},{"filterType":"PERCENT_PRICE","multiplierUp":"5","multiplierDown":"0.2","avgPriceMins":5},{"filterType":"LOT_SIZE","minQty":"0.00000100","maxQty":"9000.00000000","stepSize":"0.00000100"},{"filterType":"MIN_NOTIONAL","minNotional":"10.0000","applyToMarket":true,"avgPriceMins":5},{"filterType":"ICEBERG_PARTS","limit":10},{"filterType":"MARKET_LOT_SIZE","minQty":"0.00000000","maxQty":"60.25597724","stepSize":"0.00000000"},{"filterType":"TRAILING_DELTA","minTrailingAboveDelta":10,"maxTrailingAboveDelta":2000,"minTrailingBelowDelta":10,"maxTrailingBelowDelta":2000},{"filterType":"MAX_NUM_ORDERS","maxNumOrders":200},{"filterType":"MAX_NUM_ALGO_ORDERS","maxNumAlgoOrders":5}],"permissions":["SPOT"],"defaultSelfTradePreventionMode":"EXPIRE_MAKER","allowedSelfTradePreventionModes":["EXPIRE_TAKER","EXPIRE_MAKER","EXPIRE_BOTH"]},{"symbol":"ETHUSD4","status":"BREAK","baseAsset":"ETH","baseAssetPrecision":8,"quoteAsset":"USD4","quotePrecision":4,"quoteAssetPrecision":4,"baseCommissionPrecision":8,"quoteCommissionPrecision":4,"orderTypes":["LIMIT","LIMIT_MAKER","MARKET","STOP_LOSS_LIMIT","TAKE_PROFIT_LIMIT"],"icebergAllowed":true,"ocoAllowed":true,"quoteOrderQtyMarketAllowed":true,"allowTrailingStop":true,"cancelReplaceAllowed":true,"isSpotTradingAllowed":true,"isMarginTradingAllowed":false,"filters":[{"filterType":"PRICE_FILTER","minPrice":"0.0100","maxPrice":"100000.0000","tickSize":"0.0100"},{"filterType":"PERCENT_PRICE","multiplierUp":"5","multiplierDown":"0.2","avgPriceMins":5},{"filterType":"LOT_SIZE","minQty":"0.00001000","maxQty":"9000.00000000","stepSize":"0.00001000"},{"filterType":"MIN_NOTIONAL","minNotional":"10.0000","applyToMarket":true,"avgPriceMins":5},{"filterType":"ICEBERG_PARTS","limit":10},{"filterType":"MARKET_LOT_SIZE","minQty":"0.00000000","maxQty":"514.38625342","stepSize":"0.00000000"},{"filterType":"TRAILING_DELTA","minTrailingAboveDelta":10,"maxTrailingAboveDelta":2000,"minTrailingBelowDelta":10,"maxTrailingBelowDelta":2000},{"filterType":"MAX_NUM_ORDERS","maxNumOrders":200},{"filterType":"MAX_NUM_ALGO_ORDERS","maxNumAlgoOrders":5}],"permissions":["SPOT"],"defaultSelfTradePreventionMode":"EXPIRE_MAKER","allowedSelfTradePreventionModes":["EXPIRE_TAKER","EXPIRE_MAKER","EXPIRE_BOTH"]},{"symbol":"XRPUSD4","status":"BREAK","baseAsset":"XRP","baseAssetPrecision":8,"quoteAsset":"USD4","quotePrecision":4,"quoteAssetPrecision":4,"baseCommissionPrecision":8,"quoteCommissionPrecision":4,"orderTypes":["LIMIT","LIMIT_MAKER","MARKET","STOP_LOSS_LIMIT","TAKE_PROFIT_LIMIT"],"icebergAllowed":true,"ocoAllowed":true,"quoteOrderQtyMarketAllowed":true,"allowTrailingStop":true,"cancelReplaceAllowed":true,"isSpotTradingAllowed":true,"isMarginTradingAllowed":false,"filters":[{"filterType":"PRICE_FILTER","minPrice":"0.0001","maxPrice":"1000.0000","tickSize":"0.0001"},{"filterType":"PERCENT_PRICE","multiplierUp":"5","multiplierDown":"0.2","avgPriceMins":5},{"filterType":"LOT_SIZE","minQty":"1.00000000","maxQty":"9222449.00000000","stepSize":"1.00000000"},{"filterType":"MIN_NOTIONAL","minNotional":"1.0000","applyToMarket":true,"avgPriceMins":5},{"filterType":"ICEBERG_PARTS","limit":10},{"filterType":"MARKET_LOT_SIZE","minQty":"0.00000000","maxQty":"2797914.31380753","stepSize":"0.00000000"},{"filterType":"TRAILING_DELTA","minTrailingAboveDelta":10,"maxTrailingAboveDelta":2000,"minTrailingBelowDelta":10,"maxTrailingBelowDelta":2000},{"filterType":"PERCENT_PRICE_BY_SIDE","bidMultiplierUp":"5","bidMultiplierDown":"0.2","askMultiplierUp":"5","askMultiplierDown":"0.2","avgPriceMins":5},{"filterType":"MAX_NUM_ORDERS","maxNumOrders":200},{"filterType":"MAX_NUM_ALGO_ORDERS","maxNumAlgoOrders":5}],"permissions":["SPOT"],"defaultSelfTradePreventionMode":"EXPIRE_MAKER","allowedSelfTradePreventionModes":["EXPIRE_TAKER","EXPIRE_MAKER","EXPIRE_BOTH"]}]})";

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

struct MopedTypeDefinitions {
  std::string name;
  std::string description;
};

} // namespace view

#define ASSERT_EQ(A, B) REQUIRE(A == B)

TEST_CASE("Instrument Data load Test with full payload parser and full "
          "std::string_view member support",
          "[JSON FULL PAYLOAD (VIEW) PARSER]") {

  auto result = moped::parseCompositeFromJSONView<view::ExchangeInfo>(
      instrumentData_view);
  REQUIRE(result.has_value());

  const view::ExchangeInfo &info = result.value();

  // Top-level checks
  ASSERT_EQ(info.timezone, "UTC");
  ASSERT_EQ(info.serverTime, moped::TimePoint{std::chrono::nanoseconds{
                                         1748461268460000000LL}});
  ASSERT_EQ(info.rateLimits.size(), 4);
  ASSERT_EQ(info.exchangeFilters.size(), 0);
  ASSERT_EQ(info.symbols.size(), 3);

  // Check first rate limit
  ASSERT_EQ(info.rateLimits[0].rateLimitType, "REQUEST_WEIGHT");
  ASSERT_EQ(info.rateLimits[0].interval, "MINUTE");
  ASSERT_EQ(info.rateLimits[0].intervalNum, 1);
  ASSERT_EQ(info.rateLimits[0].limit, 1200);

  // Check first symbol
  const view::Symbol &sym = info.symbols[0];
  ASSERT_EQ(sym.symbol, "BTCUSD4");
  ASSERT_EQ(sym.status, "BREAK");
  ASSERT_EQ(sym.baseAsset, "BTC");
  ASSERT_EQ(sym.baseAssetPrecision, 8);
  ASSERT_EQ(sym.quoteAsset, "USD4");
  ASSERT_EQ(sym.quotePrecision, 4);
  ASSERT_EQ(sym.quoteAssetPrecision, 4);
  ASSERT_EQ(sym.baseCommissionPrecision, 8);
  ASSERT_EQ(sym.quoteCommissionPrecision, 2);
  ASSERT_EQ(sym.orderTypes.size(), 5);
  REQUIRE(sym.icebergAllowed);
  REQUIRE(sym.ocoAllowed);
  REQUIRE(sym.quoteOrderQtyMarketAllowed);
  REQUIRE(sym.allowTrailingStop);
  REQUIRE(sym.cancelReplaceAllowed);
  REQUIRE(sym.isSpotTradingAllowed);
  REQUIRE(!sym.isMarginTradingAllowed);
  ASSERT_EQ(sym.filters.size(), 9);
  ASSERT_EQ(sym.permissions.size(), 1);
  ASSERT_EQ(sym.permissions[0], "SPOT");
  ASSERT_EQ(sym.defaultSelfTradePreventionMode, "EXPIRE_MAKER");
  ASSERT_EQ(sym.allowedSelfTradePreventionModes.size(), 3);

  // Check a filter in the first symbol
  const view::Filter &filter = sym.filters[0];
  ASSERT_EQ(filter.filterType, "PRICE_FILTER");
  REQUIRE(filter.minPrice.has_value());
  ASSERT_EQ(filter.minPrice.value(), S9Int{"0.0100"});
  REQUIRE(filter.maxPrice.has_value());
  ASSERT_EQ(filter.maxPrice.value(), S9Int{"100000.0000"});
  REQUIRE(filter.tickSize.has_value());
  ASSERT_EQ(filter.tickSize.value(), S9Int{"0.0100"});
}
