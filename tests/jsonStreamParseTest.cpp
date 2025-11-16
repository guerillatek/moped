
#include <catch2/catch.hpp>

#include "moped/JSONEmitterContext.hpp"
#include "moped/TimeFormatters.hpp"
#include "moped/tests/mopedTestTypes.hpp"

#include <optional>
using DFTF = moped::DurationSinceEpochFormatter<std::chrono::milliseconds>;

std::string_view instrumentData =
    R"({"timezone":"UTC","serverTime":1748461268460,"rateLimits":[{"rateLimitType":"REQUEST_WEIGHT","interval":"MINUTE","intervalNum":1,"limit":1200},{"rateLimitType":"ORDERS","interval":"SECOND","intervalNum":10,"limit":100},{"rateLimitType":"ORDERS","interval":"DAY","intervalNum":1,"limit":200000},{"rateLimitType":"RAW_REQUESTS","interval":"MINUTE","intervalNum":5,"limit":6100}],"exchangeFilters":[],"symbols":[{"symbol":"BTCUSD4","status":"BREAK","baseAsset":"BTC","baseAssetPrecision":8,"quoteAsset":"USD4","quotePrecision":4,"quoteAssetPrecision":4,"baseCommissionPrecision":8,"quoteCommissionPrecision":2,"orderTypes":["LIMIT","LIMIT_MAKER","MARKET","STOP_LOSS_LIMIT","TAKE_PROFIT_LIMIT"],"icebergAllowed":true,"ocoAllowed":true,"quoteOrderQtyMarketAllowed":true,"allowTrailingStop":true,"cancelReplaceAllowed":true,"isSpotTradingAllowed":true,"isMarginTradingAllowed":false,"filters":[{"filterType":"PRICE_FILTER","minPrice":"0.0100","maxPrice":"100000.0000","tickSize":"0.0100"},{"filterType":"PERCENT_PRICE","multiplierUp":"5","multiplierDown":"0.2","avgPriceMins":5},{"filterType":"LOT_SIZE","minQty":"0.00000100","maxQty":"9000.00000000","stepSize":"0.00000100"},{"filterType":"MIN_NOTIONAL","minNotional":"10.0000","applyToMarket":true,"avgPriceMins":5},{"filterType":"ICEBERG_PARTS","limit":10},{"filterType":"MARKET_LOT_SIZE","minQty":"0.00000000","maxQty":"60.25597724","stepSize":"0.00000000"},{"filterType":"TRAILING_DELTA","minTrailingAboveDelta":10,"maxTrailingAboveDelta":2000,"minTrailingBelowDelta":10,"maxTrailingBelowDelta":2000},{"filterType":"MAX_NUM_ORDERS","maxNumOrders":200},{"filterType":"MAX_NUM_ALGO_ORDERS","maxNumAlgoOrders":5}],"permissions":["SPOT"],"defaultSelfTradePreventionMode":"EXPIRE_MAKER","allowedSelfTradePreventionModes":["EXPIRE_TAKER","EXPIRE_MAKER","EXPIRE_BOTH"]},{"symbol":"ETHUSD4","status":"BREAK","baseAsset":"ETH","baseAssetPrecision":8,"quoteAsset":"USD4","quotePrecision":4,"quoteAssetPrecision":4,"baseCommissionPrecision":8,"quoteCommissionPrecision":4,"orderTypes":["LIMIT","LIMIT_MAKER","MARKET","STOP_LOSS_LIMIT","TAKE_PROFIT_LIMIT"],"icebergAllowed":true,"ocoAllowed":true,"quoteOrderQtyMarketAllowed":true,"allowTrailingStop":true,"cancelReplaceAllowed":true,"isSpotTradingAllowed":true,"isMarginTradingAllowed":false,"filters":[{"filterType":"PRICE_FILTER","minPrice":"0.0100","maxPrice":"100000.0000","tickSize":"0.0100"},{"filterType":"PERCENT_PRICE","multiplierUp":"5","multiplierDown":"0.2","avgPriceMins":5},{"filterType":"LOT_SIZE","minQty":"0.00001000","maxQty":"9000.00000000","stepSize":"0.00001000"},{"filterType":"MIN_NOTIONAL","minNotional":"10.0000","applyToMarket":true,"avgPriceMins":5},{"filterType":"ICEBERG_PARTS","limit":10},{"filterType":"MARKET_LOT_SIZE","minQty":"0.00000000","maxQty":"514.38625342","stepSize":"0.00000000"},{"filterType":"TRAILING_DELTA","minTrailingAboveDelta":10,"maxTrailingAboveDelta":2000,"minTrailingBelowDelta":10,"maxTrailingBelowDelta":2000},{"filterType":"MAX_NUM_ORDERS","maxNumOrders":200},{"filterType":"MAX_NUM_ALGO_ORDERS","maxNumAlgoOrders":5}],"permissions":["SPOT"],"defaultSelfTradePreventionMode":"EXPIRE_MAKER","allowedSelfTradePreventionModes":["EXPIRE_TAKER","EXPIRE_MAKER","EXPIRE_BOTH"]},{"symbol":"XRPUSD4","status":"BREAK","baseAsset":"XRP","baseAssetPrecision":8,"quoteAsset":"USD4","quotePrecision":4,"quoteAssetPrecision":4,"baseCommissionPrecision":8,"quoteCommissionPrecision":4,"orderTypes":["LIMIT","LIMIT_MAKER","MARKET","STOP_LOSS_LIMIT","TAKE_PROFIT_LIMIT"],"icebergAllowed":true,"ocoAllowed":true,"quoteOrderQtyMarketAllowed":true,"allowTrailingStop":true,"cancelReplaceAllowed":true,"isSpotTradingAllowed":true,"isMarginTradingAllowed":false,"filters":[{"filterType":"PRICE_FILTER","minPrice":"0.0001","maxPrice":"1000.0000","tickSize":"0.0001"},{"filterType":"PERCENT_PRICE","multiplierUp":"5","multiplierDown":"0.2","avgPriceMins":5},{"filterType":"LOT_SIZE","minQty":"1.00000000","maxQty":"9222449.00000000","stepSize":"1.00000000"},{"filterType":"MIN_NOTIONAL","minNotional":"1.0000","applyToMarket":true,"avgPriceMins":5},{"filterType":"ICEBERG_PARTS","limit":10},{"filterType":"MARKET_LOT_SIZE","minQty":"0.00000000","maxQty":"2797914.31380753","stepSize":"0.00000000"},{"filterType":"TRAILING_DELTA","minTrailingAboveDelta":10,"maxTrailingAboveDelta":2000,"minTrailingBelowDelta":10,"maxTrailingBelowDelta":2000},{"filterType":"PERCENT_PRICE_BY_SIDE","bidMultiplierUp":"5","bidMultiplierDown":"0.2","askMultiplierUp":"5","askMultiplierDown":"0.2","avgPriceMins":5},{"filterType":"MAX_NUM_ORDERS","maxNumOrders":200},{"filterType":"MAX_NUM_ALGO_ORDERS","maxNumAlgoOrders":5}],"permissions":["SPOT"],"defaultSelfTradePreventionMode":"EXPIRE_MAKER","allowedSelfTradePreventionModes":["EXPIRE_TAKER","EXPIRE_MAKER","EXPIRE_BOTH"]}]})";

