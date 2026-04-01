# CPU Scheduler Simulator - Test Plan

## Direct Execution Tests (Standalone)

### Basic Algorithm Tests

```bash
./schedsim --algorithm=FCFS --input=workload.txt
./schedsim --algorithm=SJF --input=workload.txt
./schedsim --algorithm=STCF --input=workload.txt
./schedsim --algorithm=RR --quantum=50 --input=workload.txt
./schedsim --algorithm=MLFQ --mlfq-config=test/mlfq_config.txt --input=test/workload.txt
```

**Expected:** All exit with code 0 and display per-process metrics + Gantt chart

### Compare Mode

```bash
./schedsim --compare --input=workload.txt
```

**Expected:** Exit code 0, displays unified comparison table with all 5 algorithms

## Error Handling Tests

### Invalid Parameters

```bash
./schedsim --algorithm=RR --quantum=0 --input=workload.txt ; echo $?
./schedsim --algorithm=INVALID --input=workload.txt ; echo $?
./schedsim --algorithm=FCFS --input=does_not_exist.txt ; echo $?
./schedsim --algorithm=MLFQ --input=workload.txt ; echo $?
```

**Expected:** All exit with code 1, display appropriate error messages

### Missing Required Flags

```bash
./schedsim --algorithm=FCFS ; echo $?
./schedsim --input=workload.txt ; echo $?
./schedsim ; echo $?
```

**Expected:** All exit with code 1, display usage error

---

## Process Execution Model Tests (fork/exec)

### Automated Test Harness

The test harness validates your simulator as a child process spawned via fork() and exec():

```bash
/mnt/c/Users/Botoy/Desktop/3rd\ Year/CMSC\ 125/mysh/CMSC-125-lab1/mysh
mysh> ./schedsim --algorithm=FCFS --input=test/workload.txt
```

### Standalone ###
./schedsim --algorithm=FCFS --input=workload.txt

### Fork/Exec Test ###
./test_exec

### Shell Integration Test (Lab 1 Context)

If your Lab 1 shell supports process launching, test:

```bash
# From your Lab 1 shell:
/path/to/schedsim --algorithm=FCFS --input=/path/to/workload.txt
```

**Expected:** Output displays, shell regains control with exit status 0

---

## Validation Checklist for Defense

- [ ] Simulator compiles without warnings: `gcc -Wall -o schedsim *.c`
- [ ] All 5 algorithms produce output via standalone execution
- [ ] test_exec_model shows 9/9 tests passing
- [ ] Compare mode produces aligned table output
- [ ] Error cases return exit code 1
- [ ] Gantt charts render correctly (ASCII + scaled)
- [ ] Per-process metrics show formulas (TT, WT, RT)
- [ ] Context switch counts displayed
- [ ] Manual fork/exec test works
- [ ] Exit codes properly propagate through fork/exec/waitpid chain

---

## Performance Notes

**Typical execution time on workload.txt:**
- FCFS: < 1ms
- SJF: < 1ms
- STCF: < 2ms
- RR (quantum=50): < 2ms
- MLFQ: < 2ms
- Compare (all 5): < 10ms



Run the existing automated suite first

From project root:

bash test/test_suite.sh

This already covers:

- queue unit tests

- metrics unit tests

- core functional runs for FCFS, SJF, STCF, RR, MLFQ

## Process arrival testing (ready-queue timing)

Create tiny workloads with known arrivals, then check first-run timing in output.

Example command:

```bash
./schedsim --algorithm=FCFS --workload="P1,0,5;P2,2,3;P3,5,1"
```

What to verify:

P1 starts at time 0

P2 is not scheduled before time 2

P3 is not scheduled before time 5

Also test simultaneous arrival:

```bash
./schedsim --algorithm=FCFS --input=test/workload_simultaneous.txt
```

## Queue operations (enqueue/dequeue + FIFO + edge cases)

Already in test/test_queue.c

Run directly:

make test-queue

This validates:

invalid init capacity

FIFO push/pop behavior

full/overflow

underflow

wraparound correctness

## SJF/STCF sorting behavior

Use a small crafted workload where shortest job should win.

Example:

```bash
./schedsim --algorithm=SJF --workload="A,0,8;B,0,2;C,0,4"

./schedsim --algorithm=STCF --workload="A,0,8;B,1,2;C,2,1"
```

Verify:

SJF picks shortest among ready jobs

STCF preempts when a shorter remaining-time process arrives

```bash
./schedsim --algorithm=STCF --workload="A,0,240;B,10,180;C,20,150;D,25,80;E,30,130
```