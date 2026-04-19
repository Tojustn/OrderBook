#include <catch2/catch_test_macros.hpp>
#include "price_level.hpp"
#include "order.hpp"
#include "types.hpp"

static int countOrders(const Order* head) {
    int n = 0;
    for (const Order* o = head; o; o = o->next_) ++n;
    return n;
}

TEST_CASE("PriceLevel - addOrder inserts order", "[pricelevel][add]") {
    PriceLevel level(100);
    Order order(1, Side::BUY, 100, 50, 1);
    level.addOrder(&order);

    const Order* head = level.getOrders();
    REQUIRE(countOrders(head) == 1);
    CHECK(head->getId() == 1);
    CHECK(head->getSide() == Side::BUY);
    CHECK(head->getPrice() == 100);
    CHECK(head->getQuantity() == 50);
}

TEST_CASE("PriceLevel - addOrder multiple orders aggregates quantity", "[pricelevel][add]") {
    PriceLevel level(100);
    Order o1(1, Side::BUY, 100, 50, 1);
    Order o2(2, Side::BUY, 100, 30, 2);
    level.addOrder(&o1);
    level.addOrder(&o2);

    CHECK(countOrders(level.getOrders()) == 2);
    CHECK(level.getTotalQuantity() == 80);
}

TEST_CASE("PriceLevel - removeOrderById removes order", "[pricelevel][remove]") {
    PriceLevel level(100);
    Order order(1, Side::BUY, 100, 50, 1);
    level.addOrder(&order);
    level.removeOrderById(1);

    CHECK(level.getOrders() == nullptr);
}

TEST_CASE("PriceLevel - removeOrderById decreases total quantity", "[pricelevel][remove]") {
    PriceLevel level(100);
    Order o1(1, Side::BUY, 100, 50, 1);
    Order o2(2, Side::BUY, 100, 30, 2);
    level.addOrder(&o1);
    level.addOrder(&o2);
    level.removeOrderById(1);

    CHECK(level.getTotalQuantity() == 30);
    CHECK(countOrders(level.getOrders()) == 1);
}
