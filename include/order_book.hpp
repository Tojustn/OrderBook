#pragma once
#include "types.hpp"
#include "order.hpp"
#include "price_level.hpp"
#include <unordered_map>
#include <map>
#include <memory>

// OrderLocation holds the price and side for fast cancel lookups
struct OrderLocation {
    Price price_;
    Side side_;
};
class OrderBook{
    public:
        // Use default to start empty
        OrderBook() =default;
        void addOrder(const Order& order);
        void cancelOrder(const OrderId orderId);
        const std::map<Price, PriceLevel>& getBids() const noexcept { return bids_; }
        const std::map<Price, PriceLevel>& getAsks() const noexcept { return asks_; }
    private:   
        std::map<Price, PriceLevel> bids_;
        std::map<Price, PriceLevel> asks_;
        std::unordered_map<OrderId, OrderLocation> orderMap_;
};