// Holds all orders at that level
#pragma once
#include "order.hpp"
#include "types.hpp"
class PriceLevel {
public:
  PriceLevel() = default;
  explicit PriceLevel(Price price) : head_(nullptr), tail_(nullptr) {};

  void addOrder(Order* order);
  void removeOrder(Order* order);
  void reduceFrontQuantity(Quantity qty) noexcept;
  void modifyOrderQuantity(Quantity qty) noexcept { totalQuantity_ += qty; };
  Quantity getTotalQuantity() const noexcept { return totalQuantity_; }
  const Order* getOrders() const noexcept { return head_; }
  Order* popFront();
  Order& front();
  const Order& front() const;

  PriceLevel* next_ = nullptr;

private:
  // Use both a head and a tail for O(1) insertion
  Order* head_;
  Order* tail_;

  Quantity totalQuantity_{};
};
