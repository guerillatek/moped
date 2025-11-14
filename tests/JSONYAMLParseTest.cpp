
#include "moped/TimeFormatters.hpp"
#include "moped/mopedJSON.hpp"
#include "moped/mopedYAML.hpp"

#include <catch2/catch.hpp>

#include "moped/tests/mdConfigSampleDefns.hpp"
#include <optional>
#include <string>
#include <vector>

using DFTF = moped::DefaultTimePointFormatter<>;

    std::string_view jsonTestMDConfig = R"(
{
  "MarketDataService": {
    "sessions": {
      "binanceSessionMDAws": {
        "enabled": true,
        "venue": "BINANCE",
        "runContext": "binanceSpot",
        "serviceContext": "aeronPubSub",
        "maxRequestPerSecond": 4,
        "maxSymbolsPerConnection": 50,
        "subscriptions": {
          "BTCUSDT": {
            "bookUpdateSettings": {
              "levels": 20,
              "arbTrades": true,
              "removeRestingOrders": true
            },
            "id": 1,
            "subscribeFlags": [
              "TopOfBook",
              "Trade",
              "DepthOfBook"
            ]
          },
          "ETHUSDT": {
            "id": 2,
            "subscribeFlags": [
              "TopOfBook",
              "Trade",
              "DepthOfBook"
            ],
            "bookUpdateSettings": {
              "levels": 20,
              "arbTrades": true,
              "removeRestingOrders": true
            }
          },
          "LTCUSDT": {
            "id": 3,
            "subscribeFlags": [
              "TopOfBook",
              "Trade",
              "DepthOfBook"
            ],
            "bookUpdateSettings": {
              "levels": 20,
              "arbTrades": true,
              "removeRestingOrders": true
            }
          }
        },
        "subscriptionEndpoint": {
          "name": "binanceSpotMD",
          "host": "stream.binance.com",
          "port": 9443,
          "uri": "/ws"
        },
        "snapshotEndpoint": {
          "name": "binanceSpotSnapShot",
          "host": "api.binance.com",
          "port": 443,
          "uri": "/api/v3/depth"
        },
        "instrumentDataEndpoint": {
          "name": "binanceInstruments",
          "host": "api.binance.com",
          "port": 443,
          "uri": "/api/v3/exchangeInfo"
        }
      }
    }
  }
}
)";

std::string_view yamlTestMDConfig = R"(
MarketDataService:
  sessions:
    binanceSessionMDAws:
      enabled: true
      venue: BINANCE
      runContext: binanceSpot
      serviceContext: aeronPubSub
      maxRequestPerSecond: 4
      maxSymbolsPerConnection: 50
      subscriptions:
        BTCUSDT:
          bookUpdateSettings:
            levels: 20
            arbTrades: true
            removeRestingOrders: true
          id: 1
          subscribeFlags: 
            - TopOfBook
            - Trade
            - DepthOfBook
        ETHUSDT:
          id: 2
          subscribeFlags: 
            - TopOfBook
            - Trade
            - DepthOfBook
          bookUpdateSettings:
            levels: 20
            arbTrades: true
            removeRestingOrders: true
        LTCUSDT:
          id: 3
          subscribeFlags: 
            - TopOfBook
            - Trade
            - DepthOfBook
          bookUpdateSettings:
            levels: 20
            arbTrades: true
            removeRestingOrders: true
      subscriptionEndpoint: 
        name: "binanceSpotMD" 
        host: "stream.binance.com" 
        port: 9443 
        uri : "/ws"
      snapshotEndpoint:
        name: "binanceSpotSnapShot" 
        host: "api.binance.com" 
        port: 443 
        uri: "/api/v3/depth"
      instrumentDataEndpoint:
        name: "binanceInstruments" 
        host: "api.binance.com" 
        port: 443 
        uri: "/api/v3/exchangeInfo"   
)";

#define ASSERT_EQ(A, B) REQUIRE(A == B)

