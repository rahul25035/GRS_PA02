#!/bin/bash

# Check for root privileges
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run with sudo to manage network namespaces and perf."
   exit 1
fi

# Configuration based on requirements
PORT=8080
DURATION=5
MSG_SIZES=(1024 4096 16384 65536)
THREAD_COUNTS=(1 2 4 8)
VERSIONS=("A1" "A2" "A3")
CSV_FILE="results.csv"
SERVER_IP="10.0.0.1"
CLIENT_IP="10.0.0.2"

# Ensure perf can access hardware counters
echo -1 > /proc/sys/kernel/perf_event_paranoid 2>/dev/null

# 1. Compilation
echo "Compiling implementations..."
# All versions use the common header
gcc MT25035_Part_A_A1_server.c -o A1_server -lpthread
gcc MT25035_Part_A_A1_client.c -o A1_client
gcc MT25035_Part_A_A2_server.c -o A2_server -lpthread
gcc MT25035_Part_A_A2_client.c -o A2_client
gcc MT25035_Part_A_A3_server.c -o A3_server -lpthread
gcc MT25035_Part_A_A3_client.c -o A3_client

# 2. Setup Network Namespaces (Requirement: Separate namespaces)
echo "Setting up isolated network namespaces..."
ip netns add ns_server
ip netns add ns_client
ip link add veth_s type veth peer name veth_c
ip link set veth_s netns ns_server
ip link set veth_c netns ns_client

ip netns exec ns_server ip addr add ${SERVER_IP}/24 dev veth_s
ip netns exec ns_server ip link set veth_s up
ip netns exec ns_server ip link set lo up

ip netns exec ns_client ip addr add ${CLIENT_IP}/24 dev veth_c
ip netns exec ns_client ip link set veth_c up
ip netns exec ns_client ip link set lo up

# Initialize CSV Header
echo "Version,MsgSize,Threads,Throughput_Gbps,Latency_us,Cycles,L1_Misses,LLC_Misses,CS" > $CSV_FILE

# 3. Experiment Loop
for v in "${VERSIONS[@]}"; do
    for size in "${MSG_SIZES[@]}"; do
        for threads in "${THREAD_COUNTS[@]}"; do
            echo "------------------------------------------------"
            echo "Testing $v | MsgSize: $size | Threads: $threads"

            # Start Server in background with perf profiling
            # Tracking cycles, L1 misses, LLC misses, and context switches
            ip netns exec ns_server perf stat -e cycles,L1-dcache-load-misses,LLC-load-misses,context-switches \
                -x, -o perf_output.tmp \
                ./"${v}_server" $PORT $size $threads > server_log.tmp 2>&1 &
            SERVER_PID=$!
            
            sleep 2 # Wait for server to bind

            # Run Client
            # Client connects to SERVER_IP and runs for DURATION seconds
            client_log=$(ip netns exec ns_client ./"${v}_client" $SERVER_IP $PORT $size $DURATION 2>/dev/null)
            
            # Stop the server
            kill $SERVER_PID >/dev/null 2>&1
            wait $SERVER_PID 2>/dev/null
            sleep 1

            # --- Data Extraction & Calculation ---
            # Extract total bytes from client output
            total_bytes=$(echo "$client_log" | grep "Total bytes received" | awk '{print $NF}' | tr -d '\r')
            
            # Default to 0 if command failed to avoid awk syntax errors
            [[ -z "$total_bytes" ]] && total_bytes=0

            # Throughput (Gbps): (Bytes * 8 bits) / (Duration * 10^9)
            throughput=$(awk "BEGIN {if ($total_bytes > 0) print ($total_bytes * 8) / ($DURATION * 1000000000); else print 0}")
            
            # Latency (us): Average time per full message (8 fields)
            latency=$(awk "BEGIN {if ($total_bytes > 0) print ($DURATION * 1000000) / ($total_bytes / ($size * 8)); else print 0}")

            # Extract Perf Metrics (Cycles, L1/LLC Misses, Context Switches)
            cycles=$(grep "cycles" perf_output.tmp | cut -d, -f1 || echo 0)
            l1_miss=$(grep "L1-dcache-load-misses" perf_output.tmp | cut -d, -f1 || echo 0)
            llc_miss=$(grep "LLC-load-misses" perf_output.tmp | cut -d, -f1 || echo 0)
            cs=$(grep "context-switches" perf_output.tmp | cut -d, -f1 || echo 0)

            # Log to CSV
            echo "$v,$size,$threads,$throughput,$latency,$cycles,$l1_miss,$llc_miss,$cs" >> $CSV_FILE
            echo "Result: ${throughput} Gbps | ${latency} us"
        done
    done
done

# 4. Cleanup
echo "Cleaning up network configurations..."
ip netns del ns_server 2>/dev/null
ip netns del ns_client 2>/dev/null
rm A1_server A1_client A2_server A2_client A3_server A3_client *.tmp 2>/dev/null
echo "Experiments complete. Data saved to $CSV_FILE"