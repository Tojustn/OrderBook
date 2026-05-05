#include "order_pool.hpp"
#include "order.hpp"
#include "types.hpp"

// Allocate a Order from the Pool to a Order from the OrderBook
OrderPool::OrderPool(size_t capacity) {
  for (size_t i = 0; i < capacity; i++) {
    Order* o = new Order();
    o->next_ = head_;
    head_ = o;
  }
}

Order* OrderPool::allocate(const Order& order) {
  Order* slot = nullptr;
  if (head_) {
    slot = head_;
    head_ = head_->next_;
  }
  else {
    slot = new Order();
  }
  *slot = order;
  slot->next_ = nullptr;
  slot->prev_ = nullptr;
  return slot;
}

// Add a order to the pool
void OrderPool::deallocate(Order* order) noexcept {
  order->next_ = head_;
  head_ = order;
}

OrderPool::~OrderPool() {
  while (head_) {
    Order* next = head_->next_;
    delete head_;
    head_ = next;
  }
}

OrderPool::OrderPool(OrderPool&& orderPool) noexcept {
  this->head_ = (orderPool.head_);
  orderPool.head_ = nullptr;
}

OrderPool& OrderPool::operator=(OrderPool&& orderPool) noexcept {
  if (this == &orderPool) {
    return *this;
  }
  while (head_) {
    Order* next = head_->next_;
    delete head_;
    head_ = next;
  }
  head_ = orderPool.head_;
  orderPool.head_ = nullptr;
  return *this;
}
