//Holds all orders at that level
#pragma once
#include "types.hpp"
#include <list>
#include "order.hpp"

class PriceLevel{
    public:
        explicit PriceLevel(Price price): price_(price){};

        void addOrder(const Order& order);
        void removeOrder(std::list<Order>::iterator it);

    private:
        Price price_;
        // Using a linked list for FIFO priority
        std::list<Order> orders_;
     

};
