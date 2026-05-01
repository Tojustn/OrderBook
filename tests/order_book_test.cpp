#include "order.hpp"
#include "order_book.hpp"
#include "types.hpp"
#include <cassert>
#include <catch2/catch_test_macros.hpp>
#include <map>

static bool hasLevel(const std::map<Price, PriceLevel*>& m, Price p) { return m.count(p) > 0; }

static const PriceLevel& atLevel(const std::map<Price, PriceLevel*>& m, Price p) { return *m.at(p); }

// ── addOrder ─────────────────────────────────────────────────────────────────

TEST_CASE("addOrder - buy order rests in bids", "[orderbook][add]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::BUY, 100, 50, 1));

  auto& bids = book.getBids();
  REQUIRE(hasLevel(bids, 100));
  CHECK(atLevel(bids, 100).getTotalQuantity() == 50);
}

TEST_CASE("addOrder - sell order rests in asks", "[orderbook][add]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 200, 30, 1));

  auto& asks = book.getAsks();
  REQUIRE(hasLevel(asks, 200));
  CHECK(atLevel(asks, 200).getTotalQuantity() == 30);
}

TEST_CASE("addOrder - multiple buys at same price aggregate", "[orderbook][add]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::BUY, 100, 50, 1));
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(2, Side::BUY, 100, 30, 2));

  CHECK(atLevel(book.getBids(), 100).getTotalQuantity() == 80);
}

TEST_CASE("addOrder - different prices create separate levels", "[orderbook][add]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::BUY, 100, 50, 1));
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(2, Side::BUY, 200, 30, 2));

  REQUIRE(book.getBids().size() == 2);
  CHECK(atLevel(book.getBids(), 100).getTotalQuantity() == 50);
  CHECK(atLevel(book.getBids(), 200).getTotalQuantity() == 30);
}

// ── cancelOrder ───────────────────────────────────────────────────────────────

TEST_CASE("cancelOrder - removes only order, clears price level", "[orderbook][cancel]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::BUY, 100, 50, 1));
  book.cancelOrder(1);

  CHECK(!hasLevel(book.getBids(), 100));
}

TEST_CASE("cancelOrder - one order cancelled, other remains", "[orderbook][cancel]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::BUY, 100, 50, 1));
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(2, Side::BUY, 100, 30, 2));
  book.cancelOrder(1);

  CHECK(atLevel(book.getBids(), 100).getTotalQuantity() == 30);
}

TEST_CASE("cancelOrder - nonexistent order returns NOT_FOUND", "[orderbook][cancel]") {
  OrderBook book;
  CHECK(book.cancelOrder(999) == CancelResult::NOT_FOUND);
}

TEST_CASE("cancelOrder - cancel same order twice is safe", "[orderbook][cancel]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::BUY, 100, 50, 1));
  book.cancelOrder(1);
  CHECK(book.cancelOrder(1) == CancelResult::NOT_FOUND);
}

// ── matching ──────────────────────────────────────────────────────────────────

TEST_CASE("matching - buy fully fills resting sell", "[orderbook][matching]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 100, 50, 1));
  AddResult r = book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(2, Side::BUY, 100, 50, 2));

  CHECK(r == AddResult::FILLED);
  CHECK(book.getAsks().empty());
  CHECK(book.getBids().empty());
}

TEST_CASE("matching - buy partially fills resting sell, remainder rests", "[orderbook][matching]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 100, 30, 1));
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(2, Side::BUY, 100, 50, 2));

  CHECK(book.getAsks().empty());
  REQUIRE(hasLevel(book.getBids(), 100));
  CHECK(atLevel(book.getBids(), 100).getTotalQuantity() == 20);
}

TEST_CASE("matching - buy sweeps multiple ask levels", "[orderbook][matching]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 100, 20, 1));
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(2, Side::SELL, 101, 20, 2));
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(3, Side::BUY, 101, 50, 3));

  CHECK(book.getAsks().empty());
  REQUIRE(hasLevel(book.getBids(), 101));
  CHECK(atLevel(book.getBids(), 101).getTotalQuantity() == 10);
}

