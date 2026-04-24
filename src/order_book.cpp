#include "order_book.hpp"
#include "price_level.hpp"
#include "types.hpp"
#include <algorithm>

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

// O(logn) Returns iterator to the PriceLevel from the std::vector::iterator
std::vector<PriceLevel>::iterator OrderBook::findPriceLevel(const Price price, const Side side){
    if(side == Side::BUY){
        return std::lower_bound(bids_.begin(), bids_.end(), price, 
        [](const PriceLevel& level, const Price price){return level.getPrice() > price;});
    }
    else{
        return std::lower_bound(asks_.begin(), asks_.end(), price, 
        [](const PriceLevel& level, const Price price){return level.getPrice() < price;});
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
    // Check if PriceLevel exists
    auto it = findPriceLevel(order_price, order_side);
    if(order_side == Side::BUY){
        // If the lower bound PriceLevel is not equal to the wanted pricelevel or iterator doesnt exists
        if(it == bids_.end() || it->getPrice() != order_price)
            it = bids_.insert(it, PriceLevel(order_price));
    }
    else{
        if(it == asks_.end() || it->getPrice() != order_price)
            it = asks_.insert(it, PriceLevel(order_price));
    }
    it->addOrder(slot);
    slot->level_ = &(*it);
    return AddResult::ADDED;
}

// For client initiated cancels
CancelResult OrderBook::cancelOrder(const OrderId orderId){
    auto it = orderMap_.find(orderId);
    if (it == orderMap_.end()) return CancelResult::NOT_FOUND;
    Order* order = it->second;

    PriceLevel* level = order->level_;
    level->removeOrderById(order->getId());
    if(level->getTotalQuantity() == 0){
        if(order->getSide() == Side::BUY){
            auto level_it = bids_.begin() + (level - bids_.data());
            bids_.erase(level_it);
        }
        else{
            auto level_it = asks_.begin() + (level - asks_.data());
            asks_.erase(level_it);
        }
    }

    orderMap_.erase(orderId);
    pool_.deallocate(order);
    return CancelResult::CANCELLED;
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
        while(it != asks_.end() && (*it).getPrice() <= price && remainingQuantity > 0){
            PriceLevel& level = *it;
            while(level.getTotalQuantity() > 0 && remainingQuantity > 0){
                const Order lowest_ask = level.front();
                if (lowest_ask.getUserId() == orderUserId){
                    return MatchResult{0, true};
                }
                const Quantity ask_quantity = lowest_ask.getQuantity();

                // If the ask is not enough quantity
                if(ask_quantity <= remainingQuantity){
                    fillOrder(level);
                    remainingQuantity -= ask_quantity;
                }
                else{
                    level.reduceFrontQuantity(remainingQuantity);
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
        auto it = bids_.begin();
        while(it != bids_.end() && (*it).getPrice() >= price && remainingQuantity > 0){
            PriceLevel& level = *it;
            while(level.getTotalQuantity() > 0 && remainingQuantity > 0){
                Order& highest_bid = level.front();
                if(highest_bid.getUserId() == orderUserId){
                    return MatchResult{0, true};
                }
                const Quantity bid_quantity = highest_bid.getQuantity();

                if(bid_quantity <= remainingQuantity){
                    fillOrder(level);
                    remainingQuantity -= bid_quantity;
                }
                else{
                    level.reduceFrontQuantity(remainingQuantity);
                    remainingQuantity = 0;
                }
            }
            if(level.getTotalQuantity() == 0)
                it = bids_.erase(it);
            else
                ++it;
        }
    }

    return MatchResult{remainingQuantity, false};
}


ModifyResult OrderBook::modifyOrder(const OrderId orderId, const Quantity newQuantity){
    if(newQuantity < 0){
        return ModifyResult::INVALID_QUANTITY;
    }
    else if(newQuantity == 0){
        const CancelResult result = cancelOrder(orderId);
        return result == CancelResult::NOT_FOUND ? ModifyResult::ORDER_NOT_FOUND : ModifyResult::SUCCESS;
    }

    auto it = orderMap_.find(orderId);
    if(it == orderMap_.end()){
        return ModifyResult::ORDER_NOT_FOUND;
    }
    else{
        const Quantity currentQuantity = it->second->getQuantity();
        it->second->setQuantity(newQuantity);
        it->second->level_->modifyOrderQuantity(newQuantity - currentQuantity);
        return ModifyResult::SUCCESS;
        
    }


}