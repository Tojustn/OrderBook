#pragma once
#include "types.hpp"
class Order{
    public:
        Order(OrderId id, Side side, Price price, Quantity quantity)
            : id_(id), side_(side), price_(price), quantity_(quantity) {}

        OrderId getId() const noexcept { return id_; }
        Side getSide() const noexcept { return side_; }
        Price getPrice() const noexcept { return price_; }
        Quantity getQuantity() const noexcept { return quantity_; }
    private:
        Price price_; // in PPU (price per unit)
        Side side_;
        OrderId id_;
        Quantity quantity_;
        
};