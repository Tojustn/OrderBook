import matplotlib.pyplot as plt        
import numpy as np
import pandas as pd
from scipy.stats import gaussian_kde

def main():
    WARMUP_EVENTS = 100_001
    
    df = pd.read_csv("data/latency.csv")
    df = df.iloc[WARMUP_EVENTS:]

    p95 = df["latency_ns"].quantile(0.95)
    df = df[df["latency_ns"] <= p95]

    median = df["latency_ns"].median()

    # Sample for KDE speed
    df_sample = df.sample(n=500_000, random_state=42)
    
    x = np.linspace(0, p95, 1000)
    kde = gaussian_kde(df_sample["latency_ns"], bw_method=0.15)
    y = kde(x)
    y = y / y.max()  # normalize to 0-1 scale

    plt.figure(figsize=(12, 5), facecolor="white")
    plt.fill_between(x, y, alpha=0.5, color="#5B9BD5")
    plt.plot(x, y, color="#5B9BD5", linewidth=1.5, label=f"std::map (median: {median:.0f}ns)")
    plt.axvline(median, color="#5B9BD5", linestyle="--", linewidth=1.5,
                label=f"Median: {median:.0f} ns")

    plt.xlim(0, p95)
    plt.xlabel("Latency (ns)")
    plt.ylabel("Relative Frequency")
    plt.title("OrderBook Latency Distribution")
    plt.legend()
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()