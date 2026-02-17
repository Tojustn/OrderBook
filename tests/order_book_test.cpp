#include <catch2/catch_test_macros.hpp>
#include "order_book.hpp"
#include "order.hpp"
#include "types.hpp"

TEST_CASE("OrderBook addOrder", "[orderbook]") {
    OrderBook book;

    SECTION("adds buy order to bids") {
        Order order(1, Side::BUY, 100, 50);
        book.addOrder(order);

        auto& bids = book.getBids();
        REQUIRE(bids.size() == 1);
        REQUIRE(bids.count(100) == 1);
        CHECK(bids.at(100).getTotalQuantity() == 50);
    }

    SECTION("adds sell order to asks") {
        Order order(1, Side::SELL, 200, 30);
        book.addOrder(order);

        auto& asks = book.getAsks();
        REQUIRE(asks.size() == 1);
        REQUIRE(asks.count(200) == 1);
        CHECK(asks.at(200).getTotalQuantity() == 30);
    }

    SECTION("multiple buy orders at same price") {
        Order order1(1, Side::BUY, 100, 50);
        Order order2(2, Side::BUY, 100, 30);
        book.addOrder(order1);
        book.addOrder(order2);

        auto& bids = book.getBids();
        REQUIRE(bids.size() == 1);
        CHECK(bids.at(100).getTotalQuantity() == 80);
        CHECK(bids.at(100).getOrders().size() == 2);
    }

    SECTION("orders at different prices create separate levels") {
        Order order1(1, Side::BUY, 100, 50);
        Order order2(2, Side::BUY, 200, 30);
        book.addOrder(order1);
        book.addOrder(order2);

        auto& bids = book.getBids();
        REQUIRE(bids.size() == 2);
        CHECK(bids.at(100).getTotalQuantity() == 50);
        CHECK(bids.at(200).getTotalQuantity() == 30);
    }

    SECTION("buy and sell orders go to separate sides") {
        Order buy(1, Side::BUY, 100, 50);
        Order sell(2, Side::SELL, 200, 30);
        book.addOrder(buy);
        book.addOrder(sell);

        REQUIRE(book.getBids().size() == 1);
        REQUIRE(book.getAsks().size() == 1);
    }
}

TEST_CASE("OrderBook cancelOrder", "[orderbook]") {
    OrderBook book;

    SECTION("cancels a buy order") {
        Order order(1, Side::BUY, 100, 50);
        book.addOrder(order);
        book.cancelOrder(1);

        CHECK(book.getBids().at(100).getTotalQuantity() == 0);
    }

    SECTION("cancels a sell order") {
        Order order(1, Side::SELL, 200, 30);
        book.addOrder(order);
        book.cancelOrder(1);

        CHECK(book.getAsks().at(200).getTotalQuantity() == 0);
    }

    SECTION("cancels one order, other remains") {
        Order order1(1, Side::BUY, 100, 50);
        Order order2(2, Side::BUY, 100, 30);
        book.addOrder(order1);
        book.addOrder(order2);

        book.cancelOrder(1);

        CHECK(book.getBids().at(100).getTotalQuantity() == 30);
        CHECK(book.getBids().at(100).getOrders().size() == 1);
    }

    SECTION("cancel nonexistent order does nothing") {
        Order order(1, Side::BUY, 100, 50);
        book.addOrder(order);

        book.cancelOrder(999);

        CHECK(book.getBids().at(100).getTotalQuantity() == 50);
    }

    SECTION("cancel same order twice does nothing on second call") {
        Order order(1, Side::BUY, 100, 50);
        book.addOrder(order);

        book.cancelOrder(1);
        book.cancelOrder(1);

        CHECK(book.getBids().at(100).getTotalQuantity() == 0);
    }
}
