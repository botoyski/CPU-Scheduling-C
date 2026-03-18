# CPU Scheduler Simulator - Test Plan

## Direct Execution Tests (Standalone)

### Basic Algorithm Tests

```bash
./schedsim --algorithm=FCFS --input=workload.txt
./schedsim --algorithm=SJF --input=workload.txt
./schedsim --algorithm=STCF --input=workload.txt
./schedsim --algorithm=RR --quantum=50 --input=workload.txt
./schedsim --algorithm=MLFQ --mlfq-config=mlfq_config.txt --input=workload.txt
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
mysh> ./schedsim --algorithm=FCFS --input=workload.txt
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