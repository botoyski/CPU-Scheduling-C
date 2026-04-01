# CPU Scheduling Simulator (schedism)
CMSC 125 – Lab 2 Design Notes
CPU scheduling in C, discrete-event simulator that demonstrates how operating systems make scheduling decisions to optimize system performance
by Eryl Joseph Aspera and Luis Victor Borbolla

## Build and Test (Current Workflow)
Run all build and test commands from this directory.

```bash
make clean
make
make test-metrics
make test-queue
```

## CLI Quick Reference

```bash
# Single algorithm with workload file
./schedsim --algorithm=RR --quantum=30 --input=test/workload.txt

# Verbose process logging
./schedsim --algorithm=RR --quantum=30 --input=test/workload.txt --verbose

# Compare all algorithms on one workload
./schedsim --compare --input=test/workload.txt

# MLFQ with explicit config
./schedsim --algorithm=MLFQ --mlfq-config=test/mlfq_config.txt --input=test/workload.txt
```

Supported options:
* `--algorithm=FCFS|SJF|STCF|RR|MLFQ`
* `--input=<file>`
* `--workload="PID,ARRIVAL,BURST;..."`
* `--processes="PID:ARRIVAL:BURST,..."`
* `--quantum=<q>` (RR)
* `--mlfq-config=<file>` (MLFQ)
* `--compare`
* `--verbose`

### Expected Output (Examples)

Example 1: RR with verbose logs

```bash
./schedsim --algorithm=RR --quantum=30 --input=test/workload.txt --verbose | head -n 12
```

Expected pattern in output:
* Starts with `Running RR` and `Using time quantum q=30`
* Shows process-level events such as:
    * `Process A arrives and joins RR queue`
    * `Process A runs for up to 30 units`
    * `Process A re-queued (...)`

Example 2: MLFQ without verbose

```bash
./schedsim --algorithm=MLFQ --mlfq-config=test/mlfq_config.txt --input=test/workload.txt
```

Expected pattern in output:
* Starts with `Running MLFQ`
* Prints `=== Per-Process Metrics ===`
* Prints `=== Averages ===`
* Prints Gantt chart / execution summary

Example 3: Compare mode

```bash
./schedsim --compare --input=test/workload.txt
```

Expected pattern in output:
* Prints `=== Algorithm Comparison for ... ===`
* Includes rows for `FCFS`, `SJF`, `STCF`, `RR`, and `MLFQ`
* Shows Avg TT, Avg WT, Avg RT, and context switches

# 1. Problem Analysis
## 1.1 Core Problem
Modern operating systems must decide:
> Which process runs next?
> For how long?
> According to what policy?
Multiple processes compete for CPU time. Different scheduling algorithms optimize different performance goals:
| Goal                               | Optimized By |
| ---------------------------------- | ------------ |
| Simplicity                         | FCFS         |
| Low average turnaround             | SJF          |
| Optimal turnaround (preemptive)    | STCF         |
| Fairness                           | RR           |
| Balanced responsiveness + fairness | MLFQ         |

The challenge of this lab is to:
* Simulate multiple scheduling algorithms
* Compare their performance
* Implement a realistic MLFQ
* Design it as a standalone executable launched via fork/exec
* Use discrete-event simulation

# 2. System Goals
The simulator must:
1. Read workload input (file or CLI)
2. Implement 5 schedulers:
   * FCFS
   * SJF
   * STCF
   * RR
   * MLFQ
3. Calculate metrics:
   * Finish Time (FT)
   * Turnaround Time (TT)
   * Waiting Time (WT)
   * Response Time (RT)
4. Generate Gantt charts
5. Provide comparison mode
6. Work as compiled binary invoked from Unix shell
7. Return proper exit codes
8. Avoid using BurstTime inside MLFQ decisions

# 3. High-Level Architecture

The current codebase is split by responsibility:

