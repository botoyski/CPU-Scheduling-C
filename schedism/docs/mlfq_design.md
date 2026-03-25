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
# 3.1 Number of Queues
We chose **3 priority levels**.
### Why not 2?
Two levels are insufficient to distinguish:
* Very short interactive jobs
* Medium-length jobs
* Long batch jobs
### Why not 4 or more?
Adding more queues increases:
* Complexity
* Context switching
* Tuning difficulty
Empirical testing showed diminishing returns beyond 3 levels.
### Workload observation:
We observed typical job clusters:
* Short (< 50 time units)
* Medium (50–200)
* Long (> 200)
Three queues effectively capture these categories.

# 3.2 Time Quantum per Level
| Queue | Quantum | Purpose                    |
| ----- | ------- | -------------------------- |
| Q0    | 10      | Interactive responsiveness |
| Q1    | 30      | Balanced execution         |
| Q2    | FCFS    | Throughput-oriented        |
### Why Q0 = 10?
Small quantum ensures:
* Fast response for new jobs
* Low latency
* Quick detection of CPU-bound jobs
If quantum were too large:
* Interactive jobs would wait longer
* Responsiveness would degrade
If too small:
* Context switching overhead increases
Testing showed 10 is a good balance.

### Why Q1 = 30?
Medium quantum:
* Reduces context switching
* Improves throughput
* Still allows preemption
This stage filters medium-length jobs from long-running ones.

### Why Q2 = FCFS?
At lowest priority:
* We optimize throughput
* Long jobs run to completion
* Context switching minimized
This mimics batch processing behavior.

# 3.3 Allotment Per Queue
Allotment determines:
> How much total time a process can spend at a level before demotion.
| Queue | Allotment |
| ----- | --------- |
| Q0    | 50        |
| Q1    | 150       |
| Q2    | Infinite  |
### Why Allotment is Necessary
Without allotment:
* Processes could yield before quantum ends
* They could remain at high priority forever
* This enables gaming the scheduler
Allotment ensures:
* Fair demotion
* Long jobs eventually move down

### Why Q0 allotment = 50?
Since Q0 quantum = 10:
* Process gets 5 quantums before demotion
This is enough for:
* Most interactive jobs to complete
* Short CPU bursts to finish quickly

### Why Q1 allotment = 150?
Since Q1 quantum = 30
* 5 quantums before demotion
This provides:
* Fair opportunity for medium jobs
* Gradual transition to batch queue

# 3.4 Priority Boost Period (S = 200)
Boost ensures:
> No process suffers starvation.
Without boosting:
* Long jobs may never execute if short jobs constantly arrive.

Why 200?
Testing showed:
* Boosting too frequently (<100):
  * Destroys queue differentiation
  * Long jobs repeatedly jump to top
* Boosting too rarely (>400):
  * Starvation risk increases
  * Waiting time variance increases
200 provided best balance between:
* Responsiveness
* Fairness
* Stability

# 4. Empirical Testing Results
We tested on four workload types.

# 4.1 Lecture Quiz Workload
A 0 240
B 10 180
C 20 150
D 25 80
E 30 130

Results:
| Algorithm | Avg TT |
| --------- | ------ |
| FCFS      | 515    |
| SJF       | 461    |
| STCF      | 393    |
| RR (q=30) | 627    |
| MLFQ      | 410    |

Observation:
* MLFQ close to STCF
* Much better than FCFS
* Much better response time than SJF
  
# 4.2 All Short Jobs (<50)
MLFQ behavior:
* Most jobs completed in Q0
* Rare demotion
* Response time ≈ RR
* Turnaround time ≈ SJF
Result:
Near-optimal performance.

# 4.3 All Long Jobs (>200)
Behavior:
* Quickly demoted to Q2
* System behaves like FCFS

Result:
* Stable throughput
* Minimal context switching
* Fair sharing

# 4.4 Mixed Workload (Bimodal)
Half short, half long:
Observations:
* Short jobs finish in Q0 or Q1
* Long jobs demoted
* Boost prevents starvation

Average turnaround:
Better than RR
Near SJF
Much fairer than STCF

# 5. Comparison with Standard 3-Level MLFQ
Standard textbook MLFQ often uses:
| Queue | Quantum |
| ----- | ------- |
| Q0    | 8       |
| Q1    | 16      |
| Q2    | 32      |

Differences in our design:
| Aspect          | Textbook              | Our Design         |
| --------------- | --------------------- | ------------------ |
| Quantum scaling | Doubles each level    | Grows non-linearly |
| Allotment       | Often omitted         | Explicitly tracked |
| Lowest queue    | RR                    | FCFS               |
| Boost period    | Sometimes unspecified | Fixed 200          |

### Why we changed it:
* Explicit allotment prevents gaming.
* FCFS lowest level improves throughput.
* Larger second quantum reduces switching overhead.

# 6. Tradeoff Discussion
# 6.1 Responsiveness vs Throughput
Small quantum:
* Better response
* More context switches
Large quantum:
* Better throughput
* Worse response

Our design balances this:
* Small Q0 quantum
* Larger Q1 quantum
* FCFS at bottom

# 6.2 Fairness vs Optimal Turnaround
STCF:
* Optimal turnaround
* Unfair to long jobs

MLFQ:
* Slightly worse turnaround
* Much fairer
* Realistic behavior

# 6.3 Boost Frequency Tradeoff
Too frequent:
* Behaves like RR
* No differentiation
Too rare:
* Starvation risk
Chosen: 200 after testing variance in waiting times.

# 6.4 Context Switching Overhead
Measured average context switches:
| Algorithm | Switches  |
| --------- | --------- |
| STCF      | High      |
| RR        | Very High |
| MLFQ      | Moderate  |
MLFQ provides:
Better performance with fewer switches than RR.

# 7. Why This Design is Realistic
Modern operating systems (e.g., Unix-based systems) use:
* Dynamic priority adjustment
* Feedback-based scheduling
* Starvation prevention
* Multi-level queues
Our design captures these behaviors.
It does not rely on burst time knowledge, making it realistic.

# 8. Conclusion
Our MLFQ design:
* Approximates STCF without burst time knowledge
* Maintains low response time for interactive jobs
* Prevents starvation via boosting
* Controls gaming via allotment tracking
* Reduces context switching at lower levels
* Demonstrates empirical superiority over FCFS and RR
* Provides fairness absent in STCF
It represents a balanced and practical scheduling strategy suitable for general-purpose operating systems.
