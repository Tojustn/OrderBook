#include "order_book.hpp"
#include "price_level.hpp"
#include "types.hpp"
#include <list>
#include <map>
// Not a pointer as a param since caller doesnt manage memory
void OrderBook::fillOrder(PriceLevel& level) {
    Order* filled = level.popFront();
    orderMap_.erase(filled->getId());
    pool_.deallocate(filled);
}

OrderBook::~OrderBook() {
    for (auto& [id, order] : orderMap_) {
        pool_.deallocate(order);
    }
}

AddResult OrderBook::addOrder(const Order& order){
    const Price order_price = order.getPrice();
    const Side order_side = order.getSide();
    const OrderId order_id = order.getId();
    const MatchResult result = matchOrder(order);
    if(result.stpTriggered){
        return AddResult::STP_CANCELLED; 
    }
    else if(result.remaining == 0){
        return AddResult::FILLED;
    }
    Order* slot = pool_.allocate(order);
    slot->setQuantity(result.remaining);
    orderMap_[order_id] = slot;
    if(order_side == Side::BUY){
        bids_.try_emplace(order_price, order_price);
        bids_.at(order_price).addOrder(slot);
    }
    else{
        asks_.try_emplace(order_price, order_price);
        asks_.at(order_price).addOrder(slot);
    }
    return AddResult::ADDED;
}
void OrderBook::cancelOrder(const OrderId orderId){
    auto it = orderMap_.find(orderId);
    if (it == orderMap_.end()) return;
    Order* order = it->second;

    if(order->getSide() == Side::BUY){
        bids_.at(order->getPrice()).removeOrderById(orderId);
    }
    else{
        asks_.at(order->getPrice()).removeOrderById(orderId);
    }

    orderMap_.erase(orderId);
    pool_.deallocate(order);
}

MatchResult OrderBook::matchOrder(const Order& order){
    const Price price = order.getPrice();
    const Side side = order.getSide();
    Quantity remainingQuantity = order.getQuantity();
    const UserId orderUserId = order.getUserId();
    if(side == Side::BUY){
        // MBP + FIFO
        // If a buy order comes in we want to match, the lowest possible sell and then check the quantity
        // Then keep going if the buy matches the sell and check quantity because of the list it is FIFO

        auto it = asks_.begin();
        while(it != asks_.end() && it->first <= price && remainingQuantity > 0){
            PriceLevel& level = it->second;
            while(level.getTotalQuantity() > 0 && remainingQuantity > 0){
                const Order lowest_ask = it->second.front();
                if (lowest_ask.getUserId() == orderUserId){
                    return MatchResult{0, true};
                }
                const Quantity ask_quantity = lowest_ask.getQuantity();

                // If the ask is not enough quantity
                if(ask_quantity <= remainingQuantity){
                    fillOrder(it->second);
                    remainingQuantity -= ask_quantity;
                }
                else{
                    it->second.reduceFrontQuantity(remainingQuantity);
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
                const Quantity bid_quantity = highest_bid.getQuantity();

                if(bid_quantity <= remainingQuantity){
                    fillOrder(it->second);
                    remainingQuantity -= bid_quantity;
                }
                else{
                    it->second.reduceFrontQuantity(remainingQuantity);
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