* `src/main.c`: top-level orchestration only (load config, dispatch run mode, cleanup)
* `src/cli.c` + `include/cli.h`: CLI parsing, usage output, workload source loading
* `src/scheduler.c`: shared scheduler utilities (algorithm dispatch, compare mode, MLFQ config parsing, common helper logic)
* `src/fcfs.c`, `src/sjf.c`, `src/stcf.c`, `src/rr.c`, `src/mlfq.c`: algorithm-specific scheduling policy
* `src/metrics.c`: summary/per-process metric reporting
* `src/gantt.c`: execution trace storage and Gantt rendering
* `src/queue.c`: queue utilities used by RR/MLFQ

Flow summary:

`main.c` -> `cli.c` (options + workload) -> `scheduler.c` (dispatch/run mode) -> selected algorithm module -> `metrics.c` + `gantt.c`

# 4. Core Design Decisions
## 4.1 Discrete-Time Simulation
We chose **discrete-time simulation (1 unit per tick)** instead of full event-driven priority queue.
Why?
* Easier debugging
* Easier Gantt generation
* Clear logic for preemption
* Deterministic behavior
  
Simulation loop:

for (t = 0; not all complete; t++) {
    handle arrivals
    run current process (if any)
    check completion
    check preemption
    select next process
    record gantt[t]
}

## 4.2 Process Representation
typedef struct {
    char pid[16];
    int arrival_time;
    int burst_time;
    int remaining_time
    
    int start_time;
    int finish_time;

    int turnaround_time;
    int waiting_time;
    int response_time;

    int priority;         // MLFQ
    int time_in_queue;    // MLFQ
} Process;

Key insights:
* `remaining_time` required for STCF
* `start_time` required for response time
* MLFQ does NOT read burst_time
* All metrics computed AFTER simulation

# 5. Algorithm Design
# 5.1 FCFS (First-Come First-Serve)
### Type: Non-preemptive
### Data Structure: FIFO Queue
### Logic:
* Sort by arrival
* Run process until completion
* No context switches
### Strengths:
* Simple
* Low overhead
### Weakness:
* Convoy effect

Example from your output:
Convoy effect detected: Process B waited 230 time units

This happens because:
Long job A blocks all others.

# 5.2 SJF (Shortest Job First)
### Type: Non-preemptive
### Data Structure: Linear ready-process scan per decision point

Current implementation detail:
* At each scheduling decision, SJF scans all arrived-but-unfinished processes
* Chooses the minimum burst time among ready processes

### Strength:
* Optimal average turnaround (non-preemptive)

### Weakness:
* Requires knowing burst time
* Not realistic in OS

# 5.3 STCF (Shortest Time to Completion First)
### Type: Preemptive SJF
### Key Rule:
If a ready process has shorter remaining time, preempt.

### Implementation:
* Tick-by-tick linear scan across ready processes
* Select minimum remaining time each tick

### Strength:
* Provably optimal average turnaround

### Weakness:
* High context switching
* Requires knowing total burst time

# 5.4 Round Robin (RR)
### Type: Preemptive
### Data Structure: Circular Queue

### Rules:
* Each process runs for quantum q
* If unfinished → move to back

### Configurable:
--quantum=30

### Strength:
* Fair
* Good response time

### Weakness:
* High context switching if q too small
* Poor turnaround if q too large

Tradeoff:
| Small q | Better response | More switches |
| Large q | Fewer switches | Worse response |

# 5.5 Multi-Level Feedback Queue (MLFQ)
This is the most important part.

## Core Principle
Unlike SJF/STCF:
> MLFQ DOES NOT KNOW BURST TIME.
It learns behavior dynamically.

## Our Design
We chose 3 queues:
| Queue | Quantum | Allotment | Purpose     |
| ----- | ------- | --------- | ----------- |
| Q0    | 10      | 50        | Interactive |
| Q1    | 30      | 150       | Mixed       |
| Q2    | FCFS    | infinite  | Batch       |

Boost period: 200

## Why 3 Queues?
Because workloads typically fall into:
* Short (<50)
* Medium (50–200)
* Long (>200)
More queues = complexity without large gain.

