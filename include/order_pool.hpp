#pragma once
#include "order.hpp"

class OrderPool {
public:
  OrderPool() = default;
  explicit OrderPool(size_t capacity);
  Order* allocate(const Order& order);
  void deallocate(Order* order);

  ~OrderPool();
  OrderPool(const OrderPool&) = delete;
  OrderPool& operator=(const OrderPool&) = delete;
  OrderPool(OrderPool&&) noexcept;
  OrderPool& operator=(OrderPool&&) noexcept;

private:
  Order* head_ = nullptr;
};
