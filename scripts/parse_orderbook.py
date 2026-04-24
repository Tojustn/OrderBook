import json
import csv
import sys

input_file = sys.argv[1] if len(sys.argv) > 1 else "data/2026-04-22_BTCUSDT_ob200.data"
output_file = sys.argv[2] if len(sys.argv) > 2 else "data/events.csv"

# I got this data from bybit 
# Symbol: BTCUSDT, Depth: 200 levels
# Date: 2026-04-22
# URL: https://public.bybit.com/orderbook/

# Raw format:
# {
#   "topic": "orderbook.200.BTCUSDT",
#   "ts": 1776816000869,        # timestamp in ms (Bybit server time)
#   "type": "snapshot" | "delta",
#   "data": {
#     "s": "BTCUSDT",           # symbol
#     "b": [["price", "size"]], # bid updates, sorted descending
#     "a": [["price", "size"]], # ask updates, sorted ascending
#     "u": 28058049,            # update ID, sequential
#     "seq": 105954705558       # cross sequence number
#   },
#   "cts": 1776816000866        # client timestamp ms
# }
#
# type="snapshot": full book state, first message only
# type="delta":    incremental update, all subsequent messages
#
# size="0" means the price level was deleted
# size>0   means add or modify at that price level

#Output: CSV 
# ts        — timestamp ms
# side      — B (bid) or A (ask)
# op        — A (add/modify) or D (delete)
# price     — price level
# size      — volume at that level (0 if deleted)

events = []

with open(input_file, "r") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue

        msg = json.loads(line)
        msg_type = msg.get("type")  # snapshot or delta
        data = msg.get("data", {})
        ts = msg.get("ts")

        bids = data.get("b", [])
        asks = data.get("a", [])

        for bid in bids:
            price, size = bid[0], bid[1]
            op = "D" if float(size) == 0.0 else "A"
            events.append((ts, "B", op, price, size))

        for ask in asks:
            price, size = ask[0], ask[1]
            op = "D" if float(size) == 0.0 else "A"
            events.append((ts, "A", op, price, size))

print(f"Parsed {len(events)} events")

with open(output_file, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["ts", "side", "op", "price", "size"])
    writer.writerows(events)

print(f"Written to {output_file}")

# --- Stats ---
total = len(events)
deletes = sum(1 for e in events if e[2] == "D")
adds = total - deletes
print(f"\nEvent breakdown:")
print(f"  Adds/modifies: {adds} ({100*adds/total:.1f}%)")
print(f"  Deletes:       {deletes} ({100*deletes/total:.1f}%)")

prices = [float(e[3]) for e in events]
mid = sum(prices) / len(prices)
distances = [abs(float(e[3]) - mid) for e in events]

import statistics
print(f"\nPrice distance from mid:")
print(f"  Median: ${statistics.median(distances):.2f}")
print(f"  Mean:   ${statistics.mean(distances):.2f}")
print(f"  p95:    ${sorted(distances)[int(0.95*len(distances))]:.2f}")
print(f"\nDone. Feed events.csv into your C++ replay harness.")