#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

#define TEST_PASS "\033[32m✓ PASS\033[0m"
#define TEST_FAIL "\033[31m✗ FAIL\033[0m"

typedef struct {
    const char *test_name;
    const char **argv;
    int expected_exit_code;
    int capture_output;  // 1 = capture stdout, 0 = display
} TestCase;

int run_test(const TestCase *test) {
    printf("Testing: %s ... ", test->test_name);
    fflush(stdout);
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return 0;
    }
    
    if (pid == 0) {
        // Child process
        if (test->capture_output) {
            // Redirect stdout to /dev/null to suppress output
            int null_fd = open("/dev/null", O_WRONLY);
            if (null_fd >= 0) {
                dup2(null_fd, STDOUT_FILENO);
                close(null_fd);
            }
        }
        
        // Execute simulator
        execvp(test->argv[0], (char * const *)test->argv);
        perror("exec failed");
        exit(1);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            int actual_exit = WEXITSTATUS(status);
            if (actual_exit == test->expected_exit_code) {
                printf("%s (exit code: %d)\n", TEST_PASS, actual_exit);
                return 1;
            } else {
                printf("%s (expected: %d, got: %d)\n", TEST_FAIL, 
                       test->expected_exit_code, actual_exit);
                return 0;
            }
        } else if (WIFSIGNALED(status)) {
            printf("%s (terminated by signal %d)\n", TEST_FAIL, WTERMSIG(status));
            return 0;
        }
    }
    
    return 0;
}

int main() {
    printf("\n=== Process Execution Model Test Suite ===\n\n");
    
    // Define test cases
    TestCase tests[] = {
        // Test 1: Basic fork/exec with FCFS
        {
            "Fork/Exec FCFS with workload.txt",
            (const char *[]){"./schedsim", "--algorithm=FCFS", "--input=workload.txt", NULL},
            0, 1
        },
        
        // Test 2: STCF via fork/exec
        {
            "Fork/Exec STCF with workload.txt",
            (const char *[]){"./schedsim", "--algorithm=STCF", "--input=workload.txt", NULL},
            0, 1
        },
        
        // Test 3: RR with quantum via fork/exec
        {
            "Fork/Exec RR with quantum and workload.txt",
            (const char *[]){"./schedsim", "--algorithm=RR", "--quantum=50", "--input=workload.txt", NULL},
            0, 1
        },
        
        // Test 4: MLFQ with config via fork/exec
        {
            "Fork/Exec MLFQ with config",
            (const char *[]){"./schedsim", "--algorithm=MLFQ", "--mlfq-config=mlfq_config.txt", "--input=workload.txt", NULL},
            0, 1
        },
        
        // Test 5: Compare mode via fork/exec
        {
            "Fork/Exec Compare mode",
            (const char *[]){"./schedsim", "--compare", "--input=workload.txt", NULL},
            0, 1
        },
        
        // Test 6: Error handling - missing input file
        {
            "Fork/Exec Error: Missing input file",
            (const char *[]){"./schedsim", "--algorithm=FCFS", "--input=nonexistent.txt", NULL},
            1, 1
        },
        
        // Test 7: Error handling - invalid algorithm
        {
            "Fork/Exec Error: Invalid algorithm",
            (const char *[]){"./schedsim", "--algorithm=INVALID", "--input=workload.txt", NULL},
            1, 1
        },
        
        // Test 8: Error handling - invalid quantum (0)
        {
            "Fork/Exec Error: RR quantum=0",
            (const char *[]){"./schedsim", "--algorithm=RR", "--quantum=0", "--input=workload.txt", NULL},
            1, 1
        },
        
        // Test 9: Error handling - missing required flag
        {
            "Fork/Exec Error: Missing input file flag",
            (const char *[]){"./schedsim", "--algorithm=FCFS", NULL},
            1, 1
        },
    };
    
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;
    
    // Run all tests
    for (int i = 0; i < num_tests; i++) {
        if (run_test(&tests[i])) {
            passed++;
        }
    }
    
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d/%d\n", passed, num_tests);
    
    if (passed == num_tests) {
        printf("All tests passed! ✓\n");
        return 0;
    } else {
        printf("Some tests failed.\n");
        return 1;
    }
}
