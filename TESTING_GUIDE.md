# CPU Scheduler Simulation - Process Execution Model Testing Guide

## Overview

Your CPU scheduler simulator is tested using the **fork/exec Process Execution Model**, which is the standard way operating systems launch child processes.

## What's Being Tested

Your simulator must:

1. **Run as a standalone executable** (no dependencies)
2. **Accept command-line arguments** via `argv` 
3. **Return appropriate exit codes** (0 = success, 1 = error)
4. **Work when spawned by parent via fork() + exec()**

## Quick Start

### 1. Verify Standalone Execution

```bash
./schedsim --algorithm=FCFS --input=workload.txt
```

You should see:
- Per-process metrics (Turnaround Time, Waiting Time, Response Time)
- Average metrics
- ASCII Gantt chart
- Context switch count

✓ Exit code 0 if successful, 1 if error

### 2. Run the Automated Fork/Exec Test

```bash
gcc -Wall test_exec_model.c -o test_exec_model
./test_exec_model
```

You should see:
```
=== Process Execution Model Test Suite ===

Testing: Fork/Exec FCFS with workload.txt ... ✓ PASS (exit code: 0)
Testing: Fork/Exec STCF with workload.txt ... ✓ PASS (exit code: 0)
...
=== Test Summary ===
Passed: 9/9
```

This proves:
- ✓ Your simulator can be spawned as a child process
- ✓ It accepts arguments correctly
- ✓ Exit codes propagate properly
- ✓ Error handling works

### 3. Manual Fork/Exec Test

The test harness itself demonstrates fork/exec. Here's what it does:

```c
pid_t pid = fork();                    // Create child process
if (pid == 0) {
    // In child process
    char *args[] = {"./schedsim", "--algorithm=FCFS", 
                    "--input=workload.txt", NULL};
    execvp(args[0], args);             // Replace child with ./schedsim
} else {
    // In parent process
    int status;
    waitpid(pid, &status, 0);          // Wait for child to complete
    int exit_code = WEXITSTATUS(status);
    // Now parent has the exit code!
}
```

---

## Test Scenarios (Different Workloads)

### Scenario 1: Short Bursts
**File:** `workload_short.txt`

```bash
./schedsim --compare --input=workload_short.txt
```

**What to observe:**
- RR/MLFQ have lower response times (better interactivity)
- FCFS has high response time (first process waits for all)
- Context switches visible in RR/MLFQ

**Why:** With short jobs, preemptive scheduling allows better fairness.

### Scenario 2: Long Bursts
**File:** `workload_long.txt`

```bash
./schedsim --compare --input=workload_long.txt
```

**What to observe:**
- SJF/STCF have lower turnaround times
- MLFQ performs similarly to STCF
- More context switches in STCF vs non-preemptive

**Why:** With longer jobs, sorting by burst time reduces total wait.

### Scenario 3: Simultaneous Arrivals
**File:** `workload_simultaneous.txt`

```bash
./schedsim --compare --input=workload_simultaneous.txt
```

**What to observe:**
- FCFS strictly orders by arrival (all at t=0, so by PID)
- MLFQ's priority boost ensures fairness
- RR distributes time evenly

**Why:** Tests queuing behavior when many processes arrive together.

---

## Exit Code Validation

### Success Cases

```bash
# All should exit 0
./schedsim --algorithm=FCFS --input=workload.txt ; echo $?
./schedsim --algorithm=RR --quantum=50 --input=workload.txt ; echo $?
./schedsim --algorithm=MLFQ --mlfq-config=mlfq_config.txt --input=workload.txt ; echo $?
./schedsim --compare --input=workload.txt ; echo $?
```

### Error Cases

```bash
# All should exit 1
./schedsim --algorithm=INVALID --input=workload.txt ; echo $?
./schedsim --algorithm=RR --quantum=0 --input=workload.txt ; echo $?
./schedsim --algorithm=FCFS --input=missing.txt ; echo $?
./schedsim --algorithm=FCFS ; echo $?
```

---

## Understanding the Output

### Per-Process Metrics

```
--- Process 1 ---
Arrival Time: 0
Burst Time: 5
Start Time: 0
Finish Time: 5
Response Time: 0 - 0 = 0
Turnaround Time: 5 - 0 = 5
Waiting Time: 5 - 5 = 0
```

**Formula explanations:**
- **Turnaround Time (TT)** = Finish Time - Arrival Time = How long the process took total
- **Waiting Time (WT)** = Turnaround Time - Burst Time = How long it waited
- **Response Time (RT)** = Start Time - Arrival Time = How long until first scheduled

