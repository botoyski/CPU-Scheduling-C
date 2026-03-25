#!/bin/bash

# CPU Scheduling Simulator - Automated Test Suite
# This script runs all unit tests and functional tests for the scheduling simulator

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
SIMULATOR="$PROJECT_DIR/schedsim"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Counters
TESTS_PASSED=0
TESTS_FAILED=0

# Helper functions
print_header() {
    echo -e "\n${YELLOW}====== $1 ======${NC}"
}

print_pass() {
    echo -e "${GREEN}✓ PASS${NC}: $1"
    ((TESTS_PASSED++))
}

print_fail() {
    echo -e "${RED}✗ FAIL${NC}: $1"
    ((TESTS_FAILED++))
}

# Build the project
print_header "Building Project"
cd "$PROJECT_DIR"
if make clean && make; then
    print_pass "Project build"
else
    print_fail "Project build"
    echo "Build failed. Exiting."
    exit 1
fi

# Run unit tests
print_header "Running Unit Tests"

# Test: Queue operations
if make test-queue 2>&1 | grep -q "passed"; then
    print_pass "Queue unit tests"
else
    print_fail "Queue unit tests"
fi

# Test: Metrics computation
if make test-metrics 2>&1 | grep -q "passed"; then
    print_pass "Metrics unit tests"
else
    print_fail "Metrics unit tests"
fi

# Functional tests with different workloads
print_header "Running Functional Tests"

# Helper function to run scheduler
run_scheduler_test() {
    local alg=$1
    local workload=$2
    local quantum=${3:-30}
    local config=${4:-}
    
    cd "$PROJECT_DIR"
    
    if [ -z "$config" ]; then
        timeout 10s "$SIMULATOR" --algorithm="$alg" --input="$workload" --quantum="$quantum" > /dev/null 2>&1
    else
        timeout 10s "$SIMULATOR" --algorithm="$alg" --input="$workload" --quantum="$quantum" --mlfq-config="$config" > /dev/null 2>&1
    fi
    return $?
}

# Test each algorithm with short workload
for alg in FCFS SJF STCF RR; do
    if run_scheduler_test "$alg" "test/workload_short.txt"; then
        print_pass "$alg with short workload"
    else
        print_fail "$alg with short workload"
    fi
done

# Test MLFQ with config
if run_scheduler_test "MLFQ" "test/workload_short.txt" 30 "test/mlfq_config.txt"; then
    print_pass "MLFQ with standard config"
else
    print_fail "MLFQ with standard config"
fi

# Test with long workload
for alg in FCFS SJF STCF RR; do
    if run_scheduler_test "$alg" "test/workload_long.txt"; then
        print_pass "$alg with long workload"
    else
        print_fail "$alg with long workload"
    fi
done

# Test comparison mode
print_header "Testing Comparison Mode"
if timeout 10s "$SIMULATOR" --compare --input="test/workload.txt" > /dev/null 2>&1; then
    print_pass "Comparison mode with multiple algorithms"
else
    print_fail "Comparison mode with multiple algorithms"
fi

# Test inline workload format
print_header "Testing Inline Workload Input"
if timeout 10s "$SIMULATOR" --algorithm=FCFS --workload="P1,0,10;P2,1,20;P3,3,15" > /dev/null 2>&1; then
    print_pass "Inline workload format (semicolon-delimited)"
else
    print_fail "Inline workload format (semicolon-delimited)"
fi

if timeout 10s "$SIMULATOR" --algorithm=SJF --processes="P1:0:10,P2:1:20,P3:3:15" > /dev/null 2>&1; then
    print_pass "Inline processes format (colon-delimited)"
else
    print_fail "Inline processes format (colon-delimited)"
fi

# Test with simultaneous arrivals
print_header "Testing Special Workload Cases"
if timeout 10s "$SIMULATOR" --algorithm=RR --input="test/workload_simultaneous.txt" --quantum=20 > /dev/null 2>&1; then
    print_pass "Simultaneous arrivals handling"
else
    print_fail "Simultaneous arrivals handling"
fi

# Test different RR quantums
for quantum in 5 10 25 50; do
    if timeout 10s "$SIMULATOR" --algorithm=RR --input="test/workload.txt" --quantum="$quantum" > /dev/null 2>&1; then
        print_pass "RR with quantum=$quantum"
    else
        print_fail "RR with quantum=$quantum"
    fi
done

# Print summary
print_header "Test Summary"
TOTAL=$((TESTS_PASSED + TESTS_FAILED))
echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests failed: ${RED}$TESTS_FAILED${NC}"
echo "Total tests:  $TOTAL"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed.${NC}"
    exit 1
fi
