#include <catch2/catch_test_macros.hpp>
#include "price_level.hpp"
#include "order.hpp"
#include "types.hpp"

TEST_CASE("PriceLevel - addOrder inserts order", "[pricelevel][add]") {
    PriceLevel level(100);
    Order order(1, Side::BUY, 100, 50);
    level.addOrder(order);

    auto orders = level.getOrders();
    REQUIRE(orders.size() == 1);

    const Order& first = orders.front();
    CHECK(first.getId() == 1);
    CHECK(first.getSide() == Side::BUY);
    CHECK(first.getPrice() == 100);
    CHECK(first.getQuantity() == 50);
}

TEST_CASE("PriceLevel - addOrder multiple orders aggregates quantity", "[pricelevel][add]") {
    PriceLevel level(100);
    level.addOrder(Order(1, Side::BUY, 100, 50));
    level.addOrder(Order(2, Side::BUY, 100, 30));

    CHECK(level.getOrders().size() == 2);
    CHECK(level.getTotalQuantity() == 80);
}

TEST_CASE("PriceLevel - removeOrderById removes order", "[pricelevel][remove]") {
    PriceLevel level(100);
    level.addOrder(Order(1, Side::BUY, 100, 50));
    level.removeOrderById(1);

    CHECK(level.getOrders().empty());
}

TEST_CASE("PriceLevel - removeOrderById decreases total quantity", "[pricelevel][remove]") {
    PriceLevel level(100);
    level.addOrder(Order(1, Side::BUY, 100, 50));
    level.addOrder(Order(2, Side::BUY, 100, 30));
    level.removeOrderById(1);

    CHECK(level.getTotalQuantity() == 30);
    CHECK(level.getOrders().size() == 1);
}
