// Holds all orders at that level
#pragma once
#include "order.hpp"
#include "types.hpp"
class PriceLevel {
  friend class PriceLevelPool;

public:
  PriceLevel() = default;
  explicit PriceLevel(Price price) : head_(nullptr), tail_(nullptr) {}

  void addOrder(Order* order) noexcept;
  void removeOrder(Order* order) noexcept;
  void reduceFrontQuantity(Quantity qty) noexcept;
  void modifyOrderQuantity(Quantity qty) noexcept { totalQuantity_ += qty; }
  Quantity getTotalQuantity() const noexcept { return totalQuantity_; }
  size_t orderCount() const noexcept { return orderCount_; }
  const Order* getOrders() const noexcept { return head_; }
  Order* popFront() noexcept;
  Order& front() noexcept;
  const Order& front() const noexcept;

private:
  // Use both a head and a tail for O(1) insertion
  Order* head_ = nullptr;
  Order* tail_ = nullptr;
  PriceLevel* next_ = nullptr;

  Quantity totalQuantity_{};
  size_t orderCount_ = 0;
};
