#pragma once
#include "types.hpp"

class PriceLevel;

class Order {
  friend class PriceLevel;
  friend class OrderPool;
  friend class OrderBook;

public:
  Order(OrderId id, Side side, Price price, Quantity quantity, UserId userId)
      : price_(price), id_(id), quantity_(quantity), userId_(userId), side_(side) {}

  Order() = default;

  OrderId getId() const noexcept { return id_; }
  Side getSide() const noexcept { return side_; }
  Price getPrice() const noexcept { return price_; }
  Quantity getQuantity() const noexcept { return quantity_; }
  UserId getUserId() const noexcept { return userId_; }

  void setQuantity(const Quantity quantity) noexcept { quantity_ = quantity; }

private:
  Order* next_ = nullptr;
  Order* prev_ = nullptr;
  PriceLevel* level_ = nullptr;
  Price price_;
  OrderId id_;
  Quantity quantity_;
  UserId userId_;
  Side side_;
};
