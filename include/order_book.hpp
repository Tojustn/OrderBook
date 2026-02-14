#pragma once
#include "types.hpp"
#include "order.hpp"
#include "price_level.hpp"
#include <unordered_map>
#include <map>
#include <memory>

// OrderLocation holds the price, side and an iterator to the Orders position
// For easy removal we need both price and side and the iterator for O(1) removal w
struct OrderLocation {
    Price price_;
    Side side_;
    std::list<Order>::iterator it_;
};
class OrderBook{
    public:
        // Use default to start empty
        OrderBook() =default;
        void addOrder(const Order& order);
        void cancelOrder(const OrderId orderId);
    private:   
        std::map<Price, PriceLevel> bids_;
        std::map<Price, PriceLevel> asks_;
        std::unordered_map<OrderId, OrderLocation> orderMap_;
};