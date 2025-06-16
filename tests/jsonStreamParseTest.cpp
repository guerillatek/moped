#include "moped/MappedObjectParseEncoderDispatcher.hpp"
#include "moped/ScaledInteger.hpp"
#include "moped/mopedJSON.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include <optional>

std::string_view instrumentData =
    R"({"timezone":"UTC","serverTime":1748461268460,"rateLimits":[{"rateLimitType":"REQUEST_WEIGHT","interval":"MINUTE","intervalNum":1,"limit":1200},{"rateLimitType":"ORDERS","interval":"SECOND","intervalNum":10,"limit":100},{"rateLimitType":"ORDERS","interval":"DAY","intervalNum":1,"limit":200000},{"rateLimitType":"RAW_REQUESTS","interval":"MINUTE","intervalNum":5,"limit":6100}],"exchangeFilters":[],"symbols":[{"symbol":"BTCUSD4","status":"BREAK","baseAsset":"BTC","baseAssetPrecision":8,"quoteAsset":"USD4","quotePrecision":4,"quoteAssetPrecision":4,"baseCommissionPrecision":8,"quoteCommissionPrecision":2,"orderTypes":["LIMIT","LIMIT_MAKER","MARKET","STOP_LOSS_LIMIT","TAKE_PROFIT_LIMIT"],"icebergAllowed":true,"ocoAllowed":true,"quoteOrderQtyMarketAllowed":true,"allowTrailingStop":true,"cancelReplaceAllowed":true,"isSpotTradingAllowed":true,"isMarginTradingAllowed":false,"filters":[{"filterType":"PRICE_FILTER","minPrice":"0.0100","maxPrice":"100000.0000","tickSize":"0.0100"},{"filterType":"PERCENT_PRICE","multiplierUp":"5","multiplierDown":"0.2","avgPriceMins":5},{"filterType":"LOT_SIZE","minQty":"0.00000100","maxQty":"9000.00000000","stepSize":"0.00000100"},{"filterType":"MIN_NOTIONAL","minNotional":"10.0000","applyToMarket":true,"avgPriceMins":5},{"filterType":"ICEBERG_PARTS","limit":10},{"filterType":"MARKET_LOT_SIZE","minQty":"0.00000000","maxQty":"60.25597724","stepSize":"0.00000000"},{"filterType":"TRAILING_DELTA","minTrailingAboveDelta":10,"maxTrailingAboveDelta":2000,"minTrailingBelowDelta":10,"maxTrailingBelowDelta":2000},{"filterType":"MAX_NUM_ORDERS","maxNumOrders":200},{"filterType":"MAX_NUM_ALGO_ORDERS","maxNumAlgoOrders":5}],"permissions":["SPOT"],"defaultSelfTradePreventionMode":"EXPIRE_MAKER","allowedSelfTradePreventionModes":["EXPIRE_TAKER","EXPIRE_MAKER","EXPIRE_BOTH"]},{"symbol":"ETHUSD4","status":"BREAK","baseAsset":"ETH","baseAssetPrecision":8,"quoteAsset":"USD4","quotePrecision":4,"quoteAssetPrecision":4,"baseCommissionPrecision":8,"quoteCommissionPrecision":4,"orderTypes":["LIMIT","LIMIT_MAKER","MARKET","STOP_LOSS_LIMIT","TAKE_PROFIT_LIMIT"],"icebergAllowed":true,"ocoAllowed":true,"quoteOrderQtyMarketAllowed":true,"allowTrailingStop":true,"cancelReplaceAllowed":true,"isSpotTradingAllowed":true,"isMarginTradingAllowed":false,"filters":[{"filterType":"PRICE_FILTER","minPrice":"0.0100","maxPrice":"100000.0000","tickSize":"0.0100"},{"filterType":"PERCENT_PRICE","multiplierUp":"5","multiplierDown":"0.2","avgPriceMins":5},{"filterType":"LOT_SIZE","minQty":"0.00001000","maxQty":"9000.00000000","stepSize":"0.00001000"},{"filterType":"MIN_NOTIONAL","minNotional":"10.0000","applyToMarket":true,"avgPriceMins":5},{"filterType":"ICEBERG_PARTS","limit":10},{"filterType":"MARKET_LOT_SIZE","minQty":"0.00000000","maxQty":"514.38625342","stepSize":"0.00000000"},{"filterType":"TRAILING_DELTA","minTrailingAboveDelta":10,"maxTrailingAboveDelta":2000,"minTrailingBelowDelta":10,"maxTrailingBelowDelta":2000},{"filterType":"MAX_NUM_ORDERS","maxNumOrders":200},{"filterType":"MAX_NUM_ALGO_ORDERS","maxNumAlgoOrders":5}],"permissions":["SPOT"],"defaultSelfTradePreventionMode":"EXPIRE_MAKER","allowedSelfTradePreventionModes":["EXPIRE_TAKER","EXPIRE_MAKER","EXPIRE_BOTH"]},{"symbol":"XRPUSD4","status":"BREAK","baseAsset":"XRP","baseAssetPrecision":8,"quoteAsset":"USD4","quotePrecision":4,"quoteAssetPrecision":4,"baseCommissionPrecision":8,"quoteCommissionPrecision":4,"orderTypes":["LIMIT","LIMIT_MAKER","MARKET","STOP_LOSS_LIMIT","TAKE_PROFIT_LIMIT"],"icebergAllowed":true,"ocoAllowed":true,"quoteOrderQtyMarketAllowed":true,"allowTrailingStop":true,"cancelReplaceAllowed":true,"isSpotTradingAllowed":true,"isMarginTradingAllowed":false,"filters":[{"filterType":"PRICE_FILTER","minPrice":"0.0001","maxPrice":"1000.0000","tickSize":"0.0001"},{"filterType":"PERCENT_PRICE","multiplierUp":"5","multiplierDown":"0.2","avgPriceMins":5},{"filterType":"LOT_SIZE","minQty":"1.00000000","maxQty":"9222449.00000000","stepSize":"1.00000000"},{"filterType":"MIN_NOTIONAL","minNotional":"1.0000","applyToMarket":true,"avgPriceMins":5},{"filterType":"ICEBERG_PARTS","limit":10},{"filterType":"MARKET_LOT_SIZE","minQty":"0.00000000","maxQty":"2797914.31380753","stepSize":"0.00000000"},{"filterType":"TRAILING_DELTA","minTrailingAboveDelta":10,"maxTrailingAboveDelta":2000,"minTrailingBelowDelta":10,"maxTrailingBelowDelta":2000},{"filterType":"PERCENT_PRICE_BY_SIDE","bidMultiplierUp":"5","bidMultiplierDown":"0.2","askMultiplierUp":"5","askMultiplierDown":"0.2","avgPriceMins":5},{"filterType":"MAX_NUM_ORDERS","maxNumOrders":200},{"filterType":"MAX_NUM_ALGO_ORDERS","maxNumAlgoOrders":5}],"permissions":["SPOT"],"defaultSelfTradePreventionMode":"EXPIRE_MAKER","allowedSelfTradePreventionModes":["EXPIRE_TAKER","EXPIRE_MAKER","EXPIRE_BOTH"]}]})";

