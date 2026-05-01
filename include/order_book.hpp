#pragma once
#include "order.hpp"
#include "order_pool.hpp"
#include "price_level.hpp"
#include "price_level_pool.hpp"
#include "types.hpp"
#include <map>
#include <unordered_map>

class OrderBook {
public:
  OrderBook() = default;
  explicit OrderBook(size_t order_capacity, size_t level_capacity);
  ~OrderBook();
  AddResult addOrder(OrderType, const Order& order);
  // overload Market Order Type
  AddResult addOrder(const OrderId, const Side, const Quantity, const UserId);
  CancelResult cancelOrder(const OrderId orderId);
  ModifyResult modifyOrder(const OrderId orderId, const Quantity newQuantity);
  PriceLevel* getBestBid() const noexcept { return bestBid_; }
  PriceLevel* getBestAsk() const noexcept { return bestAsk_; }
  const std::map<Price, PriceLevel*>& getBids() const noexcept { return bids_; }
  const std::map<Price, PriceLevel*>& getAsks() const noexcept { return asks_; }

private:
  MatchResult matchOrder(const Order& order);
  bool canFill(const Order& order) const noexcept;
  bool couldMatch(const Order& order) const noexcept;
  void fillOrder(PriceLevel& level);
  std::map<Price, PriceLevel*> bids_;
  std::map<Price, PriceLevel*> asks_;
  std::unordered_map<OrderId, Order*> orderMap_;

  // Points to a pricelevel, O(1) still don't have to adjust on every addOrder
  PriceLevel* bestBid_ = nullptr;
  PriceLevel* bestAsk_ = nullptr;

  OrderPool order_pool_;
  PriceLevelPool price_level_pool_;
};
