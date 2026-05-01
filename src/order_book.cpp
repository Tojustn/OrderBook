#include "order_book.hpp"
#include "price_level.hpp"
#include "price_level_pool.hpp"
#include "types.hpp"
#include <type_traits>
// Not a pointer as a param since caller doesnt manage memory
void OrderBook::fillOrder(PriceLevel& level) {
  Order* filled = level.popFront();
  orderMap_.erase(filled->getId());
  order_pool_.deallocate(filled);
}

OrderBook::~OrderBook() {
  for (auto& [id, order] : orderMap_) {
    order_pool_.deallocate(order);
  }
  for (auto& [price, level] : bids_) {
    price_level_pool_.deallocate(level);
  }
  for (auto& [price, level] : asks_) {
    price_level_pool_.deallocate(level);
  }
}

AddResult OrderBook::addOrder(OrderType orderType, const Order& order) {
  const Price order_price = order.getPrice();
  const Side order_side = order.getSide();
  const OrderId order_id = order.getId();

  switch (orderType) {
    case (OrderType::GOOD_TILL_CANCEL): {
      const MatchResult result = matchOrder(order);
      if (result.stpTriggered)
        return AddResult::STP_CANCELLED;
      if (result.remaining == 0)
        return AddResult::FILLED;
      Order* slot = order_pool_.allocate(order);
      slot->setQuantity(result.remaining);
      orderMap_[order_id] = slot;
      auto& map = (order_side == Side::BUY) ? bids_ : asks_;
      auto [it, inserted] = map.try_emplace(order_price, nullptr);
      if (inserted)
        it->second = price_level_pool_.allocate();
      it->second->addOrder(slot);
      slot->level_ = it->second;
      break;
    }
    case (OrderType::FILL_AND_KILL): {
      const MatchResult result = matchOrder(order);
      if (result.stpTriggered)
        return AddResult::STP_CANCELLED;
      return result.remaining == 0 ? AddResult::FILLED : AddResult::KILLED;
    }
    case (OrderType::MARKET_ORDER): {
      matchOrder(order);
      return AddResult::FILLED;
    }
    case (OrderType::FILL_OR_KILL): {
      if (!canFill(order))
        return AddResult::KILLED;
      matchOrder(order);
      return AddResult::FILLED;
    }
    case (OrderType::POST_ONLY): {
      if (couldMatch(order))
        return AddResult::KILLED;
      Order* slot = order_pool_.allocate(order);
      orderMap_[order_id] = slot;
      auto& map = (order_side == Side::BUY) ? bids_ : asks_;
      auto [it, inserted] = map.try_emplace(order_price, nullptr);
      if (inserted)
        it->second = price_level_pool_.allocate();
      it->second->addOrder(slot);
      slot->level_ = it->second;
      break;
    }
    default:
      return AddResult::FILLED;
  }

  bestBid_ = bids_.empty() ? nullptr : bids_.rbegin()->second;
  bestAsk_ = asks_.empty() ? nullptr : asks_.begin()->second;
  return AddResult::ADDED;
}
// For Market Order Type
AddResult OrderBook::addOrder(const OrderId orderId, const Side side, const Quantity quantity, const UserId userId) {
  const Price price = static_cast<Price>(side == Side::BUY ? MarketPrice::BUY : MarketPrice::SELL);
  Order order{orderId, side, price, quantity, userId};
  return addOrder(OrderType::MARKET_ORDER, order);
}

// For client initiated cancels
CancelResult OrderBook::cancelOrder(const OrderId orderId) {
  auto it = orderMap_.find(orderId);
  if (it == orderMap_.end())
    return CancelResult::NOT_FOUND;
  Order* order = it->second;

  PriceLevel* level = order->level_;

  level->removeOrder(order);
  if (level->getTotalQuantity() == 0) {
    if (order->getSide() == Side::BUY) {
      bids_.erase(order->getPrice());
      bestBid_ = (bids_.empty() ? nullptr : bids_.rbegin()->second);
    }
    else {
      asks_.erase(order->getPrice());
      bestAsk_ = (asks_.empty() ? nullptr : asks_.begin()->second);
    }
    price_level_pool_.deallocate(level);
  }

  orderMap_.erase(orderId);
  order_pool_.deallocate(order);
  return CancelResult::CANCELLED;
}

