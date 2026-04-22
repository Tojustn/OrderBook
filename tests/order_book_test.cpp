#include <catch2/catch_test_macros.hpp>
#include "order_book.hpp"
#include "order.hpp"
#include "types.hpp"

// ── addOrder ─────────────────────────────────────────────────────────────────

TEST_CASE("addOrder - buy order rests in bids", "[orderbook][add]") {
    OrderBook book;
    Order order(1, Side::BUY, 100, 50, 1);
    book.addOrder(order);

    auto& bids = book.getBids();
    REQUIRE(bids.size() == 1);
    REQUIRE(bids.count(100) == 1);
    CHECK(bids.at(100).getTotalQuantity() == 50);
}

TEST_CASE("addOrder - sell order rests in asks", "[orderbook][add]") {
    OrderBook book;
    Order order(1, Side::SELL, 200, 30, 1);
    book.addOrder(order);

    auto& asks = book.getAsks();
    REQUIRE(asks.size() == 1);
    REQUIRE(asks.count(200) == 1);
    CHECK(asks.at(200).getTotalQuantity() == 30);
}

TEST_CASE("addOrder - multiple buys at same price aggregate", "[orderbook][add]") {
    OrderBook book;
    Order order1(1, Side::BUY, 100, 50, 1);
    Order order2(2, Side::BUY, 100, 30, 2);
    book.addOrder(order1);
    book.addOrder(order2);

    auto& bids = book.getBids();
    REQUIRE(bids.size() == 1);
    CHECK(bids.at(100).getTotalQuantity() == 80);
}

TEST_CASE("addOrder - different prices create separate levels", "[orderbook][add]") {
    OrderBook book;
    Order order1(1, Side::BUY, 100, 50, 1);
    Order order2(2, Side::BUY, 200, 30, 2);
    book.addOrder(order1);
    book.addOrder(order2);

    auto& bids = book.getBids();
    REQUIRE(bids.size() == 2);
    CHECK(bids.at(100).getTotalQuantity() == 50);
    CHECK(bids.at(200).getTotalQuantity() == 30);
}

TEST_CASE("addOrder - buy and sell go to separate sides", "[orderbook][add]") {
    OrderBook book;
    Order buy(1, Side::BUY, 100, 50, 1);
    Order sell(2, Side::SELL, 200, 30, 2);
    book.addOrder(buy);
    book.addOrder(sell);

    REQUIRE(book.getBids().size() == 1);
    REQUIRE(book.getAsks().size() == 1);
}

// ── cancelOrder ───────────────────────────────────────────────────────────────

TEST_CASE("cancelOrder - cancels a buy order", "[orderbook][cancel]") {
    OrderBook book;
    Order order(1, Side::BUY, 100, 50, 1);
    book.addOrder(order);
    book.cancelOrder(1);

    CHECK(book.getBids().count(100) == 0);
}

TEST_CASE("cancelOrder - cancels a sell order", "[orderbook][cancel]") {
    OrderBook book;
    Order order(1, Side::SELL, 200, 30, 1);
    book.addOrder(order);
    book.cancelOrder(1);

    CHECK(book.getAsks().count(200) == 0);
}

TEST_CASE("cancelOrder - one order cancelled, other remains", "[orderbook][cancel]") {
    OrderBook book;
    Order order1(1, Side::BUY, 100, 50, 1);
    Order order2(2, Side::BUY, 100, 30, 2);
    book.addOrder(order1);
    book.addOrder(order2);
    book.cancelOrder(1);

    CHECK(book.getBids().at(100).getTotalQuantity() == 30);
}

TEST_CASE("cancelOrder - nonexistent order does nothing", "[orderbook][cancel]") {
    OrderBook book;
    Order order(1, Side::BUY, 100, 50, 1);
    book.addOrder(order);
    book.cancelOrder(999);

    CHECK(book.getBids().at(100).getTotalQuantity() == 50);
}

TEST_CASE("cancelOrder - cancel same order twice is safe", "[orderbook][cancel]") {
    OrderBook book;
    Order order(1, Side::BUY, 100, 50, 1);
    book.addOrder(order);
    book.cancelOrder(1);
    book.cancelOrder(1);

    CHECK(book.getBids().count(100) == 0);
}

