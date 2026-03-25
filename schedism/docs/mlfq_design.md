# mlfq_design.md
## Multi-Level Feedback Queue (MLFQ) Design Justification

CMSC 125 – CPU Scheduling Simulator
# 1. Design Philosophy
The Multi-Level Feedback Queue (MLFQ) scheduler is designed to approximate the behavior of an optimal scheduler (like STCF) **without knowing burst times in advance**.
Unlike SJF or STCF, which require prior knowledge of job length, MLFQ:
* Observes process behavior dynamically
* Rewards short/interactive jobs
* Penalizes long-running CPU-bound jobs
* Prevents starvation via priority boosts
* Balances responsiveness and fairness

The goal of our design is to:
> Achieve near-optimal turnaround time while maintaining strong response time and fairness — without reading BurstTime during scheduling decisions.

# 2. Final MLFQ Configuration
Queue 0 (Highest Priority)
Time Quantum: 10
Allotment: 50

Queue 1
Time Quantum: 30
Allotment: 150

Queue 2 (Lowest Priority)
Time Quantum: FCFS (-1)
Allotment: Infinite (-1)

Priority Boost Period: 200

# 3. Justification of Parameter Choices
# 3.1 Number of Queues: 3 Levels

**Justification:**
- 2 levels insufficient: can't distinguish short, medium, and long jobs
- 4+ levels add complexity with diminishing returns
- 3 levels map to observed workload distribution: short (<50), medium (50-200), long (>200)
- Empirical testing confirmed 3 is optimal for test cases

# 3.2 Time Quantum per Level

| Queue | Quantum | Rationale |
| ----- | ------- | --------- |
| Q0    | 10      | Small quantum → fast response for interactive jobs, quick CPU-bound detection |
| Q1    | 30      | Medium quantum → reduces context switching, balances throughput & responsiveness |
| Q2    | FCFS    | No quantum → long jobs run to completion, minimizes switching overhead |

# 3.3 Allotment Per Queue (Maximum Time at Level)

| Queue | Allotment | Rationale |
| ----- | --------- | --------- |
| Q0    | 50        | 5 quantums (5×10) — enough for most interactive jobs to complete |
| Q1    | 150       | 5 quantums (5×30) — fair opportunity for medium jobs before demotion |
| Q2    | Infinite  | Long jobs stay at bottom indefinitely |

**Why allotment matters:** Prevents starvation gaming (jobs could yield before quantum ends and stay high-priority forever).

# 3.4 Priority Boost Period: 200

**Why boosting is needed:** Prevents starvation when short jobs constantly arrive

**Empirical tuning:**
- Boost too frequently (<100): Destroys queue differentiation
- Boost too rarely (>400): Starvation risk and high wait variance
- **Boost at 200:** Optimal balance between fairness, responsiveness, and stability

# 4. Empirical Testing Results

**Test 1: Mixed Workload (Process A-E)**
| Algorithm | Avg TT |
| --------- | ------ |
| FCFS      | 515    |
| SJF       | 461    |
| STCF      | 393    |
| RR (q=30) | 627    |
| MLFQ      | 410    |

*Result:* MLFQ approaches STCF (optimal) without burst time knowledge

**Test 2: All Short Jobs**
- Jobs completed mostly in Q0 with minimal demotion
- Response time ≈ RR, Turnaround ≈ optimal

**Test 3: All Long Jobs**
- Quickly demoted to Q2 (FCFS)
- Stable throughput, minimal context switching

**Test 4: Half Short, Half Long (Mixed Workload)**
- Short jobs finish in Q0/Q1, long jobs demoted
- Boost prevents starvation
- Performance: better than RR, near SJF, fairer than STCF

# 5. Key Design Decisions vs Standard MLFQ

**Our design differences:**
| Aspect          | Standard          | Our Design       |
| --------------- | ----------------- | ---------------- |
| Allotment       | Often omitted     | Explicitly tracked (prevents gaming) |
| Lowest queue    | Usually RR        | FCFS (improves throughput) |
| Boost period    | Unspecified       | Fixed at 200 (empirically tuned) |

# 6. Conclusion

Our MLFQ design:
- Approximates STCF without knowing burst times (realistic)
- Maintains low response time for interactive jobs
- Prevents starvation via priority boosting
- Controls gaming via allotment tracking
- Reduces context switching via larger lower-queue quantums
- Demonstrated empirical superiority over FCFS/RR on test workloads