MatchResult OrderBook::matchOrder(const Order& order) {
  const Price price = order.getPrice();
  const Side side = order.getSide();
  Quantity remainingQuantity = order.getQuantity();
  const UserId orderUserId = order.getUserId();
  if (side == Side::BUY) {
    auto it = asks_.begin();
    while (it != asks_.end() && it->first <= price && remainingQuantity > 0) {
      PriceLevel* level = it->second;
      while (level->getTotalQuantity() > 0 && remainingQuantity > 0) {
        const Order lowest_ask = it->second->front();
        if (lowest_ask.getUserId() == orderUserId) {
          return MatchResult{remainingQuantity, true};
        }
        const Quantity ask_quantity = lowest_ask.getQuantity();

        if (ask_quantity <= remainingQuantity) {
          fillOrder(*it->second);
          remainingQuantity -= ask_quantity;
        }
        else {
          it->second->reduceFrontQuantity(remainingQuantity);
          remainingQuantity = 0;
        }
      }
      if (level->getTotalQuantity() == 0) {
        it = asks_.erase(it);
        price_level_pool_.deallocate(level);
      }
      else
        ++it;
    }

    bestAsk_ = (asks_.empty() ? nullptr : asks_.begin()->second);
  }
  else {
    auto it = bids_.rbegin();
    while (it != bids_.rend() && it->first >= price && remainingQuantity > 0) {
      PriceLevel* level = it->second;
      while (level->getTotalQuantity() > 0 && remainingQuantity > 0) {
        Order& highest_bid = it->second->front();
        if (highest_bid.getUserId() == orderUserId) {
          return MatchResult{remainingQuantity, true};
        }
        const Quantity bid_quantity = highest_bid.getQuantity();

        if (bid_quantity <= remainingQuantity) {
          fillOrder(*it->second);
          remainingQuantity -= bid_quantity;
        }
        else {
          it->second->reduceFrontQuantity(remainingQuantity);
          remainingQuantity = 0;
        }
      }
      if (level->getTotalQuantity() == 0) {
        it = std::reverse_iterator(bids_.erase(std::next(it).base()));
        price_level_pool_.deallocate(level);
      }
      else
        ++it;
    }
    bestBid_ = (bids_.empty() ? nullptr : bids_.rbegin()->second);
  }

  return MatchResult{remainingQuantity, false};
}

ModifyResult OrderBook::modifyOrder(const OrderId orderId, const Quantity newQuantity) {
  if (newQuantity < 0) {
    return ModifyResult::INVALID_QUANTITY;
  }
  else if (newQuantity == 0) {
    const CancelResult result = cancelOrder(orderId);
    return result == CancelResult::NOT_FOUND ? ModifyResult::ORDER_NOT_FOUND : ModifyResult::SUCCESS;
  }

  auto it = orderMap_.find(orderId);
  if (it == orderMap_.end()) {
    return ModifyResult::ORDER_NOT_FOUND;
  }
  else {
    const Quantity currentQuantity = it->second->getQuantity();
    it->second->setQuantity(newQuantity);
    it->second->level_->modifyOrderQuantity(newQuantity - currentQuantity);
    return ModifyResult::SUCCESS;
  }
}

bool OrderBook::canFill(const Order& order) const noexcept {
  const Price price = order.getPrice();
  const Side side = order.getSide();
  const UserId userId = order.getUserId();
  Quantity needed = order.getQuantity();

  if (side == Side::BUY) {
    for (auto it = asks_.begin(); it != asks_.end() && it->first <= price && needed > 0; ++it) {
      for (const Order* o = it->second->getOrders(); o != nullptr && needed > 0; o = o->next_) {
        if (o->getUserId() == userId)
          return false;
        needed -= o->getQuantity();
      }
    }
  }
  else {
    for (auto it = bids_.rbegin(); it != bids_.rend() && it->first >= price && needed > 0; ++it) {
      for (const Order* o = it->second->getOrders(); o != nullptr && needed > 0; o = o->next_) {
        if (o->getUserId() == userId)
          return false;
        needed -= o->getQuantity();
      }
    }
  }

  return needed <= 0;
}

bool OrderBook::couldMatch(const Order& order) const noexcept {
  if (order.getSide() == Side::BUY) {
    return !asks_.empty() && order.getPrice() >= asks_.begin()->first;
  }
  else {
    return !bids_.empty() && order.getPrice() <= bids_.rbegin()->first;
  }
}
