#!/bin/bash
# MT25035_Part_C_shell.sh
# Minimal, beginner-friendly script to run Part C experiments.
# Must run as root (sudo).

if [[ $EUID -ne 0 ]]; then
    echo "Run with sudo"
    exit 1
fi

PORT=8080
DURATION=5
MSG_SIZES=(1024 4096 16384 65536)
THREAD_COUNTS=(1 2 4 8)
VERSIONS=("A1" "A2" "A3")
CSV_FILE="results.csv"
SERVER_IP="10.0.0.1"
CLIENT_IP="10.0.0.2"

# Try to allow perf counters (ignore errors)
echo -1 > /proc/sys/kernel/perf_event_paranoid 2>/dev/null || true

# ---------- Compile (fail early if compile error) ----------
gcc -O2 MT25035_Part_A_A1_server.c -o A1_server -lpthread || { echo "Compile A1_server failed"; exit 1; }
gcc -O2 MT25035_Part_A_A1_client.c -o A1_client || { echo "Compile A1_client failed"; exit 1; }
gcc -O2 MT25035_Part_A_A2_server.c -o A2_server -lpthread || { echo "Compile A2_server failed"; exit 1; }
gcc -O2 MT25035_Part_A_A2_client.c -o A2_client || { echo "Compile A2_client failed"; exit 1; }
gcc -O2 MT25035_Part_A_A3_server.c -o A3_server -lpthread || { echo "Compile A3_server failed"; exit 1; }
gcc -O2 MT25035_Part_A_A3_client.c -o A3_client || { echo "Compile A3_client failed"; exit 1; }

# ---------- Cleanup helper ----------
cleanup() {
    ip netns del ns_server 2>/dev/null || true
    ip netns del ns_client 2>/dev/null || true
    pkill -f perf 2>/dev/null || true
    rm -f A*_server A*_client server.tmp perf.tmp client.tmp
}
trap cleanup EXIT

# Remove leftover namespaces (if any)
ip netns del ns_server 2>/dev/null || true
ip netns del ns_client 2>/dev/null || true

# ---------- Setup namespaces ----------
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

# ---------- CSV Header ----------
echo "Version,MsgSize,Threads,Throughput_Gbps,Latency_us,Cycles,L1_Misses,LLC_Misses,ContextSwitches" > $CSV_FILE

# ---------- Experiments ----------
for v in "${VERSIONS[@]}"; do
    for size in "${MSG_SIZES[@]}"; do
        for threads in "${THREAD_COUNTS[@]}"; do

            # Start server under perf (perf writes to perf.tmp). Keep perf in background.
            ip netns exec ns_server perf stat \
                -e cycles,L1-dcache-load-misses,LLC-load-misses,context-switches \
                -x, -o perf.tmp \
                ./${v}_server $PORT $size $threads \
                > server.tmp 2>&1 &

            PERF_PID=$!
            # give server a moment to start
            sleep 1

            # Run client from client namespace, capture output
            client_log=$(ip netns exec ns_client timeout $((DURATION+5)) \
                ./${v}_client $SERVER_IP $PORT $size $DURATION 2>&1)

            echo "$client_log" > client.tmp

            # Ask the server process inside the server namespace to shut down gracefully
            ip netns exec ns_server pkill -INT -f "${v}_server" 2>/dev/null || true

            # Wait for perf (and server) to finish
            wait $PERF_PID 2>/dev/null || true

            # Extract total bytes received (assumes client prints a line containing "Total bytes received <num>")
            total_bytes=$(awk '/Total bytes received/ {print $NF}' client.tmp | tail -n1)
            [[ -z "$total_bytes" ]] && total_bytes=0

            # Messages received (integer)
            messages_received=$(awk -v tb="$total_bytes" -v sz="$size" 'BEGIN{ if(sz>0) print int(tb/sz); else print 0 }')

            # Throughput in Gbps
            throughput=$(awk -v tb="$total_bytes" -v d="$DURATION" 'BEGIN{printf "%.6f", (tb*8)/(d*1e9)}')

            # Average latency per message in microseconds (DURATION seconds / messages)
            if [[ "$messages_received" -gt 0 ]]; then
                latency_us=$(awk -v d="$DURATION" -v m="$messages_received" 'BEGIN{printf "%.3f", (d*1e6)/m}')
            else
                latency_us=0
            fi

            # Parse perf outputs (remove thousands separators if present)
            cycles=$(awk -F, '/cycles/ {gsub(/,/, "", $1); print $1; exit}' perf.tmp 2>/dev/null || echo 0)
            l1=$(awk -F, '/L1-dcache-load-misses/ {gsub(/,/, "", $1); print $1; exit}' perf.tmp 2>/dev/null || echo 0)
            llc=$(awk -F, '/LLC-load-misses/ {gsub(/,/, "", $1); print $1; exit}' perf.tmp 2>/dev/null || echo 0)
            cs=$(awk -F, '/context-switches/ {gsub(/,/, "", $1); print $1; exit}' perf.tmp 2>/dev/null || echo 0)

            # Append to CSV
            echo "$v,$size,$threads,$throughput,$latency_us,$cycles,$l1,$llc,$cs" >> $CSV_FILE

            # small pause between runs
            sleep 0.5
        done
    done
done

# ---------- Cleanup ----------
# (trap will run cleanup)
echo "Experiments complete → $CSV_FILE"