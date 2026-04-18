#include "order_book.hpp"
#include "order.hpp"
#include "types.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

#ifdef _MSC_VER
#  include <intrin.h>
static inline uint64_t rdtsc() { return __rdtsc(); }
#else
static inline uint64_t rdtsc() {
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
}
#endif

// Calibrate by measuring cycles over a 100ms wall-clock window
static double ns_per_cycle() {
    using clk = std::chrono::steady_clock;
    const auto     wall0 = clk::now();
    const uint64_t cyc0  = rdtsc();
    while (std::chrono::duration<double>(clk::now() - wall0).count() < 0.1) {}
    const uint64_t cyc1  = rdtsc();
    const auto     wall1 = clk::now();
    const double   ns    = std::chrono::duration<double, std::nano>(wall1 - wall0).count();
    return ns / static_cast<double>(cyc1 - cyc0);
}

static double pct(std::vector<uint64_t>& v, double p) {
    return static_cast<double>(v[static_cast<size_t>(p / 100.0 * (v.size() - 1))]);
}

static void report(const char* name, std::vector<uint64_t>& samples, double ns_per_cyc) {
    std::sort(samples.begin(), samples.end());
    std::printf("%-30s  p50=%6.1f ns  p99=%6.1f ns  p99.9=%6.1f ns\n",
        name,
        pct(samples, 50.0)  * ns_per_cyc,
        pct(samples, 99.0)  * ns_per_cyc,
        pct(samples, 99.9)  * ns_per_cyc);
}

static constexpr int N_SAMPLES = 100'000;
static constexpr int PREPOP    = 100;

int main() {
    const double ns = ns_per_cycle();
    std::vector<uint64_t> samples;
    samples.reserve(N_SAMPLES);

    // addOrder — no match, warm book
    {
        samples.clear();
        OrderBook book;
        for (int j = 0; j < PREPOP; j++)
            book.addOrder(Order(j, Side::BUY, 9500 + j, 10, j + 1));

        for (int i = 0; i < N_SAMPLES; i++) {
            const Order o(PREPOP + i, Side::BUY, 5000, 50, PREPOP + i);

            const uint64_t t0 = rdtsc();
            book.addOrder(o);
            const uint64_t t1 = rdtsc();
            samples.push_back(t1 - t0);
        }
        report("addOrder (no match)", samples, ns);
    }

    // addOrder — full match (cold cache per book, constructor cost excluded)
    {
        samples.clear();
        std::vector<OrderBook> books(N_SAMPLES);
        for (int i = 0; i < N_SAMPLES; i++)
            books[i].addOrder(Order(1, Side::SELL, 10000, 50, 1));

        for (int i = 0; i < N_SAMPLES; i++) {
            const Order o(2, Side::BUY, 10000, 50, 2);
            const uint64_t t0 = rdtsc();
            books[i].addOrder(o);
            const uint64_t t1 = rdtsc();
            samples.push_back(t1 - t0);
        }
        report("addOrder (full match)", samples, ns);
    }

    // cancelOrder — warm book, all orders pre-loaded
    {
        samples.clear();
        OrderBook book;
        for (int j = 0; j < PREPOP + N_SAMPLES; j++)
            book.addOrder(Order(j, Side::BUY, 9500 + j, 10, j + 1));

        for (int i = 0; i < N_SAMPLES; i++) {
            const uint64_t t0 = rdtsc();
            book.cancelOrder(PREPOP + i);
            const uint64_t t1 = rdtsc();
            samples.push_back(t1 - t0);
        }
        report("cancelOrder", samples, ns);
    }

    // sweep N price levels
    for (int levels : {8, 64, 256, 1024}) {
        samples.clear();
        char name[48];
        std::snprintf(name, sizeof(name), "sweep %d levels", levels);
        for (int i = 0; i < N_SAMPLES; i++) {
            OrderBook book;
            for (int j = 0; j < levels; j++)
                book.addOrder(Order(j, Side::SELL, 10000 + j, 10, j + 1));
            const Order o(N_SAMPLES + i, Side::BUY, 10000 + levels, 9999, N_SAMPLES + i);

            const uint64_t t0 = rdtsc();
            book.addOrder(o);
            const uint64_t t1 = rdtsc();
            samples.push_back(t1 - t0);
        }
        report(name, samples, ns);
    }

    return 0;
}
