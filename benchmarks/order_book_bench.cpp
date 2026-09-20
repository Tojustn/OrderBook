#include "order.hpp"
#include "order_book.hpp"
#include "types.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

// Read CPU Timestamp Counter
// hi and low bits and then combine them
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

// Calibrate by measuring the number of cycles over a 100ms wall-clock window
static double ns_per_cycle() {
  // Measure elapsed time
  using clk = std::chrono::steady_clock;

  const auto wall0 = clk::now();
  const uint64_t cyc0 = rdtsc_start();

  // Wait 100ms, duration<double> is in seconds
  while (std::chrono::duration<double>(clk::now() - wall0).count() < 0.1) {
  }

  // Get end number of counters
  const uint64_t cyc1 = rdtsc_end();
  const auto wall1 = clk::now();
  const double ns = std::chrono::duration<double, std::nano>(wall1 - wall0).count();

  // return nanoseconds per TSC click
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
static constexpr int N_WARMUP = 5'000;

int main() {
  const double ns = ns_per_cycle();
  std::vector<uint64_t> samples;
  samples.reserve(N_SAMPLES);

  // Cost of adding a non-matching order to an existing level. The timed order is
  // cancelled outside the measurement so depth and pool occupancy remain flat.
  {
    samples.clear();
    OrderBook book(PREPOP + 1, PREPOP);
    for (int j = 0; j < PREPOP; j++)
      book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(j, Side::BUY, j + 1, 10, j + 1));

    OrderId next_id = PREPOP;
    for (int i = 0; i < N_WARMUP; i++) {
      const OrderId id = next_id++;
      book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(id, Side::BUY, (i % PREPOP) + 1, 50, id + 1));
      book.cancelOrder(id);
    }

    for (int i = 0; i < N_SAMPLES; i++) {
      const OrderId id = next_id++;
      const Order o(id, Side::BUY, (i % PREPOP) + 1, 50, id + 1);

      const uint64_t t0 = rdtsc_start();
      book.addOrder(OrderType::GOOD_TILL_CANCEL, o);
      const uint64_t t1 = rdtsc_end();
      samples.push_back(t1 - t0);

      book.cancelOrder(id);
    }
    report("addOrder (no match, existing level)", samples, ns);
  }

  // Cost of adding a non-matching order that creates a new price level. The
  // order is cancelled untimed so each sample begins at the same book depth.
  {
    samples.clear();
    OrderBook book(PREPOP + 1, PREPOP + 1);
    for (int j = 0; j < PREPOP; j++)
      book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(j, Side::BUY, j + 1, 10, j + 1));

    OrderId next_id = PREPOP;
    for (int i = 0; i < N_WARMUP; i++) {
      const OrderId id = next_id++;
      book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(id, Side::BUY, 10000, 50, id + 1));
      book.cancelOrder(id);
    }

    for (int i = 0; i < N_SAMPLES; i++) {
      const OrderId id = next_id++;
      const Order o(id, Side::BUY, 10000, 50, id + 1);

      const uint64_t t0 = rdtsc_start();
      book.addOrder(OrderType::GOOD_TILL_CANCEL, o);
      const uint64_t t1 = rdtsc_end();
      samples.push_back(t1 - t0);

      book.cancelOrder(id);
    }
    report("addOrder (no match, new level)", samples, ns);
  }

  // Cost of matching one resting order on a warm, long-lived book. The resting
  // order is rebuilt outside the timed region before every sample.
  {
    samples.clear();
    OrderBook book(PREPOP + 1, PREPOP + 1);
    for (int j = 0; j < PREPOP; j++)
      book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(j, Side::BUY, j + 1, 10, j + 1));

    const Order resting(PREPOP, Side::SELL, 10000, 50, PREPOP + 1);
    const Order incoming(PREPOP + 1, Side::BUY, 10000, 50, PREPOP + 2);
    for (int i = 0; i < N_WARMUP; i++) {
      book.addOrder(OrderType::GOOD_TILL_CANCEL, resting);
      book.addOrder(OrderType::GOOD_TILL_CANCEL, incoming);
    }

    for (int i = 0; i < N_SAMPLES; i++) {
      book.addOrder(OrderType::GOOD_TILL_CANCEL, resting);

      const uint64_t t0 = rdtsc_start();
      book.addOrder(OrderType::GOOD_TILL_CANCEL, incoming);
      const uint64_t t1 = rdtsc_end();
      samples.push_back(t1 - t0);
    }
    report("addOrder (full match, one order)", samples, ns);
  }

  // Cost of cancelling one order from a populated price level. The cancellable
  // order is added untimed before each sample, keeping depth constant.
  {
    samples.clear();
    OrderBook book(PREPOP + 1, PREPOP);
    for (int j = 0; j < PREPOP; j++)
      book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(j, Side::BUY, j + 1, 10, j + 1));

    OrderId next_id = PREPOP;
    for (int i = 0; i < N_WARMUP; i++) {
      const OrderId id = next_id++;
      book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(id, Side::BUY, (i % PREPOP) + 1, 10, id + 1));
      book.cancelOrder(id);
    }

    for (int i = 0; i < N_SAMPLES; i++) {
      const OrderId id = next_id++;
      book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(id, Side::BUY, (i % PREPOP) + 1, 10, id + 1));

      const uint64_t t0 = rdtsc_start();
      book.cancelOrder(id);
      const uint64_t t1 = rdtsc_end();
      samples.push_back(t1 - t0);
    }
    report("cancelOrder (existing level)", samples, ns);
  }

  // Cost of sweeping exactly N resting price levels on one warm book. Levels
  // are rebuilt outside the timed region and the incoming order never rests.
  for (int levels : {8, 64, 256, 1024}) {
    samples.clear();
    char name[48];
    std::snprintf(name, sizeof(name), "sweep %d levels", levels);

    OrderBook book(PREPOP + levels, PREPOP + levels);
    for (int j = 0; j < PREPOP; j++)
      book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(j, Side::BUY, j + 1, 10, j + 1));

    const auto rebuild_asks = [&book, levels]() {
      for (int j = 0; j < levels; j++)
        book.addOrder(OrderType::GOOD_TILL_CANCEL, Order(PREPOP + j, Side::SELL, 10000 + j, 10, PREPOP + j + 1));
    };
    const Order sweep(PREPOP + levels, Side::BUY, 10000 + levels - 1, static_cast<Quantity>(levels) * 10,
                      PREPOP + levels + 1);

    for (int i = 0; i < N_WARMUP; i++) {
      rebuild_asks();
      book.addOrder(OrderType::GOOD_TILL_CANCEL, sweep);
    }

    for (int i = 0; i < N_SAMPLES; i++) {
      rebuild_asks();

      const uint64_t t0 = rdtsc_start();
      book.addOrder(OrderType::GOOD_TILL_CANCEL, sweep);
      const uint64_t t1 = rdtsc_end();
      samples.push_back(t1 - t0);
    }
    report(name, samples, ns);
  }

  return 0;
}
