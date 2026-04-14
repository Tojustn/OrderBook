#include "order_book.hpp"
#include "price_level.hpp"
#include "types.hpp"
#include <list>
#include <map>
AddResult OrderBook::addOrder(const Order& order){
    Price order_price = order.getPrice();
    Side order_side = order.getSide();
    OrderId order_id = order.getId();
    MatchResult result = matchOrder(order);
    if(result.stpTriggered){
        return AddResult::STP_CANCELLED; 
    }
    else if(result.remaining == 0){
        return AddResult::FILLED;
    }
    // Construct an order instead of changing the original because caller wouldn't expect order to be mutated
    Order remaining_order(order);
    remaining_order.setQuantity(result.remaining);
    if(order_side == Side::BUY){
        bids_.try_emplace(order_price, order_price);
        bids_.at(order_price).addOrder(remaining_order);
    }
    else{
        asks_.try_emplace(order_price, order_price);
        asks_.at(order_price).addOrder(remaining_order);
    }
    orderMap_[order_id] = {order_price, order_side};
    return AddResult::ADDED;
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

MatchResult OrderBook::matchOrder(const Order& order){
    const Price price = order.getPrice();
    const Side side = order.getSide();
    Quantity remainingQuantity = order.getQuantity();
    UserId orderUserId = order.getUserId();
    if(side == Side::BUY){
        // MBP + FIFO
        // If a buy order comes in we want to match, the lowest possible sell and then check the quantity
        // Then keep going if the buy matches the sell and check quantity because of the list it is FIFO

        auto it = asks_.begin();
        while(it != asks_.end() && it->first <= price && remainingQuantity > 0){
            PriceLevel& level = it->second;
            while(level.getTotalQuantity() > 0 && remainingQuantity > 0){
                Order lowest_ask = it->second.front();
                if (lowest_ask.getUserId() == orderUserId){
                    return MatchResult{0, true};
                }
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
                if(highest_bid.getUserId() == orderUserId){
                    return MatchResult{0, true};
                }
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

    return MatchResult{remainingQuantity, false};
}
