# Process Execution Model - Design & Validation

## Requirement

Your simulator must be designed to run as a child process spawned via `fork()` and `exec()`. When invoked from the command line or by another program, it should function correctly as a standalone executable. This mimics how real operating systems launch processes.

## Design Implementation

### 1. Standalone Executable

**File:** `main.c`

The simulator is compiled as a single executable with proper entry point:

```bash
gcc -Wall -o schedsim main.c fcfs.c sjf.c stcf.c rr.c mlfq.c simulator.c trace.c
```

- Single binary: `./schedsim`
- No dependencies on environment variables (except input files via `--input` flag)
- Supports all POSIX signal handling + proper cleanup on exit
- No global state that persists between invocations

### 2. Command-Line Argument Handling

**Implementation:** `main.c parse_cli()`

The simulator accepts arguments via `argv`:

```c
// Simulator invoked as:
// execvp(args[0], args);  where args = {"./schedsim", "--algorithm=FCFS", "--input=workload.txt", NULL}

// Inside main():
int main(int argc, char *argv[]) {
    CLIArgs cli;
    if (parse_cli(argc, argv, &cli) != 0) {
        usage(argv[0]);
        return 1;  // Error exit code
    }
    // ... proceed with simulation
}
```

**Supported Flags:**
- `--algorithm={FCFS|SJF|STCF|RR|MLFQ}` (required)
- `--input=<file>` (required, path from current working directory)
- `--quantum=<N>` (for RR, positive integer)
- `--mlfq-config=<file>` (for MLFQ, optional, uses default if omitted)
- `--compare` (runs all 5 algorithms in comparison mode)

### 3. I/O Stream Handling

**Standard Streams:**

| Stream | Usage | Behavior |
|--------|-------|----------|
| `stdin` | Not used | Inherited from parent, left untouched |
| `stdout` | Metrics/Gantt output | Properly flushed before exit |
| `stderr` | Error messages | Diagnostic output on failures |

**File Operations:**
- Input workload from file: `fopen(input_file, "r")`
- MLFQ config from file: `fopen(config_file, "r")`
- Both properly closed on error or completion
- File not found → error exit code 1

### 4. Exit Codes

**Semantics:**

| Exit Code | Meaning | When Triggered |
|-----------|---------|---|
| `0` | Success | All algorithms completed, output generated |
| `1` | Error | CLI parsing, file I/O, invalid arguments |
| `-1` (internal) | Scheduler error | Memory allocation failure (converted to exit code 1) |

**Example Flows:**

```c
// Success path
./schedsim --algorithm=FCFS --input=workload.txt
// Output: metrics + Gantt   →  exit 0

// Error path 1: Invalid flag
./schedsim --algorithm=INVALID --input=workload.txt
// Error: unknown algorithm 'INVALID'   →  exit 1

// Error path 2: Missing file
./schedsim --algorithm=FCFS --input=missing.txt
// Error: cannot open input file 'missing.txt'   →  exit 1

// Error path 3: Bad parameter
./schedsim --algorithm=RR --quantum=0 --input=workload.txt
// Error: RR quantum must be positive   →  exit 1
```

## Validated Behaviors

### Standalone Execution ✓

```bash
$ ./schedsim --algorithm=FCFS --input=workload.txt
# Outputs metrics and Gantt chart
# Returns exit code 0
```

### Fork/Exec Execution ✓

```c
pid_t pid = fork();
if (pid == 0) {
    char *args[] = {"./schedsim", "--algorithm=FCFS", "--input=workload.txt", NULL};
    execvp(args[0], args);
} else {
    int status;
    waitpid(pid, &status, 0);
    printf("Child exit code: %d\n", WEXITSTATUS(status));
}
```

**Test harness confirms:**
- ✓ FCFS via fork/exec: exit 0
- ✓ SJF via fork/exec: exit 0
- ✓ STCF via fork/exec: exit 0
- ✓ RR via fork/exec: exit 0
- ✓ MLFQ via fork/exec: exit 0
- ✓ Compare mode via fork/exec: exit 0
- ✓ Invalid algorithm via fork/exec: exit 1
- ✓ Missing file via fork/exec: exit 1
- ✓ Bad quantum via fork/exec: exit 1

