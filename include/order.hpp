#pragma once
#include "types.hpp"
class Order{
    public:
        Order(OrderId id, Side side, Price price, Quantity quantity, UserId userId)
            : price_(price), id_(id), quantity_(quantity), userId_(userId), side_(side) {}

        OrderId getId() const noexcept { return id_; }
        Side getSide() const noexcept { return side_; }
        Price getPrice() const noexcept { return price_; }
        Quantity getQuantity() const noexcept { return quantity_; }
        UserId getUserId() const noexcept {return userId_;}

        Order* next_ = nullptr;
        Order* prev_ = nullptr;
        
        void setQuantity(const Quantity quantity) noexcept { quantity_ = quantity;}
    private:
        Price price_; // in PPU (price per unit)
        OrderId id_;
        Quantity quantity_;
        UserId userId_;
        Side side_;

        
};