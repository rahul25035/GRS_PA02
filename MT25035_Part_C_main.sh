#!/bin/bash
# MT25035_Part_C_main.sh

# --------------------------------------------------
# Compile all implementations
# --------------------------------------------------
echo "[*] Compiling programs..."

gcc -O0 -g MT25035_Part_A_A1_server.c -o a1_server -pthread
gcc -O0 -g MT25035_Part_A_A1_client.c -o a1_client -pthread

gcc -O0 -g MT25035_Part_A_A2_server.c -o a2_server -pthread
gcc -O0 -g MT25035_Part_A_A2_client.c -o a2_client -pthread

gcc -O0 -g MT25035_Part_A_A3_server.c -o a3_server -pthread
gcc -O0 -g MT25035_Part_A_A3_client.c -o a3_client -pthread

echo "[*] Compilation done"
echo "=========================================="

# --------------------------------------------------
# Experiment configuration
# --------------------------------------------------
MESSAGE_SIZES=(64 256 1024 4096)
THREAD_COUNTS=(1 2 4 8)

DURATION=10
MAX_SERVER_THREADS=32

RESULTS_FILE="MT25035_Part_C_Results.csv"

# Write CSV header
echo "impl,field_size,threads,throughput_gbps,latency_us,cycles,cache_misses,llc_misses,context_switches" \
  | tee "$RESULTS_FILE"

# --------------------------------------------------
# Run experiments
# --------------------------------------------------
for IMPL in a1 a2 a3; do
  SERVER=./${IMPL}_server
  CLIENT=./${IMPL}_client

  for FS in "${MESSAGE_SIZES[@]}"; do
    for T in "${THREAD_COUNTS[@]}"; do

      echo "[*] Running $IMPL field_size=$FS threads=$T" >&2

      # Start server
      $SERVER $FS $MAX_SERVER_THREADS &
      SERVER_PID=$!
      sleep 1

      # Run client under perf
      PERF_OUT=$(perf stat \
        -e cycles,cache-misses,LLC-load-misses,context-switches \
        $CLIENT $FS $T $DURATION 2>&1)

      # Stop server
      kill $SERVER_PID
      wait $SERVER_PID 2>/dev/null

      # --------------------------------------------------
      # Application-level metrics
      # --------------------------------------------------
      TOTAL_MSGS=$(echo "$PERF_OUT" | \
        grep MESSAGES | awk '{sum += $NF} END {print sum}')

      if [ -z "$TOTAL_MSGS" ] || [ "$TOTAL_MSGS" -eq 0 ]; then
        TOTAL_MSGS=1
      fi

      BYTES_PER_MSG=$((FS * 8))
      TOTAL_BYTES=$((TOTAL_MSGS * BYTES_PER_MSG))

      THROUGHPUT_GBPS=$(awk -v b=$TOTAL_BYTES -v d=$DURATION \
        'BEGIN { printf "%.6f", (b*8)/(d*1e9) }')

      LATENCY_US=$(awk -v d=$DURATION -v m=$TOTAL_MSGS \
        'BEGIN { printf "%.3f", (d/m)*1e6 }')

      # --------------------------------------------------
      # perf metrics (remove commas!)
      # --------------------------------------------------
      CYCLES=$(echo "$PERF_OUT" | grep -m1 cycles | awk '{print $1}' | tr -d ',')
      CACHE_MISSES=$(echo "$PERF_OUT" | grep -m1 cache-misses | awk '{print $1}' | tr -d ',')
      LLC_MISSES=$(echo "$PERF_OUT" | grep -m1 LLC-load-misses | awk '{print $1}' | tr -d ',')
      CTX_SWITCHES=$(echo "$PERF_OUT" | grep -m1 context-switches | awk '{print $1}' | tr -d ',')

      # --------------------------------------------------
      # Output (terminal + CSV)
      # --------------------------------------------------
      echo "$IMPL,$FS,$T,$THROUGHPUT_GBPS,$LATENCY_US,$CYCLES,$CACHE_MISSES,$LLC_MISSES,$CTX_SWITCHES" \
        | tee -a "$RESULTS_FILE"

    done
  done
done

#cleanup:
rm a1_server a1_client a2_server a2_client a3_server a3_client
