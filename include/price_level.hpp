//Holds all orders at that level
#pragma once
#include "types.hpp"
#include "order.hpp"
#include <unordered_map>
class PriceLevel{
    public:
        PriceLevel() = default;
        explicit PriceLevel(Price price): head_(nullptr), tail_(nullptr){};

        void addOrder(Order* order);
        void removeOrderById(OrderId id);
        void reduceFrontQuantity(Quantity qty) noexcept;
        void modifyOrderQuantity(Quantity qty) noexcept {totalQuantity_ += qty;};
        inline Quantity getTotalQuantity() const noexcept { return totalQuantity_; }
        const Order* getOrders() const noexcept { return head_; }
        Order* popFront();
        Order& front();
        const Order& front() const;


        PriceLevel* next_ = nullptr;

    private:
        // Use both a head and a tail for O(1) insertion
        Order* head_;
        Order* tail_;
        
        // Only used for pooling

        // map for O(1) deletion
        std::unordered_map<OrderId, Order*> orderMap_;
     
        void removeOrder(Order* order);

        Quantity totalQuantity_{};

};
