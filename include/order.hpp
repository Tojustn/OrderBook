#pragma once
#include "types.hpp"
class Order{
    public:
        Order(OrderId id, Side side, Price price, Quantity quantity, UserId userId)
            : id_(id), side_(side), price_(price), quantity_(quantity), userId_(userId) {}

        OrderId getId() const noexcept { return id_; }
        Side getSide() const noexcept { return side_; }
        Price getPrice() const noexcept { return price_; }
        Quantity getQuantity() const noexcept { return quantity_; }
        UserId getUserId() const noexcept {return userId_;}
        
        void setQuantity(const Quantity quantity) noexcept { quantity_ = quantity;}
    private:
        Price price_; // in PPU (price per unit)
        Side side_;
        OrderId id_;
        Quantity quantity_;
        UserId userId_;
        
};