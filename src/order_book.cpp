#include "order_book.hpp"
#include "price_level.hpp"
#include "types.hpp"
#include <map>
void OrderBook::addOrder(const Order& order){
    Price order_price = order.getPrice();
    Side order_side = order.getSide();
    OrderId order_id = order.getId();
    if(order_side == Side::BUY){
        bids_.try_emplace(order_price, order_price); // Implictly build a PriceLevel as the -> second
        bids_.at(order_price).addOrder(order);
    }
    else{
        asks_.try_emplace(order_price, order_price); // Implictly build a PriceLevel as the -> second
        asks_.at(order_price).addOrder(order);
    }
    orderMap_[order_id] = {order_price, order_side};
}
void OrderBook::cancelOrder(const OrderId orderId){
    auto it = orderMap_.find(orderId);
    if (it == orderMap_.end()) return;
    auto& [price, side] = it->second;

    if(side == Side::BUY){
        bids_.at(price).removeOrderById(orderId);
    }
    else{
        asks_.at(price).removeOrderById(orderId);
    }

    orderMap_.erase(orderId);
}