#pragma once
#include <chrono>
#include <cstdint>
#include <limits>

using Price = int64_t; // price in USD cents
using Quantity = int64_t;
using OrderId = uint64_t;
using UserId = uint64_t;
enum class Side : uint8_t {
  BUY,
  SELL
};

// Match Order would return this, to prevent self trades and also if not a self
// trade still contains the order
struct MatchResult {
  Quantity remaining;
  bool stpTriggered;
};

enum class AddResult : uint8_t {
  STP_CANCELLED, // Self Trade Prevention
  ADDED,
  FILLED,
  KILLED // FAK: partial fill, remainder discarded
};

enum class CancelResult : uint8_t {
  CANCELLED,
  NOT_FOUND
};

// Results for Modify Order
enum class ModifyResult : uint8_t {
  SUCCESS,
  ORDER_NOT_FOUND,
  INVALID_QUANTITY
};

// Currently I'm only supporting 3 types
enum class OrderType : uint8_t {
  // Kill order after matching
  FILL_AND_KILL,
  // Keep order in book until explicitly cancelled or filled
  GOOD_TILL_CANCEL,
  // No price, only quantity aka match until gone
  MARKET_ORDER,
};

enum class MarketPrice : Price {
  BUY = std::numeric_limits<int64_t>::max(),
  SELL = 0
};
