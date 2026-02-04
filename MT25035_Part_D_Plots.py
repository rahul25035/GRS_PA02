# MT25035_Part_D_Plots.py

import pandas as pd
import matplotlib.pyplot as plt

# -----------------------------
# Load results
# -----------------------------
CSV_FILE = "MT25035_Part_C_Results.csv"
df = pd.read_csv(CSV_FILE)

# Ensure numeric columns
numeric_cols = [
    "field_size", "threads", "throughput_gbps",
    "latency_us", "cycles", "cache_misses"
]
df[numeric_cols] = df[numeric_cols].apply(pd.to_numeric)

# Bytes per message (8 fields)
df["bytes_per_msg"] = df["field_size"] * 8
df["cycles_per_byte"] = df["cycles"] / (
    df["throughput_gbps"] * 1e9 * 10 / 8
)

# -----------------------------
# Plot 1: Throughput vs Message Size
# -----------------------------
plt.figure()
for impl in ["a1", "a2", "a3"]:
    sub = df[(df["impl"] == impl) & (df["threads"] == 8)]
    plt.plot(sub["field_size"], sub["throughput_gbps"], marker="o", label=impl)

plt.xlabel("Message Size (bytes)")
plt.ylabel("Throughput (Gbps)")
plt.title("Throughput vs Message Size (8 threads)")
plt.legend()
plt.grid(True)
plt.xscale("log")
plt.savefig("throughput_vs_message_size.png")
plt.show()

# -----------------------------
# Plot 2: Latency vs Thread Count
# -----------------------------
plt.figure()
for impl in ["a1", "a2", "a3"]:
    sub = df[(df["impl"] == impl) & (df["field_size"] == 1024)]
    plt.plot(sub["threads"], sub["latency_us"], marker="o", label=impl)

plt.xlabel("Thread Count")
plt.ylabel("Latency (µs)")
plt.title("Latency vs Thread Count (1024-byte messages)")
plt.legend()
plt.grid(True)
plt.savefig("latency_vs_threads.png")
plt.show()

# -----------------------------
# Plot 3: Cache Misses vs Message Size
# -----------------------------
plt.figure()
for impl in ["a1", "a2", "a3"]:
    sub = df[(df["impl"] == impl) & (df["threads"] == 8)]
    plt.plot(sub["field_size"], sub["cache_misses"], marker="o", label=impl)

plt.xlabel("Message Size (bytes)")
plt.ylabel("Cache Misses")
plt.title("Cache Misses vs Message Size (8 threads)")
plt.legend()
plt.grid(True)
plt.xscale("log")
plt.yscale("log")
plt.savefig("cache_misses_vs_message_size.png")
plt.show()

# -----------------------------
# Plot 4: CPU Cycles per Byte Transferred
# -----------------------------
plt.figure()
for impl in ["a1", "a2", "a3"]:
    sub = df[(df["impl"] == impl) & (df["threads"] == 8)]
    plt.plot(sub["field_size"], sub["cycles_per_byte"], marker="o", label=impl)

plt.xlabel("Message Size (bytes)")
plt.ylabel("CPU Cycles per Byte")
plt.title("CPU Cycles per Byte vs Message Size (8 threads)")
plt.legend()
plt.grid(True)
plt.xscale("log")
plt.savefig("cycles_per_byte_vs_message_size.png")
plt.show()
