#!/bin/bash
# run_tests.sh
# Runs the stress test for best-fit with merge at several LIVE values and saves the results to results.csv.
# Your own files are not changed: each case is built from a copy in the build/ folder.
#
# How to run (from the folder with your .c and .h files):
#   chmod +x run_tests.sh
#   ./run_tests.sh

RUNS=3                                              # how many times to repeat each case
LIVE_VALUES="512 $(seq 520 10 800)"                 # LIVE values for the tipping point: 512, then 520 to 800 in steps of 10

mkdir -p build
echo "case,best_fit,merge,live,run,success_percent,requests_before_failure,free_kb_at_failure,request_kb_at_failure,utilisation_at_failure,btf,freelist_final,freelist_max,frag_final,frag_max" > results.csv

# Builds and runs one case, then adds one line to results.csv per run
# $1 = case name, $2 = USE_BEST_FIT, $3 = USE_MERGE, $4 = LIVE
run_case()
{
    # Copy the files and set the switches and LIVE in the copies
    cp allocator.c freelist.c freelist.h build/
    sed -e "s/#define USE_BEST_FIT .*/#define USE_BEST_FIT $2/" \
        -e "s/#define USE_MERGE .*/#define USE_MERGE $3/" \
        -e "s/#define MEM_SIZE .*/#define MEM_SIZE (10 * 1024 * 1024)/" \
        allocator.h > build/allocator.h
    sed -e "s/#define LIVE .*/#define LIVE $4/" stress_test.c > build/stress_test.c

    gcc -O2 -o build/stress_test build/stress_test.c build/allocator.c build/freelist.c
    if [ $? -ne 0 ]
    then
        echo "Compile failed for $1"
        return
    fi

    for run in $(seq 1 $RUNS)
    do
        output=$(./build/stress_test)

        # Pull each number out of the stress test report
        success=$(echo "$output" | grep "Successful Requests" | grep -o "[0-9.]*")
        before=$(echo "$output" | grep -A1 "First Failure" | grep "Requests" | grep -o "[0-9]*")
        free_kb=$(echo "$output" | grep "Free Memory at Failure" | grep -o "([0-9.]* KB" | grep -o "[0-9.]*")
        req_kb=$(echo "$output" | grep "Request Size at Failure" | grep -o "[0-9.]*")
        util=$(echo "$output" | grep "Utilization at Failure" | grep -o "[0-9.]*")
        btf=$(echo "$output" | grep "BTF" | grep -o "[0-9.]* x" | grep -o "[0-9.]*")
        list_final=$(echo "$output" | grep -A1 "Freelist Length" | grep "Final" | grep -o "[0-9]*")
        list_max=$(echo "$output" | grep -A2 "Freelist Length" | grep "Maximum" | grep -o "[0-9]*")
        frag_final=$(echo "$output" | grep -A1 "External Fragmentation" | grep "Final" | grep -o "[0-9.]*")
        frag_max=$(echo "$output" | grep -A2 "External Fragmentation" | grep "Maximum" | grep -o "[0-9.]*")

        echo "$1,$2,$3,$4,$run,$success,$before,$free_kb,$req_kb,$util,$btf,$list_final,$list_max,$frag_final,$frag_max" >> results.csv
        echo "$1 (LIVE=$4) run $run: $success% successful"

        # The stress test seeds with time(0), so wait a second for a new seed
        sleep 1
    done
}

# Best-fit with merge at increasing LIVE values (LIVE=512 is the default stress test)
for live in $LIVE_VALUES
do
    run_case "best_fit_merge" 1 1 $live
done

echo
echo "Done. Results saved to results.csv"