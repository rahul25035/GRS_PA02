# Roll no: MT25035
# GRS_PA02 – Client–Server Performance

This repository contains a simple client–server benchmark written in C. The goal of this project is to study how different ways of sending data affect performance. We measure throughput, latency, CPU cycles, cache misses, and context switches.

The project is divided into three parts:

* **A1**: Basic send/recv (two copy)
* **A2**: Vectorized I/O using `sendmsg` (one copy)
* **A3**: Zero-copy send using `MSG_ZEROCOPY` (zero copy)

All experiments are run using **separate network namespaces** for the client and the server, to ensure proper isolation.


## File Overview

### Client and Server Implementations

* `MT25035_Part_A_A1_client.c` – Client for Part A1 (send/recv)

* `MT25035_Part_A_A1_server.c` – Server for Part A1

* `MT25035_Part_A_A2_client.c` – Client for Part A2 (sendmsg)

* `MT25035_Part_A_A2_server.c` – Server for Part A2

* `MT25035_Part_A_A3_client.c` – Client for Part A3 (zero-copy)

* `MT25035_Part_A_A3_server.c` – Server for Part A3

Each server handles multiple clients using threads. Each client can also create multiple threads.


### Analysis Files

* `MT25035_Part_C_main.sh` – Main script that runs all codes
* `MT25035_Part_C_Results.csv` – CSV file with all measured results
* `MT25035_Part_D_Plots.py` – Python script to generate plots from the CSV file


### Build System

* `Makefile` – Used to compile and run the programs


## Implementation Details

### Network Namespaces

* The client and server run in **different network namespaces**.
* A virtual Ethernet (veth) pair connects the two namespaces.
* This setup ensures that the client and server are isolated, similar to containers.

### Threading Model

* The server creates one thread per client connection.
* The client creates multiple threads based on input arguments.

### Communication Pattern

* Each client sends a message with 8 fixed-size fields.
* The server replies with 8 fields for each request.

Differences between parts:

* **A1**: Sends each field using separate `send()` calls.
* **A2**: Sends all fields together using `sendmsg()`.
* **A3**: Uses `sendmsg()` with `MSG_ZEROCOPY` to reduce data copying.


## How to Build

To compile all client and server programs:

```bash
make all
```

This will generate the following binaries:

* `a1_server`, `a1_client`
* `a2_server`, `a2_client`
* `a3_server`, `a3_client`


## How to Run Individual Programs

Run server first, then client (example for A1):

```bash
./a1_server 64 32
./a1_client 64 8 10
```

Arguments:

* `field_size`: size of each field in bytes
* `num_threads`: number of client threads
* `duration`: run time in seconds
* `max_threads`: maximum server threads


## How to Run Full Experiment (Part C)

**Must be run as root** (required for namespaces and perf).

```bash
sudo make partC
```

This will:

1. Create network namespaces
2. Compile all programs
3. Run benchmarks for A1, A2, and A3
4. Collect performance data using `perf`
5. Save results to `MT25035_Part_C_Results.csv`
6. Generate plots using Python


## Output

* `MT25035_Part_C_Results.csv` contains:

  * Throughput (Gbps)
  * Latency (microseconds)
  * CPU cycles
  * Cache misses
  * LLC misses
  * Context switches

* Plots are generated as `.png` files by `MT25035_Part_D_Plots.py`.


## Cleanup

To remove all binaries and generated files:

```bash
make clean
```


## Notes

* The benchmark script safely handles server termination.
* `perf` errors due to normal process exit are ignored by design.
* All logic is kept minimal to make results easy to compare.


## Summary

This project compares three client–server communication methods under the same conditions. By using threads, namespaces, and performance counters, it shows how different I/O techniques affect system performance in a clear and controlled way.