// ── self-trade prevention ─────────────────────────────────────────────────────

TEST_CASE("stp - buy hits own resting sell", "[orderbook][stp]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 100, 50, 1));
  AddResult r = book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(2, Side::BUY, 100, 50, 1));

  CHECK(r == AddResult::STP_CANCELLED);
  CHECK(atLevel(book.getAsks(), 100).getTotalQuantity() == 50);
  CHECK(book.getBids().empty());
}

TEST_CASE("stp - sell hits own resting buy", "[orderbook][stp]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::BUY, 100, 50, 1));
  AddResult r = book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(2, Side::SELL, 100, 50, 1));

  CHECK(r == AddResult::STP_CANCELLED);
  CHECK(atLevel(book.getBids(), 100).getTotalQuantity() == 50);
  CHECK(book.getAsks().empty());
}

TEST_CASE("stp - different users match normally", "[orderbook][stp]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 100, 50, 1));
  AddResult r = book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(2, Side::BUY, 100, 50, 2));

  CHECK(r == AddResult::FILLED);
  CHECK(book.getAsks().empty());
  CHECK(book.getBids().empty());
}

TEST_CASE("stp - partial fill before own order, remainder cancelled", "[orderbook][stp]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 100, 20, 2));
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(2, Side::SELL, 100, 30, 1));
  AddResult r = book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(3, Side::BUY, 100, 50, 1));

  CHECK(r == AddResult::STP_CANCELLED);
  CHECK(atLevel(book.getAsks(), 100).getTotalQuantity() == 30);
  CHECK(book.getBids().empty());
}

// ── modifyOrder ───────────────────────────────────────────────────────────────

TEST_CASE("modifyOrder - negative quantity returns INVALID_QUANTITY", "[orderbook][modify]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::BUY, 100, 50, 1));
  CHECK(book.modifyOrder(1, -1) == ModifyResult::INVALID_QUANTITY);
}

TEST_CASE("modifyOrder - change quantity", "[orderbook][modify]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::BUY, 100, 50, 1));
  book.modifyOrder(1, 80);
  CHECK(atLevel(book.getBids(), 100).getTotalQuantity() == 80);
}

TEST_CASE("modifyOrder - multiple orders at level, only target modified", "[orderbook][modify]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::BUY, 100, 50, 1));
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(2, Side::BUY, 100, 30, 2));
  book.modifyOrder(1, 20);
  CHECK(atLevel(book.getBids(), 100).getTotalQuantity() == 50); // 20 + 30
}

// ── bestBid / bestAsk ─────────────────────────────────────────────────────────

TEST_CASE("bestBid - set on add, null after cancel", "[orderbook][best]") {
  OrderBook book;
  CHECK(book.getBestBid() == nullptr);
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::BUY, 100, 50, 1));
  CHECK(book.getBestBid() != nullptr);
  CHECK(book.getBestBid()->getTotalQuantity() == 50);
  book.cancelOrder(1);
  CHECK(book.getBestBid() == nullptr);
}

TEST_CASE("bestAsk - set on add, null after cancel", "[orderbook][best]") {
  OrderBook book;
  CHECK(book.getBestAsk() == nullptr);
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 200, 30, 1));
  CHECK(book.getBestAsk() != nullptr);
  CHECK(book.getBestAsk()->getTotalQuantity() == 30);
  book.cancelOrder(1);
  CHECK(book.getBestAsk() == nullptr);
}

TEST_CASE("bestAsk - updates after match consumes best level", "[orderbook][best]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 100, 20, 1));
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(2, Side::SELL, 101, 20, 2));
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(3, Side::BUY, 100, 20, 3));

  REQUIRE(book.getBestAsk() != nullptr);
  CHECK(book.getBestAsk()->getTotalQuantity() == 20); // now points to 101 level
}

// ── order types ───────────────────────────────────────────────────────────────

