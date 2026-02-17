//Holds all orders at that level
#pragma once
#include "types.hpp"
#include <list>
#include "order.hpp"
#include <unordered_map>
class PriceLevel{
    public:
        explicit PriceLevel(Price price): price_(price){};

        void addOrder(const Order& order);
        void removeOrder(std::list<Order>::iterator it);
        void removeOrderById(OrderId id);
        Quantity getTotalQuantity();
        std::list<Order> getOrders() const noexcept { return orders_; }

    private:
        Price price_;
        // Using a linked list for FIFO priority
        std::list<Order> orders_;

        // Use unordered map for O(1) lookup of list iterators
        std::unordered_map<OrderId, std::list<Order>::iterator> orderMap_;    
     

};
