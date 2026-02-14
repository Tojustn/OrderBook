#include "include/price_level.hpp"
#include <list>

void PriceLevel::addOrder(const Order& order){
    orders_.push_back(order);
}

void PriceLevel::removeOrder(std::list<Order>::iterator it){
    orders_.erase(it);
}