### Gantt Charts

**ASCII Gantt (1 char = 1 time unit):**
```
Gantt Chart (process times):
111222333444555
0   5   10  15
```
- Shows which process ran at each time unit
- Use when simulation is small (< 80 units)

**Scaled Gantt (1 char = 10 time units):**
```
Gantt Chart (scaled):
1234
0       100     200
```
- Each cell represents 10 units
- Use when simulation is large

### Comparison Table

```
Algorithm | Avg TT  | Avg WT  | Avg RT | Context Switches
----------|---------|---------|--------|------------------
FCFS      | 50.0    | 22.5    | 0.0    | 0
SJF       | 40.0    | 12.5    | 0.0    | 0
STCF      | 35.0    | 10.2    | 5.5    | 4
RR        | 48.0    | 20.5    | 12.2   | 12
MLFQ      | 33.0    | 8.5     | 3.0    | 6
```

**Interpretation:**
- Low Avg TT → Good overall throughput
- Low Avg WT → Processes don't wait long
- Low Avg RT → Interactive (responds quickly)
- Few context switches → Low overhead (but STCF trades overhead for latency)

---

## Defense Checklist

### Compilation ✓
```bash
make clean
make
```
- [ ] Compiles with no warnings
- [ ] Binary is `./schedsim`

### Standalone Execution ✓
```bash
./schedsim --algorithm=FCFS --input=workload.txt
```
- [ ] Displays output
- [ ] Returns exit code 0

### Fork/Exec Tests ✓
```bash
./test_exec_model
```
- [ ] Shows 9/9 tests passing
- [ ] Validates all algorithms
- [ ] Validates error handling

### Error Handling ✓
```bash
./schedsim --invalid-flag ; echo $?
```
- [ ] Displays error message
- [ ] Returns exit code 1

### Compare Mode ✓
```bash
./schedsim --compare --input=workload.txt
```
- [ ] Outputs table with all 5 algorithms
- [ ] Metrics are reasonable
- [ ] Returns exit code 0

### Multiple Scenarios ✓
```bash
./schedsim --compare --input=workload_short.txt
./schedsim --compare --input=workload_long.txt
./schedsim --compare --input=workload_simultaneous.txt
```
- [ ] All produce correct output
- [ ] Different workloads show different winners
- [ ] All return exit code 0

---

## Key Points to Explain During Defense

1. **Fork/Exec Model**: "My simulator is a standard Unix process that can be spawned by another program using fork() and exec(). The test harness proves this by spawning it and checking the exit code."

2. **Exit Codes**: "I return 0 for success and 1 for errors. This follows Unix conventions and allows parent programs to detect failures."

3. **Command-Line Arguments**: "I parse arguments using standard argc/argv. This allows full flexibility in how the simulator is invoked."

4. **Workload Flexibility**: "The simulator reads workloads from files, not hardcoded. Different workload files demonstrate how each algorithm performs under different conditions."

5. **Error Handling**: "The test suite includes error cases showing the simulator gracefully handles invalid inputs and missing files."

---

## Advanced Testing (Optional)

### Create Custom Workload

```bash
cat > custom_workload.txt << 'EOF'
# My test workload
1 0 10
2 2 15
3 5 8
4 8 12
EOF

./schedsim --compare --input=custom_workload.txt
```

### Test with Your Lab 1 Shell

If your shell from Lab 1 is functional:

```bash
# Launch your shell
./shell

# Inside shell, try:
schedule ./schedsim --algorithm=FCFS --input=workload.txt

# Your shell should spawn schedsim and display output
```

This demonstrates real process management integration.

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `./schedsim: command not found` | Run `make` first to compile |
| `cannot open input file` | Use full path or ensure workload.txt exists |
| `test_exec_model` won't compile | Check `fcntl.h` is included, try: `gcc -Wall test_exec_model.c -o test_exec_model` |
| Exit code not printing | Add `; echo $?` to see exit code |
| Compare mode empty | Ensure `--input=` flag is provided |

---

## Summary

Your simulator correctly implements the Process Execution Model by:

✓ Working as a standalone executable  
✓ Accepting command-line arguments  
✓ Returning proper exit codes  
✓ Successfully spawning via fork/exec  
✓ Handling both success and error cases  

The automated test suite (`test_exec_model`) confirms all 9 test cases pass, demonstrating full compliance with the fork/exec process model requirement.
