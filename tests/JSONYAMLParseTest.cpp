
#include "moped/mopedJSON.hpp"
#include "moped/mopedYAML.hpp"

#include "moped/tests/mdConfigSampleDefns.hpp"
#include <optional>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

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

void TestCompositeValues(auto &result) {

  BOOST_CHECK(result.has_value());
  auto &mdConfig = result.value();
  auto &mdService = mdConfig.marketDataService;
  BOOST_CHECK(mdService.sessions.size() == 1);
  BOOST_CHECK(mdService.sessions.contains("binanceSessionMDAws"));
  auto &session = mdService.sessions.at("binanceSessionMDAws");
  BOOST_CHECK(session.enabled);
  BOOST_CHECK_EQUAL(session.venue, "BINANCE");
  BOOST_CHECK_EQUAL(session.runContext, "binanceSpot");
  BOOST_CHECK_EQUAL(session.serviceContext, "aeronPubSub");
  BOOST_CHECK_EQUAL(session.maxRequestPerSecond, 4);
  BOOST_CHECK_EQUAL(session.maxSymbolsPerConnection, 50);
  BOOST_CHECK_EQUAL(session.subscriptions.size(), 3);
  BOOST_CHECK(session.subscriptions.contains("BTCUSDT"));
  BOOST_CHECK(session.subscriptions.contains("ETHUSDT"));
  BOOST_CHECK(session.subscriptions.contains("LTCUSDT"));
  auto &btcSubscription = session.subscriptions.at("BTCUSDT");
  BOOST_CHECK_EQUAL(btcSubscription.id, 1);
  BOOST_CHECK(btcSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::TopOfBook));
  BOOST_CHECK(btcSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::Trade));
  BOOST_CHECK(btcSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::DepthOfBook));
  BOOST_CHECK(!btcSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::FundingRate));
  BOOST_CHECK_EQUAL(btcSubscription.bookUpdateSettings.levels, 20);
  BOOST_CHECK(btcSubscription.bookUpdateSettings.arbTrades);
  BOOST_CHECK(btcSubscription.bookUpdateSettings.removeRestingOrders);
  auto &ethSubscription = session.subscriptions.at("ETHUSDT");
  BOOST_CHECK_EQUAL(ethSubscription.id, 2);
  BOOST_CHECK(ethSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::TopOfBook));
  BOOST_CHECK(ethSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::Trade));
  BOOST_CHECK(ethSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::DepthOfBook));
  BOOST_CHECK(!ethSubscription.subscribeFlags.has_value(
      moped::tests::SubscriptionFlags::FundingRate));
  BOOST_CHECK_EQUAL(ethSubscription.bookUpdateSettings.levels, 20);
  BOOST_CHECK(ethSubscription.bookUpdateSettings.arbTrades);
  BOOST_CHECK(ethSubscription.bookUpdateSettings.removeRestingOrders);
  auto &ltcSubscription = session.subscriptions.at("LTCUSDT");
  BOOST_CHECK_EQUAL(ltcSubscription.id, 3);
  auto &subscriptionEndpoint = session.subscriptionEndpoint;
  BOOST_CHECK_EQUAL(subscriptionEndpoint.name, "binanceSpotMD");
  BOOST_CHECK_EQUAL(subscriptionEndpoint.host, "stream.binance.com");
  BOOST_CHECK_EQUAL(subscriptionEndpoint.port, 9443);
  BOOST_CHECK_EQUAL(subscriptionEndpoint.uri, "/ws");
  auto &snapshotEndpoint = session.snapshotEndpoint;
  BOOST_CHECK_EQUAL(snapshotEndpoint.name, "binanceSpotSnapShot");
  BOOST_CHECK_EQUAL(snapshotEndpoint.host, "api.binance.com");
  BOOST_CHECK_EQUAL(snapshotEndpoint.port, 443);
  BOOST_CHECK_EQUAL(snapshotEndpoint.uri, "/api/v3/depth");
  auto &instrumentDataEndpoint = session.instrumentDataEndpoint;
  BOOST_CHECK_EQUAL(instrumentDataEndpoint.name, "binanceInstruments");
  BOOST_CHECK_EQUAL(instrumentDataEndpoint.host, "api.binance.com");
  BOOST_CHECK_EQUAL(instrumentDataEndpoint.port, 443);
  BOOST_CHECK_EQUAL(instrumentDataEndpoint.uri, "/api/v3/exchangeInfo");
}

struct TestFixture {};

BOOST_FIXTURE_TEST_SUITE(MopedYAMLUnitTests, TestFixture)

BOOST_AUTO_TEST_CASE(YAML_PARSE_TEST) {
  auto result = moped::parseCompositeFromYAMLView<moped::tests::MDConfig>(
      yamlTestMDConfig);
  TestCompositeValues(result);
}

BOOST_AUTO_TEST_CASE(JSON_PARSE_TEST) {
  auto result = moped::parseCompositeFromJSONView<moped::tests::MDConfig>(
      jsonTestMDConfig);
  TestCompositeValues(result);
}

BOOST_AUTO_TEST_SUITE_END();