TEST_CASE("FAK - partial match, remainder killed", "[orderbook][ordertype]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 100, 30, 1));
  AddResult r = book.addOrder(OrderType::FILL_AND_KILL, Order(2, Side::BUY, 100, 50, 2));

  CHECK(r == AddResult::KILLED);
  CHECK(book.getAsks().empty());
  CHECK(book.getBids().empty()); // remainder not rested
}

TEST_CASE("FAK - full match returns FILLED", "[orderbook][ordertype]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 100, 50, 1));
  AddResult r = book.addOrder(OrderType::FILL_AND_KILL, Order(2, Side::BUY, 100, 50, 2));

  CHECK(r == AddResult::FILLED);
  CHECK(book.getAsks().empty());
}

TEST_CASE("FAK - no match, order killed immediately", "[orderbook][ordertype]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 200, 50, 1));
  AddResult r = book.addOrder(OrderType::FILL_AND_KILL, Order(2, Side::BUY, 100, 50, 2));

  CHECK(r == AddResult::KILLED);
  CHECK(book.getBids().empty());
}

TEST_CASE("market order - buy fills against best ask", "[orderbook][ordertype]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 100, 50, 1));
  AddResult r = book.addOrder(2, Side::BUY, 50, 2);

  CHECK(r == AddResult::FILLED);
  CHECK(book.getAsks().empty());
}

TEST_CASE("market order - sell fills against best bid", "[orderbook][ordertype]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::BUY, 100, 50, 1));
  AddResult r = book.addOrder(2, Side::SELL, 50, 2);

  CHECK(r == AddResult::FILLED);
  CHECK(book.getBids().empty());
}

TEST_CASE("market order - never rests on book", "[orderbook][ordertype]") {
  OrderBook book;
  book.addOrder(2, Side::BUY, 50, 2); // no asks to match

  CHECK(book.getBids().empty());
  CHECK(book.getAsks().empty());
}

TEST_CASE("FOK - full match returns FILLED", "[orderbook][ordertype]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::BUY, 100, 100, 2));
  AddResult r = book.addOrder(OrderType::FILL_OR_KILL, Order(2, Side::SELL, 100, 100, 1));
  CHECK(r == AddResult::FILLED);
  CHECK(book.getBids().empty());
}

TEST_CASE("FOK - insufficient quantity, order killed", "[orderbook][ordertype]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::BUY, 100, 50, 1));
  AddResult r = book.addOrder(OrderType::FILL_OR_KILL, Order(2, Side::SELL, 100, 100, 2));
  CHECK(r == AddResult::KILLED);
  CHECK(!book.getBids().empty()); // resting order untouched
}

TEST_CASE("FOK - STP cancels order", "[orderbook][ordertype]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::BUY, 100, 100, 1));
  AddResult r = book.addOrder(OrderType::FILL_OR_KILL, Order(2, Side::SELL, 100, 100, 1));
  CHECK(r == AddResult::KILLED);
  CHECK(!book.getBids().empty()); // resting order untouched
}

TEST_CASE("POST_ONLY - rests on book when no match", "[orderbook][ordertype]") {
  OrderBook book;
  AddResult r = book.addOrder(OrderType::POST_ONLY, Order(1, Side::BUY, 100, 50, 1));
  CHECK(r == AddResult::ADDED);
  CHECK(!book.getBids().empty());
}

TEST_CASE("POST_ONLY - killed if it would match", "[orderbook][ordertype]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 100, 50, 1));
  AddResult r = book.addOrder(OrderType::POST_ONLY, Order(2, Side::BUY, 100, 50, 2));
  CHECK(r == AddResult::KILLED);
  CHECK(!book.getAsks().empty()); // resting sell untouched
  CHECK(book.getBids().empty());
}

TEST_CASE("POST_ONLY - rests when price does not cross", "[orderbook][ordertype]") {
  OrderBook book;
  book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 101, 50, 1));
  AddResult r = book.addOrder(OrderType::POST_ONLY, Order(2, Side::BUY, 100, 50, 2));
  CHECK(r == AddResult::ADDED);
  CHECK(!book.getBids().empty());
  CHECK(!book.getAsks().empty());
}
