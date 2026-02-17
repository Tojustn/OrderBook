#include "price_level.hpp"

void PriceLevel::addOrder(const Order& order){
    orders_.push_back(order);
    orderMap_[order.getId()] = std::prev(orders_.end());
}

void PriceLevel::removeOrder(std::list<Order>::iterator it){
    orderMap_.erase(it->getId());
    orders_.erase(it);
}

void PriceLevel::removeOrderById(OrderId orderId){
    auto mapIt = orderMap_.find(orderId);
    if (mapIt != orderMap_.end()) {
        removeOrder(mapIt->second);
    }
}

Quantity PriceLevel::getTotalQuantity(){
    Quantity total_Quantity{0};

    for(const Order& order: orders_){
        total_Quantity += order.getQuantity();
    }

    return total_Quantity;

}