## Why decreasing quantum?
Short jobs:
* Finish quickly in Q0
Long jobs:
* Gradually demoted
* Prevent monopolization

## Allotment Tracking
Critical detail:
Allotment does NOT reset after quantum.
It resets only after demotion.
This prevents gaming by yielding early.

## Priority Boost
Every 200 units:
* All processes → Q0
* Prevent starvation
* Maintain fairness

## MLFQ Strength
Balances:
* Low response time (like RR)
* Good turnaround (like SJF)
* Fairness (like FCFS)
  
# 6. Metrics Calculation
After simulation:
TT = FT - AT
WT = TT - BT
RT = StartTime - AT

Example:
Arrival Time: 0
Burst Time: 240
Finish Time: 240

Turnaround: 240 - 0 = 240
Waiting:    240 - 240 = 0
Response:   0 - 0 = 0

We compute per-process and averages.

# 7. Gantt Chart Design
Execution is stored as contiguous trace segments (`pid_index`, `start`, `end`).

Rendered output includes:
* Grid-style time slices for readability
* Execution summary by segment
* Context-switch count in the metrics summary

Verbose mode (`--verbose`) is separate from Gantt rendering and prints process-level scheduling events during simulation.

# 8. Comparative Analysis Mode
When using:
--compare

We run all algorithms on same workload and generate:
| Algorithm | Avg TT | Avg WT | Avg RT | Context Switches |
| --------- | ------ | ------ | ------ | ---------------- |
This allows empirical justification.

# 9. Tradeoff Analysis
| Algorithm | Turnaround   | Response  | Fairness  | Realism     |
| --------- | ------------ | --------- | --------- | ----------- |
| FCFS      | Poor         | Poor      | Medium    | Realistic   |
| SJF       | Good         | Good      | Poor      | Unrealistic |
| STCF      | Best         | Good      | Poor      | Unrealistic |
| RR        | Medium       | Good      | Good      | Realistic   |
| MLFQ      | Near-optimal | Very Good | Very Good | Realistic   |

Conclusion:
MLFQ best approximates real OS behavior.

# 10. Technical Compliance
✔ Compiles to standalone binary
✔ Accepts CLI args
✔ Returns exit codes
✔ Works via fork() and exec()
✔ Uses proper memory management
✔ Modular architecture
✔ No BurstTime usage in MLFQ decisions

# 11. Common Pitfalls We Avoided
* Off-by-one completion errors
* Incorrect response time calculation
* Forgetting to reset allotment on demotion
* Using burst_time inside MLFQ decisions
* Boost triggered incorrectly
* Memory leaks

# 12. Testing Strategy
We tested:
* Lecture quiz workload
* Single process
* Simultaneous arrivals
* Zero burst time
* 100+ process stress test
* Identical burst times
* Bimodal workload

Results matched expected lecture averages.

# 12.1 Known Limitations and Assumptions

Assumptions:
* CPU-bound single-core model (one process runs at a time)
* Deterministic discrete-time simulation (time advances in integer ticks)
* No I/O blocking model; workloads only include arrival and CPU burst

Current limitations:
* Input validation focuses on format correctness; semantic edge policies (for example duplicate PID names) are not deeply enforced
* SJF and STCF use linear scans per decision/tick instead of heap-based ready structures
* Gantt rendering is text-based (terminal output), not graphical
* Context switch overhead is counted, but switch time cost is not added to execution time

# 13. Final Design Philosophy
This simulator demonstrates:
* Systems-level thinking
* Fairness vs performance tradeoffs
* Empirical evaluation of scheduling
* Realistic OS-inspired design
* Proper C modularization
* Discrete simulation modeling

# Defense Summary (Short Version)
If asked:
> Why is MLFQ better?
Answer:
Because it dynamically adapts to process behavior without knowing burst time in advance, balancing responsiveness for short jobs and fairness for long jobs, which makes it closer to real operating systems than SJF or STCF.