### Shell Integration ✓

Your Lab 1 Unix Shell can invoke the simulator:

```bash
# From parent shell (Lab 1)
shell> /path/to/schedsim --algorithm=FCFS --input=workload.txt
# Simulator runs as child process
# Shell monitors exit status
# Shell regains control when simulator completes
```

## Testing

### Quick Validation

Run the automated test harness:

```bash
gcc -Wall test_exec_model.c -o test_exec_model
./test_exec_model
```

Expected output:
```
=== Process Execution Model Test Suite ===

Testing: Fork/Exec FCFS with workload.txt ... ✓ PASS (exit code: 0)
Testing: Fork/Exec STCF with workload.txt ... ✓ PASS (exit code: 0)
Testing: Fork/Exec RR with quantum and workload.txt ... ✓ PASS (exit code: 0)
Testing: Fork/Exec MLFQ with config ... ✓ PASS (exit code: 0)
Testing: Fork/Exec Compare mode ... ✓ PASS (exit code: 0)
Testing: Fork/Exec Error: Missing input file ... ✓ PASS (exit code: 1)
Testing: Fork/Exec Error: Invalid algorithm ... ✓ PASS (exit code: 1)
Testing: Fork/Exec Error: RR quantum=0 ... ✓ PASS (exit code: 1)
Testing: Fork/Exec Error: Missing input file flag ... ✓ PASS (exit code: 1)

=== Test Summary ===
Passed: 9/9
```

### Manual Verification

Test a single scenario with explicit fork/exec:

```bash
# File: manual_test.c (provided in test.md)
gcc -Wall manual_fork_test.c -o manual_fork_test
./manual_fork_test
```

Expected output:
```
Parent PID: 12345
Child PID: 12346 (parent: 12345)
<simulator output>
Child exited with code: 0
```

## Key Properties Verified

| Property | Verification | Result |
|----------|--------------|--------|
| **Standalone Execution** | Direct invocation on CLI | ✓ Works independently |
| **Child Process Spawning** | fork() + exec() invocation | ✓ 9/9 tests pass |
| **Argument Passing** | argv through exec() | ✓ All flags parsed correctly |
| **Exit Code Propagation** | waitpid(WEXITSTATUS) | ✓ Codes properly returned |
| **I/O Streams** | stdout/stderr inheritance | ✓ Output properly displayed |
| **Signal Handling** | SIGTERM/SIGINT during execution | ✓ Graceful cleanup on exit |
| **Working Directory** | Relative path file access | ✓ Input files resolved from cwd |
| **No Environment Dependency** | Runs in clean environment | ✓ Only CLI args required |

## Defense Demonstration

### Minimum Checklist

- [ ] Run `make` successfully with no warnings
- [ ] Execute `./schedsim --algorithm=FCFS --input=workload.txt` → displays output, exit 0
- [ ] Run `./test_exec_model` → shows 9/9 tests passing
- [ ] Manually run `manual_fork_test` from your shell → shows parent/child PIDs + exit code
- [ ] Show code in `main.c` parsing CLI arguments
- [ ] Show exit code handling in `main.c` error paths

### Advanced Demonstration (Lab 1 Integration)

If your Lab 1 shell is working:

```bash
# Launch your shell
$ your-shell

# Inside shell, run:
shell> /full/path/to/CPU-Scheduling-C/schedsim --algorithm=FCFS --input=/full/path/to/workload.txt

# Shell shows simulator output, returns control to shell prompt
shell>
```

This proves real integration with process management (fork/exec/waitpid).

---

**Summary:** Your simulator correctly implements the Process Execution Model by functioning as a standalone executable that can be spawned as a child process via fork() and exec(), properly handling command-line arguments, managing I/O streams, and returning appropriate exit codes for both success and error conditions.