TEST_CASE("cancelOrder - cancelling only order removes price level from bids", "[orderbook][cancel]") {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 100, 50, 1));
    book.cancelOrder(1);
    CHECK(book.getBids().count(100) == 0);
}

TEST_CASE("cancelOrder - cancelling only order removes price level from asks", "[orderbook][cancel]") {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 200, 30, 1));
    book.cancelOrder(1);
    CHECK(book.getAsks().count(200) == 0);
}

// ── matching ──────────────────────────────────────────────────────────────────

TEST_CASE("matching - buy fully fills resting sell", "[orderbook][matching]") {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 100, 50, 1));
    book.addOrder(Order(2, Side::BUY, 100, 50, 2));

    CHECK(book.getAsks().empty());
    CHECK(book.getBids().empty());
}

TEST_CASE("matching - buy partially fills resting sell, remainder rests", "[orderbook][matching]") {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 100, 30, 1));
    book.addOrder(Order(2, Side::BUY, 100, 50, 2));

    CHECK(book.getAsks().empty());
    REQUIRE(book.getBids().count(100) == 1);
    CHECK(book.getBids().at(100).getTotalQuantity() == 20);
}

TEST_CASE("matching - buy sweeps multiple ask levels", "[orderbook][matching]") {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 100, 20, 1));
    book.addOrder(Order(2, Side::SELL, 101, 20, 2));
    book.addOrder(Order(3, Side::BUY, 101, 50, 3));

    CHECK(book.getAsks().empty());
    REQUIRE(book.getBids().count(101) == 1);
    CHECK(book.getBids().at(101).getTotalQuantity() == 10);
}

TEST_CASE("matching - buy price below ask, no match", "[orderbook][matching]") {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 102, 50, 1));
    book.addOrder(Order(2, Side::BUY, 100, 50, 2));

    CHECK(book.getAsks().at(102).getTotalQuantity() == 50);
    CHECK(book.getBids().at(100).getTotalQuantity() == 50);
}

TEST_CASE("matching - sell fully fills resting buy", "[orderbook][matching]") {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 100, 50, 1));
    book.addOrder(Order(2, Side::SELL, 100, 50, 2));

    CHECK(book.getBids().empty());
    CHECK(book.getAsks().empty());
}

TEST_CASE("matching - sell partially fills resting buy, remainder rests", "[orderbook][matching]") {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 100, 30, 1));
    book.addOrder(Order(2, Side::SELL, 100, 50, 2));

    CHECK(book.getBids().empty());
    REQUIRE(book.getAsks().count(100) == 1);
    CHECK(book.getAsks().at(100).getTotalQuantity() == 20);
}

TEST_CASE("matching - sell sweeps multiple bid levels", "[orderbook][matching]") {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 101, 20, 1));
    book.addOrder(Order(2, Side::BUY, 100, 20, 2));
    book.addOrder(Order(3, Side::SELL, 100, 50, 3));

    CHECK(book.getBids().empty());
    REQUIRE(book.getAsks().count(100) == 1);
    CHECK(book.getAsks().at(100).getTotalQuantity() == 10);
}

// ── self-trade prevention ─────────────────────────────────────────────────────

TEST_CASE("stp - buy hits own resting sell, returns STP_CANCELLED", "[orderbook][stp]") {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 100, 50, 1));
    AddResult result = book.addOrder(Order(2, Side::BUY, 100, 50, 1));

    CHECK(result == AddResult::STP_CANCELLED);
    CHECK(book.getAsks().at(100).getTotalQuantity() == 50);
    CHECK(book.getBids().empty());
}

TEST_CASE("stp - sell hits own resting buy, returns STP_CANCELLED", "[orderbook][stp]") {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 100, 50, 1));
    AddResult result = book.addOrder(Order(2, Side::SELL, 100, 50, 1));

    CHECK(result == AddResult::STP_CANCELLED);
    CHECK(book.getBids().at(100).getTotalQuantity() == 50);
    CHECK(book.getAsks().empty());
}

TEST_CASE("stp - different users match normally", "[orderbook][stp]") {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 100, 50, 1));
    AddResult result = book.addOrder(Order(2, Side::BUY, 100, 50, 2));

    CHECK(result == AddResult::FILLED);
    CHECK(book.getAsks().empty());
    CHECK(book.getBids().empty());
}

