#include "order_book.hpp"
#include "price_level.hpp"
#include "types.hpp"
#include <list>
#include <map>
void OrderBook::addOrder(const Order& order){
    Price order_price = order.getPrice();
    Side order_side = order.getSide();
    OrderId order_id = order.getId();
    if(order_side == Side::BUY){
        Quantity remaning_quantity = matchOrder(order);
        if(remaning_quantity > 0){
            // Contruct an order isntead of changing the original because caller woudlnt expect order to be mutated
            bids_.try_emplace(order_price, order_price);
            Order remaining_order(order);
            remaining_order.setQuantity(remaning_quantity);
            bids_.at(order_price).addOrder(remaining_order);
            orderMap_[order_id] = {order_price, order_side};
        }
    }
    else{
        Quantity remaining_quantity = matchOrder(order);
        if(remaining_quantity > 0){
            asks_.try_emplace(order_price, order_price);
            Order remaining_order(order);
            remaining_order.setQuantity(remaining_quantity);
            asks_.at(order_price).addOrder(remaining_order);
            orderMap_[order_id] = {order_price, order_side};
        }
    }
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

Quantity OrderBook::matchOrder(const Order& order){
    const Price price = order.getPrice();
    const Side side = order.getSide();
    Quantity remainingQuantity = order.getQuantity();
    if(side == Side::BUY){
        // MBP + FIFO
        // If a buy order comes in we want to match, the lowest possible sell and then check the quantity
        // Then keep going if the buy matches the sell and check quantity because of the list it is FIFO

        auto it = asks_.begin();
        while(it != asks_.end() && it->first <= price && remainingQuantity > 0){
            PriceLevel& level = it->second;
            while(level.getTotalQuantity() > 0 && remainingQuantity > 0){
                Order lowest_ask = it->second.front();
                Quantity ask_quantity = lowest_ask.getQuantity();

                // If the ask is not enough quantity
                if(ask_quantity < remainingQuantity){
                    it->second.popFront();
                    remainingQuantity = remainingQuantity - ask_quantity;   
                }
                else{
                    // If the remaining Quantity is fulfilled with the ask
                    Order& ask_order = it->second.front();
                    ask_order.setQuantity(ask_order.getQuantity() - remainingQuantity);
                    remainingQuantity = 0;

                }
            }
            if(level.getTotalQuantity() == 0)
                it = asks_.erase(it);
            else
                ++it;

        }
        
    }
    else{
        auto it = bids_.rbegin();
        while(it != bids_.rend() && it->first >= price && remainingQuantity > 0){
            PriceLevel& level = it->second;
            while(level.getTotalQuantity() > 0 && remainingQuantity > 0){
                Order& highest_bid = it->second.front();
                Quantity bid_quantity = highest_bid.getQuantity();

                if(bid_quantity < remainingQuantity){
                    it->second.popFront();
                    remainingQuantity = remainingQuantity - bid_quantity;
                }
                else{
                    Order& bid_order = it->second.front();
                    bid_order.setQuantity(bid_order.getQuantity() - remainingQuantity);
                    remainingQuantity = 0;
                }
            }
            if(level.getTotalQuantity() == 0)
                it = std::reverse_iterator(bids_.erase(std::next(it).base()));
            else
                ++it;
        }
    }

    return remainingQuantity;
}