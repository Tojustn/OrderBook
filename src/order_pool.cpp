#include "order_pool.hpp"
#include "order.hpp"
#include "types.hpp"

// Allocate a Order from the Pool to a Order from the OrderBook
Order* OrderPool::allocate(const Order& order) {
  Order* slot = nullptr;
  if (head_) {
    slot = head_;
    head_ = head_->next_;
  } else {
    slot = new Order();
  }
  *slot = order;
  slot->next_ = nullptr;
  slot->prev_ = nullptr;
  return slot;
}

// Add a order to the pool
void OrderPool::deallocate(Order* order) {
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
