#include <catch2/catch_test_macros.hpp>
#include "price_level.hpp"
#include "order.hpp"
#include "types.hpp"

TEST_CASE("PriceLevel operations", "[pricelevel]") {
    Price levelPrice = 100;
    PriceLevel level(levelPrice);
    
    OrderId orderId = 1;
    Side side = Side::BUY;
    Price price = 100;
    Quantity quantity = 50;
    Order order(orderId, side, price, quantity);
    
    SECTION("addOrder adds order to level") {
        level.addOrder(order);
        
        auto orders = level.getOrders();
        REQUIRE(orders.size() == 1);
        
        const Order& firstOrder = orders.front();
        CHECK(firstOrder.getId() == orderId);
        CHECK(firstOrder.getSide() == side);
        CHECK(firstOrder.getPrice() == price);
        CHECK(firstOrder.getQuantity() == quantity);
    }
    
    SECTION("removeOrder removes order from level") {
        level.addOrder(order);
        REQUIRE(level.getOrders().size() == 1);
        
        level.removeOrderById(orderId);
        
        auto orders = level.getOrders();
        REQUIRE(orders.empty());
    }
    
    SECTION("addOrder with multiple orders") {
        Order order2(2, side, price, 30);
        
        level.addOrder(order);
        level.addOrder(order2);
        
        auto orders = level.getOrders();
        REQUIRE(orders.size() == 2);
        CHECK(level.getTotalQuantity() == 80);
    }
    
    SECTION("removeOrder decreases total volume") {
        Order order2(2, side, price, 30);
        level.addOrder(order);
        level.addOrder(order2);
        
        level.removeOrderById(orderId);
        
        REQUIRE(level.getTotalQuantity() == 30);
        REQUIRE(level.getOrders().size() == 1);
    }
}