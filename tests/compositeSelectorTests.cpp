#include "moped/JSONStreamParser.hpp"
#include "moped/MappedCompositeSelector.hpp"
#include "moped/MappedObjectParseEncoderDispatcher.hpp"
#include "moped/ScaledInteger.hpp"

namespace moped {

using S9Int = moped::ScaledInteger<std::int64_t, 9>;

enum class Side { BUY, SELL };

struct LastTradeEventT {
  std::string securityId;     // Maps to "s" (Symbol)
  S9Int price;                // Maps to "p" (Price)
  S9Int quantity;             // Maps to "q" (Quantity)
  uint64_t eventId;           // Maps to "t" (Trade ID) - removed optional
  moped::TimePoint eventTime; // Maps to "E" (Event time) - removed optional
  Side aggressor;             // Maps to "m" (Is buyer market maker)
  std::uint64_t instrumentServiceId{}; // Not in JSON, set externally

  // Additional JSON fields (can keep as optional if they might be missing)
  std::string eventType;  // Maps to "e" (Event type) - removed optional
  uint64_t buyerOrderId;  // Maps to "b" (Buyer order ID) - removed optional
  uint64_t sellerOrderId; // Maps to "a" (Seller order ID) - removed optional
  moped::TimePoint tradeTime; // Maps to "T" (Trade time) - removed optional
  bool ignore;                // Maps to "M" (Ignore) - removed optional
  bool isBuyerMarketMaker;    // Maps to "m" (Is buyer market maker) - removed
                              // optional
  // Constructor to handle non-JSON fields

  template <moped::MemberIdTraitsC MemberIdTraits>
  static auto getMOPEDHandler() {
    return moped::mopedHandler<MemberIdTraits, LastTradeEventT>(
        "e", &LastTradeEventT::eventType,          // Event type
        "E", &LastTradeEventT::eventTime,          // Event time
        "s", &LastTradeEventT::securityId,         // Symbol
        "t", &LastTradeEventT::eventId,            // Trade ID
        "p", &LastTradeEventT::price,              // Price
        "q", &LastTradeEventT::quantity,           // Quantity
        "b", &LastTradeEventT::buyerOrderId,       // Buyer order ID
        "a", &LastTradeEventT::sellerOrderId,      // Seller order ID
        "T", &LastTradeEventT::tradeTime,          // Trade time
        "m", &LastTradeEventT::isBuyerMarketMaker, // Is buyer market maker
        "M", &LastTradeEventT::ignore              // Ignore field
    );
  }
};

} // namespace moped