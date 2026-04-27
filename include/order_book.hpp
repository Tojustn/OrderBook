#pragma once
#include "types.hpp"
#include "order.hpp"
#include "order_pool.hpp"
#include "price_level.hpp"
#include <unordered_map>
#include <map>

class OrderBook{
    public:
        OrderBook() =default;
        ~OrderBook();
        AddResult addOrder(const Order& order);
        CancelResult cancelOrder(const OrderId orderId);
        ModifyResult modifyOrder(const OrderId orderId, const Quantity newQuantity);
        const std::map<Price, PriceLevel>& getBids() const noexcept { return bids_; }
        const std::map<Price, PriceLevel>& getAsks() const noexcept { return asks_; }
    private:
        MatchResult matchOrder(const Order& order);
        void fillOrder(PriceLevel& level);
        std::map<Price, PriceLevel> bids_;
        std::map<Price, PriceLevel> asks_;
        std::unordered_map<OrderId, Order*> orderMap_;
        OrderPool pool_;

};