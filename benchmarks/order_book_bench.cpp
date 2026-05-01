#include "order.hpp"
#include "order_book.hpp"
#include "types.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

static inline uint64_t rdtsc_start() {
  // lfence for cpu instruction reordering
  __builtin_ia32_lfence();
  unsigned int lo, hi;

  // "memory" to remove compiler reordering
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi)::"memory");
  return (static_cast<uint64_t>(hi) << 32) | lo;
}

static inline uint64_t rdtsc_end() {
  unsigned int lo, hi, aux;
  __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux)::"memory");
  __builtin_ia32_lfence();
  return (static_cast<uint64_t>(hi) << 32) | lo;
}

// Calibrate by measuring cycles over a 100ms wall-clock window
static double ns_per_cycle() {
  using clk = std::chrono::steady_clock;
  const auto wall0 = clk::now();
  const uint64_t cyc0 = rdtsc_start();
  while (std::chrono::duration<double>(clk::now() - wall0).count() < 0.1) {
  }
  const uint64_t cyc1 = rdtsc_end();
  const auto wall1 = clk::now();
  const double ns = std::chrono::duration<double, std::nano>(wall1 - wall0).count();
  return ns / static_cast<double>(cyc1 - cyc0);
}

static double pct(std::vector<uint64_t>& v, double p) {
  return static_cast<double>(v[static_cast<size_t>(p / 100.0 * (v.size() - 1))]);
}

static void report(const char* name, std::vector<uint64_t>& samples, double ns_per_cyc) {
  std::sort(samples.begin(), samples.end());
  std::printf("%-30s  p50=%6.1f ns  p99=%6.1f ns  p99.9=%6.1f ns\n", name, pct(samples, 50.0) * ns_per_cyc,
              pct(samples, 99.0) * ns_per_cyc, pct(samples, 99.9) * ns_per_cyc);
}

static constexpr int N_SAMPLES = 100'000;
static constexpr int PREPOP = 100;

int main() {
  const double ns = ns_per_cycle();
  std::vector<uint64_t> samples;
  samples.reserve(N_SAMPLES);

  // addOrder - on a warm book adding to an existing level (steady state stable depth)
  {
    samples.clear();
    OrderBook book(PREPOP + N_SAMPLES, PREPOP);
    for (int j = 0; j < PREPOP; j++)
      book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(j, Side::BUY, j + 1, 10, j + 1));

    for (int i = 0; i < N_SAMPLES; i++) {
      const Order o(PREPOP + i, Side::BUY, (i % PREPOP) + 1, 50, PREPOP + i);

      const uint64_t t0 = rdtsc_start();
      book.addOrder(OrderType::GOOD_TILL_CANCEL, o);
      const uint64_t t1 = rdtsc_end();
      samples.push_back(t1 - t0);
    }
    report("addOrder (no match)(steady state)", samples, ns);
  }

  // addOrder - on a warm book adding to an new level (differing depth)
  {
    samples.clear();
    OrderBook book(PREPOP + N_SAMPLES, PREPOP + N_SAMPLES);
    for (int j = 0; j < PREPOP; j++)
      book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(j, Side::BUY, j + 1, 10, j + 1));

    for (int i = 0; i < N_SAMPLES; i++) {
      const Order o(PREPOP + i, Side::BUY, (i + PREPOP + 1), 50, PREPOP + i);

      const uint64_t t0 = rdtsc_start();
      book.addOrder(OrderType::GOOD_TILL_CANCEL, o);
      const uint64_t t1 = rdtsc_end();
      samples.push_back(t1 - t0);
    }
    report("addOrder (no match)(new level)", samples, ns);
  }

  // addOrder — full match (cold cache per book, constructor cost excluded)
  {
    samples.clear();
    std::vector<OrderBook> books;
    books.reserve(N_SAMPLES);
    for (int i = 0; i < N_SAMPLES; i++) {
      books.emplace_back(2, 1);
      books[i].addOrder(OrderType::GOOD_TILL_CANCEL, Order(1, Side::SELL, 10000, 50, 1));
    }

    for (int i = 0; i < N_SAMPLES; i++) {
      const Order o(2, Side::BUY, 10000, 50, 2);
      const uint64_t t0 = rdtsc_start();
      books[i].addOrder(OrderType::GOOD_TILL_CANCEL, o);
      const uint64_t t1 = rdtsc_end();
      samples.push_back(t1 - t0);
    }
    report("addOrder (full match)", samples, ns);
  }

  // cancelOrder — warm book, all orders pre-loaded
  {
    samples.clear();
    OrderBook book(PREPOP + N_SAMPLES, PREPOP);
    for (int j = 0; j < PREPOP + N_SAMPLES; j++)
      book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(j, Side::BUY, 9500 + (j % PREPOP), 10, j + 1));

    for (int i = 0; i < N_SAMPLES; i++) {
      const uint64_t t0 = rdtsc_start();
      book.cancelOrder(PREPOP + i);
      const uint64_t t1 = rdtsc_end();
      samples.push_back(t1 - t0);
    }
    report("cancelOrder", samples, ns);
  }

  // sweep N price levels
  for (int levels : {8, 64, 256, 1024}) {
    samples.clear();
    char name[48];
    std::snprintf(name, sizeof(name), "sweep %d levels", levels);

    std::vector<OrderBook> books;
    books.reserve(N_SAMPLES);
    for (int i = 0; i < N_SAMPLES; i++) {
      books.emplace_back(levels + 1, levels);
      for (int j = 0; j < levels; j++)
        books[i].addOrder(OrderType::GOOD_TILL_CANCEL, Order(j, Side::SELL, 10000 + j, 10, j + 1));
    }

    for (int i = 0; i < N_SAMPLES; i++) {
      const Order o(N_SAMPLES + i, Side::BUY, 10000 + levels, 9999, N_SAMPLES + i);
      const uint64_t t0 = rdtsc_start();
      books[i].addOrder(OrderType::GOOD_TILL_CANCEL, o);
      const uint64_t t1 = rdtsc_end();
      samples.push_back(t1 - t0);
    }
    report(name, samples, ns);
  }

  return 0;
}
