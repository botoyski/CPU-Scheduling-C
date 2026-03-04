# CPU Scheduling Simulator (schedism)
CMSC 125 – Lab 2 Design Notes
CPU scheduling in C, discrete-event simulator that demonstrates how operating systems make scheduling decisions to optimize system performance
by Eryl Joseph Aspera and Luis Victor Borbolla


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

                +---------------------+
                |     main.c          |
                | CLI + argument parse|
                +----------+----------+
                           |
                           v
                +---------------------+
                | SchedulerState      |
                | - processes         |
                | - current_time      |
                | - gantt buffer      |
                +----------+----------+
                           |
        +------------------+------------------+
        |         |         |         |        |
        v         v         v         v        v
      FCFS      SJF       STCF       RR      MLFQ
        |         |         |         |        |
        +-----------> Simulation Engine <------+
                           |
                           v
                +---------------------+
                | Metrics + Gantt     |
                +---------------------+

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
### Data Structure: Min-Heap (better than sorting every time)

Why heap?
* O(log n) insertion
* O(log n) extraction
* Efficient for dynamic arrivals

### Strength:
* Optimal average turnaround (non-preemptive)

### Weakness:
* Requires knowing burst time
* Not realistic in OS

# 5.3 STCF (Shortest Time to Completion First)
### Type: Preemptive SJF
### Key Rule:
If new process has shorter remaining time:
→ Preempt immediately.

### Implementation:
* Min-heap ordered by remaining_time
* Check at every arrival

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
Each time unit:
gantt[t] = current_process_pid

For large workloads:
* 1 char = 10 time units
* Scale output

Example:
[A---------][B------][C----]
Time: 0    240     420    570

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


