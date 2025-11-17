#pragma once
#include "moped/MappedEnumFlags.hpp"
#include "moped/moped.hpp"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace moped::tests {
// Endpoint definition
struct Endpoint {
  std::string name;
  std::string host;
  int port;
  std::string uri;

  template <moped::DecodingTraitsC DecodingTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<DecodingTraits, Endpoint>(
        "name", &Endpoint::name, "host", &Endpoint::host, "port",
        &Endpoint::port, "uri", &Endpoint::uri);
  }
};

// BookUpdateSettings definition
struct BookUpdateSettings {
  int levels;
  bool arbTrades;
  bool removeRestingOrders;

  template <moped::DecodingTraitsC DecodingTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<DecodingTraits, BookUpdateSettings>(
        "levels", &BookUpdateSettings::levels, "arbTrades",
        &BookUpdateSettings::arbTrades, "removeRestingOrders",
        &BookUpdateSettings::removeRestingOrders);
  }
};

// Subscription definition

inline constexpr char TopOfBook[] = "TopOfBook";
inline constexpr char Trade[] = "Trade";
inline constexpr char DepthOfBook[] = "DepthOfBook";
inline constexpr char FundingRate[] = "FundingRate";

enum class SubscriptionFlags : std::uint8_t {
  TopOfBook = 1 << 0,
  Trade = 1 << 1,
  DepthOfBook = 1 << 2,
  FundingRate = 1 << 3
};

using SubscriptionFlagsMappedEnum =
    moped::MappedEnum<TopOfBook, SubscriptionFlags::TopOfBook, Trade,
                      SubscriptionFlags::Trade, DepthOfBook,
                      SubscriptionFlags::DepthOfBook, FundingRate,
                      SubscriptionFlags::FundingRate>;

using SubscriptionFlagsT = moped::MappedEnumFlags<SubscriptionFlagsMappedEnum>;

struct Subscription {
  int id;
  SubscriptionFlagsT subscribeFlags;
  BookUpdateSettings bookUpdateSettings;

  template <moped::DecodingTraitsC DecodingTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<DecodingTraits, Subscription>(
        "id", &Subscription::id, "subscribeFlags",
        &Subscription::subscribeFlags, "bookUpdateSettings",
        &Subscription::bookUpdateSettings);
  }
};

// Session definition
struct Session {
  bool enabled;
  std::string venue;
  std::string runContext;
  std::string serviceContext;
  int maxRequestPerSecond;
  int maxSymbolsPerConnection;
  std::map<std::string, Subscription> subscriptions;
  Endpoint subscriptionEndpoint;
  Endpoint snapshotEndpoint;
  Endpoint instrumentDataEndpoint;

  template <moped::DecodingTraitsC DecodingTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<DecodingTraits, Session>(
        "enabled", &Session::enabled, "venue", &Session::venue, "runContext",
        &Session::runContext, "serviceContext", &Session::serviceContext,
        "maxRequestPerSecond", &Session::maxRequestPerSecond,
        "maxSymbolsPerConnection", &Session::maxSymbolsPerConnection,
        "subscriptions", &Session::subscriptions, "subscriptionEndpoint",
        &Session::subscriptionEndpoint, "snapshotEndpoint",
        &Session::snapshotEndpoint, "instrumentDataEndpoint",
        &Session::instrumentDataEndpoint);
  }
};

// Sessions map
struct Sessions {
  std::map<std::string, Session> sessions;

  template <moped::DecodingTraitsC DecodingTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<DecodingTraits, Sessions>("sessions",
                                                         &Sessions::sessions);
  }
};

// MarketDataService root
struct MarketDataService {
  std::map<std::string, Session> sessions;

  template <moped::DecodingTraitsC DecodingTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<DecodingTraits, MarketDataService>(
        "sessions", &MarketDataService::sessions);
  }
};

// Top-level config
struct MDConfig {
  MarketDataService marketDataService;

  template <moped::DecodingTraitsC DecodingTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<DecodingTraits, MDConfig>(
        "MarketDataService", &MDConfig::marketDataService);
  }
};

} // namespace moped::tests