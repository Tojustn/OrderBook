#pragma once
#include <atomic>
#include <cstdlib>
#include <new>

// Class for a SPSC Lock Free Queue
// Circular Queue, doesnt wrap just uses %
template <typename T>
class SPSC_QUEUE {
public:
  static_assert(std::atomic<size_t>::is_always_lock_free);
  SPSC_QUEUE(size_t capacity);
  bool push(const T&);
  // Take in a param, to edit since pop always follows the ring
  bool pop(T&);

private:
  T* ring_;
  // Atomics for multi thread safety, indexs
  std::atomic<size_t> push_cursor_;
  std::atomic<size_t> pop_cursor_;
  size_t capacity_;
};

template <typename T>
SPSC_QUEUE<T>::SPSC_QUEUE(size_t capacity) : capacity_(capacity) {
  // Just allocate space since objects are placement newed from the methods
  ring_ = static_cast<T*>(malloc(sizeof(T) * capacity_));
  if (!ring_)
    throw std::bad_alloc();
  push_cursor_ = 0;
  pop_cursor_ = 0;
};

template <typename T>
bool SPSC_QUEUE<T>::push(const T& val) {
  // Acquire both resources via atomic ops
  const size_t push_cursor_index = push_cursor_.load();
  const size_t pop_cursor_index = pop_cursor_.load();

  // If capacity is filled
  if (push_cursor_index == pop_cursor_index + capacity_)
    return false;

  // Placement new
  new (&ring_[push_cursor_index % capacity_]) T(val);

  push_cursor_.store(push_cursor_index + 1);
  return true;
};

// Fetch thje pop info through the value passed in
template <typename T>
bool SPSC_QUEUE<T>::pop(T& val) {
  const size_t push_cursor_index = push_cursor_.load();
  const size_t pop_cursor_index = pop_cursor_.load();
  if (pop_cursor_index == push_cursor_index)
    return false;

  val = ring_[pop_cursor_index % capacity_];

  pop_cursor_.store(pop_cursor_index + 1);
  return true;
};