using namespace moped::tests;
using namespace moped::tests::stream;

#define ASSERT_EQ(A, B) REQUIRE(A == B)

TEST_CASE("Instrument Data load Test with streaming parser and no "
          "std::string_view member support",
          "[JSON STREAMING PARSER]") {

  auto result =
      moped::parseCompositeFromJSONStream<ExchangeInfo>(DFTF{}, instrumentData);
  REQUIRE(result);
  const ExchangeInfo &info = result.value();

  // Top-level checks
  ASSERT_EQ(info.timezone, "UTC");
  ASSERT_EQ(info.serverTime,
            moped::TimePoint{std::chrono::nanoseconds{1748461268460000000LL}});
  ASSERT_EQ(info.rateLimits.size(), 4);
  ASSERT_EQ(info.exchangeFilters.size(), 0);
  ASSERT_EQ(info.symbols.size(), 3);

  // Check first rate limit
  ASSERT_EQ(info.rateLimits.empty(), false);
  ASSERT_EQ(info.rateLimits[0].rateLimitType, "REQUEST_WEIGHT");
  ASSERT_EQ(info.rateLimits[0].interval, "MINUTE");
  ASSERT_EQ(info.rateLimits[0].intervalNum, 1);
  ASSERT_EQ(info.rateLimits[0].limit, 1200);

  // Check first symbol

  const Symbol &sym = info.symbols[0];
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
  const Filter &filter = sym.filters[0];
  ASSERT_EQ(filter.filterType, "PRICE_FILTER");
  REQUIRE(filter.minPrice.has_value());
  ASSERT_EQ(filter.minPrice.value(), S9Int{"0.0100"});
  REQUIRE(filter.maxPrice.has_value());
  ASSERT_EQ(filter.maxPrice.value(), S9Int{"100000.0000"});
  REQUIRE(filter.tickSize.has_value());
  ASSERT_EQ(filter.tickSize.value(), S9Int{"0.0100"});
  moped::encodeToJSONStream(info, std::cout, moped::ISO8601GMTFormatter<>{});
}

TEST_CASE("Instrument Data load Test with streaming parser and  "
          "dispatching collection handlers for symbols",
          "[JSON STREAMING PARSER_DISPATCH]") {

  std::vector<Symbol> capturedSymbols;
  auto result =
      moped::parseCompositeFromJSONStream<ExchangeInfoWithSymbolDispatch>(
          DFTF{}, instrumentData,
          moped::CollectionFunctionDispatcher<Symbol>{
              [&capturedSymbols](const Symbol &sym) {
                capturedSymbols.push_back(sym);
              }});
  if (!result) {
    FAIL(result.error().c_str());
  }
  const ExchangeInfoWithSymbolDispatch &info = result.value();

  // Check first symbol
  const Symbol &sym = capturedSymbols[0];
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

  ASSERT_EQ(capturedSymbols.size(), 3);
}