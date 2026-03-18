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
gcc -Wall test_exec_model.c -o test_exec_model
./test_exec_model
```

**What it validates:**
- ✓ Simulator launches correctly via execvp() from child process
- ✓ All algorithms (FCFS, SJF, STCF, RR, MLFQ) return exit code 0
- ✓ Compare mode works via fork/exec
- ✓ Error conditions return exit code 1 from child
- ✓ Command-line arguments properly passed through argv
- ✓ stdin/stdout/stderr correctly inherited

### Manual fork/exec Test

You can also test manually with a simple parent process:

```bash
cat > manual_fork_test.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("Parent PID: %d\n", getpid());
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return 1;
    }
    
    if (pid == 0) {
        // Child process
        printf("Child PID: %d (parent: %d)\n", getpid(), getppid());
        
        char *args[] = {
            "./schedsim", 
            "--algorithm=FCFS", 
            "--input=workload.txt", 
            NULL
        };
        
        execvp(args[0], args);
        perror("exec failed");
        exit(1);
    } else {
        // Parent waits
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            printf("Child exited with code: %d\n", exit_code);
            return exit_code;
        }
    }
    
    return 0;
}
EOF
gcc -Wall manual_fork_test.c -o manual_fork_test
./manual_fork_test
```

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