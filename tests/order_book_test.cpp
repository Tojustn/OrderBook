#include <catch2/catch_test_macros.hpp>
#include "order_book.hpp"
#include "order.hpp"
#include "types.hpp"

TEST_CASE("addOrder - buy order rests in bids", "[orderbook][add]") {
    OrderBook book;
    Order order(1, Side::BUY, 100, 50);
    book.addOrder(order);

    auto& bids = book.getBids();
    REQUIRE(bids.size() == 1);
    REQUIRE(bids.count(100) == 1);
    CHECK(bids.at(100).getTotalQuantity() == 50);
}

TEST_CASE("addOrder - sell order rests in asks", "[orderbook][add]") {
    OrderBook book;
    Order order(1, Side::SELL, 200, 30);
    book.addOrder(order);

    auto& asks = book.getAsks();
    REQUIRE(asks.size() == 1);
    REQUIRE(asks.count(200) == 1);
    CHECK(asks.at(200).getTotalQuantity() == 30);
}

TEST_CASE("addOrder - multiple buys at same price aggregate", "[orderbook][add]") {
    OrderBook book;
    Order order1(1, Side::BUY, 100, 50);
    Order order2(2, Side::BUY, 100, 30);
    book.addOrder(order1);
    book.addOrder(order2);

    auto& bids = book.getBids();
    REQUIRE(bids.size() == 1);
    CHECK(bids.at(100).getTotalQuantity() == 80);
    CHECK(bids.at(100).getOrders().size() == 2);
}

TEST_CASE("addOrder - different prices create separate levels", "[orderbook][add]") {
    OrderBook book;
    Order order1(1, Side::BUY, 100, 50);
    Order order2(2, Side::BUY, 200, 30);
    book.addOrder(order1);
    book.addOrder(order2);

    auto& bids = book.getBids();
    REQUIRE(bids.size() == 2);
    CHECK(bids.at(100).getTotalQuantity() == 50);
    CHECK(bids.at(200).getTotalQuantity() == 30);
}

TEST_CASE("addOrder - buy and sell go to separate sides", "[orderbook][add]") {
    OrderBook book;
    Order buy(1, Side::BUY, 100, 50);
    Order sell(2, Side::SELL, 200, 30);
    book.addOrder(buy);
    book.addOrder(sell);

    REQUIRE(book.getBids().size() == 1);
    REQUIRE(book.getAsks().size() == 1);
}

TEST_CASE("cancelOrder - cancels a buy order", "[orderbook][cancel]") {
    OrderBook book;
    Order order(1, Side::BUY, 100, 50);
    book.addOrder(order);
    book.cancelOrder(1);

    CHECK(book.getBids().at(100).getTotalQuantity() == 0);
}

TEST_CASE("cancelOrder - cancels a sell order", "[orderbook][cancel]") {
    OrderBook book;
    Order order(1, Side::SELL, 200, 30);
    book.addOrder(order);
    book.cancelOrder(1);

    CHECK(book.getAsks().at(200).getTotalQuantity() == 0);
}

TEST_CASE("cancelOrder - one order cancelled, other remains", "[orderbook][cancel]") {
    OrderBook book;
    Order order1(1, Side::BUY, 100, 50);
    Order order2(2, Side::BUY, 100, 30);
    book.addOrder(order1);
    book.addOrder(order2);
    book.cancelOrder(1);

    CHECK(book.getBids().at(100).getTotalQuantity() == 30);
    CHECK(book.getBids().at(100).getOrders().size() == 1);
}

TEST_CASE("cancelOrder - nonexistent order does nothing", "[orderbook][cancel]") {
    OrderBook book;
    Order order(1, Side::BUY, 100, 50);
    book.addOrder(order);
    book.cancelOrder(999);

    CHECK(book.getBids().at(100).getTotalQuantity() == 50);
}

TEST_CASE("cancelOrder - cancel same order twice is safe", "[orderbook][cancel]") {
    OrderBook book;
    Order order(1, Side::BUY, 100, 50);
    book.addOrder(order);
    book.cancelOrder(1);
    book.cancelOrder(1);

    CHECK(book.getBids().at(100).getTotalQuantity() == 0);
}

TEST_CASE("matching - buy fully fills resting sell", "[orderbook][matching]") {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 100, 50));
    book.addOrder(Order(2, Side::BUY, 100, 50));

    CHECK(book.getAsks().empty());
    CHECK(book.getBids().empty());
}

TEST_CASE("matching - buy partially fills resting sell, remainder rests", "[orderbook][matching]") {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 100, 30));
    book.addOrder(Order(2, Side::BUY, 100, 50));

    CHECK(book.getAsks().empty());
    REQUIRE(book.getBids().count(100) == 1);
    CHECK(book.getBids().at(100).getTotalQuantity() == 20);
}

TEST_CASE("matching - buy sweeps multiple ask levels", "[orderbook][matching]") {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 100, 20));
    book.addOrder(Order(2, Side::SELL, 101, 20));
    book.addOrder(Order(3, Side::BUY, 101, 50));

    CHECK(book.getAsks().empty());
    REQUIRE(book.getBids().count(101) == 1);
    CHECK(book.getBids().at(101).getTotalQuantity() == 10);
}

TEST_CASE("matching - buy price below ask, no match", "[orderbook][matching]") {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 102, 50));
    book.addOrder(Order(2, Side::BUY, 100, 50));

    CHECK(book.getAsks().at(102).getTotalQuantity() == 50);
    CHECK(book.getBids().at(100).getTotalQuantity() == 50);
}

TEST_CASE("matching - sell fully fills resting buy", "[orderbook][matching]") {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 100, 50));
    book.addOrder(Order(2, Side::SELL, 100, 50));

    CHECK(book.getBids().empty());
    CHECK(book.getAsks().empty());
}

TEST_CASE("matching - sell partially fills resting buy, remainder rests", "[orderbook][matching]") {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 100, 30));
    book.addOrder(Order(2, Side::SELL, 100, 50));

    CHECK(book.getBids().empty());
    REQUIRE(book.getAsks().count(100) == 1);
    CHECK(book.getAsks().at(100).getTotalQuantity() == 20);
}

TEST_CASE("matching - sell sweeps multiple bid levels", "[orderbook][matching]") {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 101, 20));
    book.addOrder(Order(2, Side::BUY, 100, 20));
    book.addOrder(Order(3, Side::SELL, 100, 50));

    CHECK(book.getBids().empty());
    REQUIRE(book.getAsks().count(100) == 1);
    CHECK(book.getAsks().at(100).getTotalQuantity() == 10);
}