void TestCompositeValues(auto &result) {

  REQUIRE(result.has_value());
  auto &mdConfig = result.value();
  auto &mdService = mdConfig.marketDataService;
  REQUIRE(mdService.sessions.size() == 1);
  REQUIRE(mdService.sessions.contains("binanceSessionMDAws"));
  auto &session = mdService.sessions.at("binanceSessionMDAws");
  REQUIRE(session.enabled);
  ASSERT_EQ(session.venue, "BINANCE");
  ASSERT_EQ(session.runContext, "binanceSpot");
  ASSERT_EQ(session.serviceContext, "aeronPubSub");
  ASSERT_EQ(session.maxRequestPerSecond, 4);
  ASSERT_EQ(session.maxSymbolsPerConnection, 50);
  ASSERT_EQ(session.subscriptions.size(), 3);
  REQUIRE(session.subscriptions.contains("BTCUSDT"));
  REQUIRE(session.subscriptions.contains("ETHUSDT"));
  REQUIRE(session.subscriptions.contains("LTCUSDT"));
  auto &btcSubscription = session.subscriptions.at("BTCUSDT");
  ASSERT_EQ(btcSubscription.id, 1);
  REQUIRE(btcSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::TopOfBook));
  REQUIRE(btcSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::Trade));
  REQUIRE(btcSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::DepthOfBook));
  REQUIRE(!btcSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::FundingRate));
  ASSERT_EQ(btcSubscription.bookUpdateSettings.levels, 20);
  REQUIRE(btcSubscription.bookUpdateSettings.arbTrades);
  REQUIRE(btcSubscription.bookUpdateSettings.removeRestingOrders);
  auto &ethSubscription = session.subscriptions.at("ETHUSDT");
  ASSERT_EQ(ethSubscription.id, 2);
  REQUIRE(ethSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::TopOfBook));
  REQUIRE(ethSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::Trade));
  REQUIRE(ethSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::DepthOfBook));
  REQUIRE(!ethSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::FundingRate));
  ASSERT_EQ(ethSubscription.bookUpdateSettings.levels, 20);
  REQUIRE(ethSubscription.bookUpdateSettings.arbTrades);
  REQUIRE(ethSubscription.bookUpdateSettings.removeRestingOrders);
  auto &ltcSubscription = session.subscriptions.at("LTCUSDT");
  ASSERT_EQ(ltcSubscription.id, 3);
  auto &subscriptionEndpoint = session.subscriptionEndpoint;
  ASSERT_EQ(subscriptionEndpoint.name, "binanceSpotMD");
  ASSERT_EQ(subscriptionEndpoint.host, "stream.binance.com");
  ASSERT_EQ(subscriptionEndpoint.port, 9443);
  ASSERT_EQ(subscriptionEndpoint.uri, "/ws");
  auto &snapshotEndpoint = session.snapshotEndpoint;
  ASSERT_EQ(snapshotEndpoint.name, "binanceSpotSnapShot");
  ASSERT_EQ(snapshotEndpoint.host, "api.binance.com");
  ASSERT_EQ(snapshotEndpoint.port, 443);
  ASSERT_EQ(snapshotEndpoint.uri, "/api/v3/depth");
  auto &instrumentDataEndpoint = session.instrumentDataEndpoint;
  ASSERT_EQ(instrumentDataEndpoint.name, "binanceInstruments");
  ASSERT_EQ(instrumentDataEndpoint.host, "api.binance.com");
  ASSERT_EQ(instrumentDataEndpoint.port, 443);
  ASSERT_EQ(instrumentDataEndpoint.uri, "/api/v3/exchangeInfo");
}

TEST_CASE("Loading a Market data service config definition with moped mapping "
          "using YAML  "
          "[YAML DATA SERVICE CONFIG]") {
  auto result = moped::parseCompositeFromYAMLView<moped::tests::MDConfig>(
      DFTF{}, yamlTestMDConfig);
  TestCompositeValues(result);
}

TEST_CASE("Same definition, Same mapping but now loading with JSON"
          "[JSON MARKET DATA SERVICE]") {
  auto result = moped::parseCompositeFromJSONView<moped::tests::MDConfig>(
      DFTF{}, jsonTestMDConfig);
  TestCompositeValues(result);
}
