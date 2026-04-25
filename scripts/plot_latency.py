import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy.stats import gaussian_kde

def main():
    WARMUP_EVENTS = 100_001

    files = [
        ("data/latency_map.csv",             "#5B9BD5", "std::map"),
        ("data/latency_vector.csv",          "#E07B54", "std::vector"),
        ("data/latency_vector_reversed.csv", "#6DBF67", "vector reversed"),
    ]

    # Load all data first to find global x max
    datasets = []
    global_p95 = 0
    for path, color, label in files:
        df = pd.read_csv(path).iloc[WARMUP_EVENTS:]
        p95 = df["latency_ns"].quantile(0.95)
        global_p95 = max(global_p95, p95)
        datasets.append((df, p95, color, label))

    x = np.linspace(0, global_p95, 1000)

    plt.figure(figsize=(12, 5), facecolor="white")

    for df, p95, color, label in datasets:
        df_clipped = df[df["latency_ns"] <= p95]
        median = df_clipped["latency_ns"].median()

        df_sample = df_clipped.sample(n=500_000, random_state=42)
        kde = gaussian_kde(df_sample["latency_ns"], bw_method=0.15)
        y = kde(x)
        y = y / y.max()

        plt.fill_between(x, y, alpha=0.3, color=color)
        plt.plot(x, y, color=color, linewidth=1.5, label=f"{label} (median: {median:.0f}ns)")
        plt.axvline(median, color=color, linestyle="--", linewidth=1.2)

    plt.xlim(0, global_p95)
    plt.xlabel("Latency (ns)")
    plt.ylabel("Relative Frequency")
    plt.title("OrderBook Latency Distribution")
    plt.legend()
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