using S9Int = moped::ScaledInteger<std::int64_t, 9>;

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

struct MopedTypeDefinitions {
  std::string name;
  std::string description;
};

BOOST_FIXTURE_TEST_SUITE(MopedUnitTests, MopedTypeDefinitions);

BOOST_AUTO_TEST_CASE(INSTRUMENT_DATA_LOAD_TEST) {
  static_assert(moped::IsSettable<std::optional<S9Int>>,
                "std::optional<S9Int> must be settable in MOPED");
  auto result =
      moped::parseCompositeFromJSONStream<ExchangeInfo>(instrumentData);
  if (!result.has_value()) {
    BOOST_FAIL("Failed to parse instrument data: " + result.error());
  }

  const ExchangeInfo &info = result.value();

  // Top-level checks
  BOOST_CHECK_EQUAL(info.timezone, "UTC");
  BOOST_CHECK_EQUAL(info.serverTime, moped::TimePoint{std::chrono::nanoseconds{
                                         1748461268460000000LL}});
  BOOST_CHECK_EQUAL(info.rateLimits.size(), 4);
  BOOST_CHECK_EQUAL(info.exchangeFilters.size(), 0);
  BOOST_CHECK_EQUAL(info.symbols.size(), 3);

  // Check first rate limit
  BOOST_CHECK_EQUAL(info.rateLimits[0].rateLimitType, "REQUEST_WEIGHT");
  BOOST_CHECK_EQUAL(info.rateLimits[0].interval, "MINUTE");
  BOOST_CHECK_EQUAL(info.rateLimits[0].intervalNum, 1);
  BOOST_CHECK_EQUAL(info.rateLimits[0].limit, 1200);

  // Check first symbol
  const Symbol &sym = info.symbols[0];
  BOOST_CHECK_EQUAL(sym.symbol, "BTCUSD4");
  BOOST_CHECK_EQUAL(sym.status, "BREAK");
  BOOST_CHECK_EQUAL(sym.baseAsset, "BTC");
  BOOST_CHECK_EQUAL(sym.baseAssetPrecision, 8);
  BOOST_CHECK_EQUAL(sym.quoteAsset, "USD4");
  BOOST_CHECK_EQUAL(sym.quotePrecision, 4);
  BOOST_CHECK_EQUAL(sym.quoteAssetPrecision, 4);
  BOOST_CHECK_EQUAL(sym.baseCommissionPrecision, 8);
  BOOST_CHECK_EQUAL(sym.quoteCommissionPrecision, 2);
  BOOST_CHECK_EQUAL(sym.orderTypes.size(), 5);
  BOOST_CHECK(sym.icebergAllowed);
  BOOST_CHECK(sym.ocoAllowed);
  BOOST_CHECK(sym.quoteOrderQtyMarketAllowed);
  BOOST_CHECK(sym.allowTrailingStop);
  BOOST_CHECK(sym.cancelReplaceAllowed);
  BOOST_CHECK(sym.isSpotTradingAllowed);
  BOOST_CHECK(!sym.isMarginTradingAllowed);
  BOOST_CHECK_EQUAL(sym.filters.size(), 9);
  BOOST_CHECK_EQUAL(sym.permissions.size(), 1);
  BOOST_CHECK_EQUAL(sym.permissions[0], "SPOT");
  BOOST_CHECK_EQUAL(sym.defaultSelfTradePreventionMode, "EXPIRE_MAKER");
  BOOST_CHECK_EQUAL(sym.allowedSelfTradePreventionModes.size(), 3);

  // Check a filter in the first symbol
  const Filter &filter = sym.filters[0];
  BOOST_CHECK_EQUAL(filter.filterType, "PRICE_FILTER");
  BOOST_CHECK(filter.minPrice.has_value());
  BOOST_CHECK_EQUAL(filter.minPrice.value(), S9Int{"0.0100"});
  BOOST_CHECK(filter.maxPrice.has_value());
  BOOST_CHECK_EQUAL(filter.maxPrice.value(), S9Int{"100000.0000"});
  BOOST_CHECK(filter.tickSize.has_value());
  BOOST_CHECK_EQUAL(filter.tickSize.value(), S9Int{"0.0100"});
}

BOOST_AUTO_TEST_SUITE_END();
