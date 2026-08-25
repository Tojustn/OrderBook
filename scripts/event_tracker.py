import json
import csv
import sys

""" 
Goal: Track adds, updates and deletes
Reasoning: Compare the L2 Bitcoin orderbook data to typical NASDAQ data

200 LEVEL ORDERBOOK
Raw format:
{
  "topic": "orderbook.200.BTCUSDT",
  "ts": 1776816000869,        # timestamp in ms (Bybit server time)
  "type": "snapshot" | "delta",
  "data": {
    "s": "BTCUSDT",           # symbol
    "b": [["price", "size"]], # bid updates, sorted descending
    "a": [["price", "size"]], # ask updates, sorted ascending
    "u": 28058049,            # update ID, sequential
    "seq": 105954705558       # cross sequence number
  },
  "cts": 1776816000866        # client timestamp ms
}

type="snapshot": full book state, first message only
type="delta":    incremental update, all subsequent messages

size="0" means the price level was deleted
size>0   means add or modify at that price level
"""

# Track combined total
# Track split total per orderbook side


input_file = (
    sys.argv[1]
    if len(sys.argv) > 1
    else "data/2026-04-22_BTCUSDT_ob200.data/2026-04-22_BTCUSDT_ob200.data"
)
output_file = sys.argv[2] if len(sys.argv) > 2 else "data/event_stats.csv"

with open(input_file, "r") as f:
    total_adds = 0
    total_updates = 0 
    total_deletes = 0 

    bid_adds = 0
    bid_updates = 0
    bid_deletes = 0

    ask_adds = 0
    ask_updates = 0
    ask_deletes = 0

    seen_bids = set()
    seen_asks = set()
    for line in f:
        line = line.strip()
        if not line:
            continue
        # Each line is a json that holds what action happened at that timestamp
        # b = bids, a = asks
        # b: (price:side)
        msg = json.loads(line)
        msg_type = msg.get("type")  # snapshot or delta
        data = msg.get("data", {})
        # Timestamp
        ts = msg.get("ts")

        bids = data.get("b", [])
        asks = data.get("a", [])

        for bid in bids:
            price, size = bid[0], bid[1]
            size = float(size)

            if size == 0.0:
                total_deletes += 1
                bid_deletes += 1
                seen_bids.discard(price)
            elif price in seen_bids:
                total_updates += 1
                bid_updates += 1
            else:
                total_adds += 1
                bid_adds += 1
                seen_bids.add(price)

        for ask in asks:
            price, size = ask[0], ask[1]
            size = float(size)

            if size == 0.0:
                total_deletes += 1
                ask_deletes += 1
                seen_asks.discard(price)
            elif price in seen_asks:
                total_updates += 1
                ask_updates += 1
            else:
                total_adds += 1
                ask_adds += 1
                seen_asks.add(price)

with open(output_file, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["side", "adds", "updates", "deletes", "total"])
    writer.writerow(
        [
            "combined",
            total_adds,
            total_updates,
            total_deletes,
            total_adds + total_updates + total_deletes,
        ]
    )
    writer.writerow(
        [
            "bid",
            bid_adds,
            bid_updates,
            bid_deletes,
            bid_adds + bid_updates + bid_deletes,
        ]
    )
    writer.writerow(
        [
            "ask",
            ask_adds,
            ask_updates,
            ask_deletes,
            ask_adds + ask_updates + ask_deletes,
        ]
    )

print(f"Written event statistics to {output_file}")
