#pragma once
#include "order.hpp"

class OrderPool {
public:
    OrderPool() = default;
    explicit OrderPool(size_t capacity);
    Order* allocate(const Order& order);
    void deallocate(Order* order);
    ~OrderPool();

private:
    Order* head_ = nullptr;
};
