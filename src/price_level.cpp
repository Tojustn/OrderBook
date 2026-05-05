#include "price_level.hpp"
#include "types.hpp"

void PriceLevel::addOrder(Order* order) noexcept {
    totalQuantity_ += order->getQuantity();
    ++orderCount_;

    if (!tail_) {
        head_ = tail_ = order;
        order->prev_ = nullptr; 
        order->next_ = nullptr;
    } else {
        Order* oldTail = tail_;
        oldTail->next_ = order;
        order->prev_ = oldTail; 
        tail_ = order;         
    }
    order->next_ = nullptr; 
}

void PriceLevel::removeOrder(Order* order) noexcept {
    if (!order) return;

    if (order->prev_) {
        order->prev_->next_ = order->next_;
    } else {
        head_ = order->next_;
    }

    if (order->next_) {
        order->next_->prev_ = order->prev_;
    } else {
        tail_ = order->prev_;
    }

    totalQuantity_ -= order->getQuantity();
    --orderCount_;
    order->next_ = nullptr;
    order->prev_ = nullptr;
}
void PriceLevel::reduceFrontQuantity(Quantity qty) noexcept {
    head_->setQuantity(head_->getQuantity() - qty);
    totalQuantity_ -= qty;
}

Order& PriceLevel::front() noexcept {
    return *head_;
}

const Order& PriceLevel::front() const noexcept {
    return *head_;
}

Order* PriceLevel::popFront() noexcept {
    Order* order = head_;
    removeOrder(head_);
    return order;
}