TEST_CASE("stp - partial fill before own order, remainder cancelled", "[orderbook][stp]") {
    OrderBook book;
    // userId=2 fills first (20 qty), then userId=1's own order blocks the rest
    book.addOrder(Order(1, Side::SELL, 100, 20, 2));
    book.addOrder(Order(2, Side::SELL, 100, 30, 1));
    AddResult result = book.addOrder(Order(3, Side::BUY, 100, 50, 1));

    CHECK(result == AddResult::STP_CANCELLED);
    // own sell still resting intact
    CHECK(book.getAsks().at(100).getTotalQuantity() == 30);
    // remainder of buy not rested
    CHECK(book.getBids().empty());
}

// ── modifyOrder ─────────────────────────────────────────────────────────────

// Invalid input
TEST_CASE("modifyOrder - negative quantity returns INVALID_QUANTITY", "[orderbook][modify]") {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 100, 50, 1));
    CHECK(book.modifyOrder(1, -1) == ModifyResult::INVALID_QUANTITY);
    CHECK(book.getBids().at(100).getTotalQuantity() == 50);
}

// Order not found
TEST_CASE("modifyOrder - nonexistent order returns ORDER_NOT_FOUND", "[orderbook][modify]") {
    OrderBook book;
    CHECK(book.modifyOrder(999, 50) == ModifyResult::ORDER_NOT_FOUND);
}

// Zero quantity (cancel path)
TEST_CASE("modifyOrder - zero quantity cancels buy order", "[orderbook][modify]") {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 100, 50, 1));
    CHECK(book.modifyOrder(1, 0) == ModifyResult::SUCCESS);
    CHECK(book.getBids().count(100) == 0);
}

TEST_CASE("modifyOrder - zero quantity cancels sell order", "[orderbook][modify]") {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 200, 30, 1));
    CHECK(book.modifyOrder(1, 0) == ModifyResult::SUCCESS);
    CHECK(book.getAsks().count(200) == 0);
}

TEST_CASE("modifyOrder - zero quantity on nonexistent order returns ORDER_NOT_FOUND", "[orderbook][modify]") {
    OrderBook book;
    CHECK(book.modifyOrder(999, 0) == ModifyResult::ORDER_NOT_FOUND);
}

// Quantity update
TEST_CASE("modifyOrder - increase buy order quantity", "[orderbook][modify]") {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 100, 50, 1));
    CHECK(book.modifyOrder(1, 80) == ModifyResult::SUCCESS);
    CHECK(book.getBids().at(100).getTotalQuantity() == 80);
}

TEST_CASE("modifyOrder - decrease buy order quantity", "[orderbook][modify]") {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 100, 50, 1));
    CHECK(book.modifyOrder(1, 20) == ModifyResult::SUCCESS);
    CHECK(book.getBids().at(100).getTotalQuantity() == 20);
}

TEST_CASE("modifyOrder - increase sell order quantity", "[orderbook][modify]") {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 200, 30, 1));
    CHECK(book.modifyOrder(1, 60) == ModifyResult::SUCCESS);
    CHECK(book.getAsks().at(200).getTotalQuantity() == 60);
}

TEST_CASE("modifyOrder - decrease sell order quantity", "[orderbook][modify]") {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 200, 30, 1));
    CHECK(book.modifyOrder(1, 10) == ModifyResult::SUCCESS);
    CHECK(book.getAsks().at(200).getTotalQuantity() == 10);
}

// Multi-order level isolation
TEST_CASE("modifyOrder - multiple orders at level, only target modified", "[orderbook][modify]") {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 100, 50, 1));
    book.addOrder(Order(2, Side::BUY, 100, 30, 2));
    book.modifyOrder(1, 20);
    CHECK(book.getBids().at(100).getTotalQuantity() == 50); // 20 + 30
}

TEST_CASE("modifyOrder - zero quantity with multiple orders at level, other remains", "[orderbook][modify]") {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 100, 50, 1));
    book.addOrder(Order(2, Side::BUY, 100, 30, 2));
    book.modifyOrder(1, 0);
    REQUIRE(book.getBids().count(100) == 1);
    CHECK(book.getBids().at(100).getTotalQuantity() == 30);
}
