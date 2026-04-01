// main.c is the entry point of the CPU scheduling simulator. It handles command-line argument parsing, process loading from various sources (file or inline), MLFQ configuration parsing, and orchestrates the execution of the selected scheduling algorithm(s). It also manages the overall flow of the program, including error handling and output formatting.

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/process.h"
#include "../include/scheduler.h"
#include "../include/utils.h"
#include "gantt.h"

// CliOptions holds the parsed command-line options for configuring the scheduling simulation
typedef struct {
    char algorithm[16];
    const char *input_path;
    const char *inline_workload;
    const char *inline_processes;
    const char *mlfq_config_path;
    int quantum;
    int compare;
} CliOptions;

// parse_args processes the command-line arguments and populates a CliOptions structure. It returns 1 on success and 0 on failure (e.g., invalid arguments). It also prints usage information if needed.
/* Generic inline process parser supporting different delimiters */
static int load_processes_inline_generic(const char *spec, const char *item_delim,
                                         const char *field_delim, const char *option_name,
                                         Process **out_processes, int *out_n)
{
    // validate input spec string
    if (spec == NULL || *spec == '\0')
    {
        fprintf(stderr, "Error: --%s must not be empty.\n", option_name);
        return 0;
    }

    // make a mutable copy of the spec string for tokenization, since strtok modifies the string. We will free this buffer before returning.
    char *buf = (char *)malloc(strlen(spec) + 1);
    if (buf == NULL) {
        fprintf(stderr, "Error: memory allocation failed.\n");
        return 0;
    }
    strcpy(buf, spec);

    // tokenize the spec string into items using the item_delim, then for each item, split it into fields using field_delim to extract pid, arrival_time, and burst_time. We will store the processes in a dynamically growing array. We will also trim spaces from the fields and validate them. If any entry is invalid, we will print an error and return failure.
    int capacity = 16, count = 0;
    Process *processes = (Process *)malloc((size_t)capacity * sizeof(Process));
    if (processes == NULL) {
        fprintf(stderr, "Error: memory allocation failed.\n");
        free(buf);
        return 0;
    }

    // use strtok_r for thread-safe tokenization, and trim spaces from each item. Then split each item into fields and validate them. We expect exactly 3 fields: PID, ARRIVAL, and BURST, separated by field_delim. If the format is invalid, print an error and return failure.
    char *saveptr = NULL;
    char *entry = strtok_r(buf, item_delim, &saveptr);
    // for each entry, trim spaces and split into fields
    while (entry != NULL) {
        char *item = trim_spaces(entry);
        // if the trimmed item is not empty, process it; otherwise, skip it
        if (*item != '\0') {
            char *c1 = strchr(item, *field_delim);
            char *c2 = c1 ? strchr(c1 + 1, *field_delim) : NULL;
            char *c3 = c2 ? strchr(c2 + 1, *field_delim) : NULL;

            // validate that we have exactly 3 fields (2 delimiters) and that the first field (PID) is not empty. If not, print an error and return failure.
            if (c1 == NULL || c2 == NULL || c3 != NULL) {
                fprintf(stderr, "Error: invalid --%s entry '%s'. Expected PID%cARRIVAL%cBURST\n",
                        option_name, item, *field_delim, *field_delim);
                free(processes);
                free(buf);
                return 0;
            }

            // split the item into pid, arrival_time, and burst_time fields by replacing the delimiters with null terminators. Then trim spaces from each field and validate them. PID must not be empty, and arrival_time and burst_time must be valid integers. If any validation fails, print an error and return failure.
            *c1 = *c2 = '\0';
            char *pid = trim_spaces(item);
            int arrival_time, burst_time;
            if (*pid == '\0' || !parse_int_strict(trim_spaces(c1 + 1), &arrival_time) ||
                !parse_int_strict(trim_spaces(c2 + 1), &burst_time)) {
                fprintf(stderr, "Error: invalid --%s entry.\n", option_name);
                free(processes);
                free(buf);
                return 0;
            }

            // if we have reached capacity, double the size of the processes array. If realloc fails, print an error and return failure.
            if (count == capacity) {
                capacity *= 2;
                Process *resized = (Process *)realloc(processes, (size_t)capacity * sizeof(Process));
                // check for allocation failure
                if (resized == NULL) {
                    fprintf(stderr, "Error: memory allocation failed.\n");
                    free(processes);
                    free(buf);
                    return 0;
                }
                processes = resized;
            }

            // store the process information in the array, ensuring to null-terminate the PID and validate that arrival_time and burst_time are non-negative. If validation fails, print an error and return failure.
            strncpy(processes[count].pid, pid, sizeof(processes[count].pid) - 1);
            processes[count].pid[sizeof(processes[count].pid) - 1] = '\0';
            processes[count].arrival_time = arrival_time;
            processes[count].burst_time = burst_time;
            count++;
        }
        entry = strtok_r(NULL, item_delim, &saveptr);
    }

    // free the mutable buffer we used for tokenization
    free(buf);

    // if we found no valid processes, print an error and return failure
    if (count == 0) {
        fprintf(stderr, "Error: no processes found in --%s.\n", option_name);
        free(processes);
        return 0;
    }

    // reset all processes to initialize their metrics and state, then set the output parameters and return success
    process_reset_all(processes, count);
    *out_processes = processes;
    *out_n = count;
    return 1;
}

// load_workload_inline is a wrapper around load_processes_inline_generic that uses ';' as the item delimiter and ',' as the field delimiter for parsing an inline workload specification from the command line. It returns 1 on success and 0 on failure.
static int load_workload_inline(const char *spec, Process **out_processes, int *out_n)
{
    return load_processes_inline_generic(spec, ";", ",", "workload", out_processes, out_n);
}

// load_processes_inline is a wrapper around load_processes_inline_generic that uses ',' as the item delimiter and ':' as the field delimiter for parsing an inline processes specification from the command line. It returns 1 on success and 0 on failure.
static int load_processes_inline(const char *spec, Process **out_processes, int *out_n)
{
    return load_processes_inline_generic(spec, ",", ":", "processes", out_processes, out_n);
}

// load_workload reads a workload specification from a file, where each line contains a process definition in the format "PID ARRIVAL BURST". It ignores blank lines and comments (lines starting with '#'). It returns 1 on success and 0 on failure, and outputs the loaded processes and their count through the out parameters.
static int load_workload(const char *path, Process **out_processes, int *out_n)
{
    FILE *fp = fopen(path, "r");
    // check if file was opened successfully
    if (fp == NULL)
    {
        fprintf(stderr, "Error: cannot open input file '%s'.\n", path);
        return 0;
    }

    // read lines from the file and parse process definitions, ignoring blank lines and comments. We will store the processes in a dynamically growing array. Each valid line should contain a PID, arrival time, and burst time. If any line is invalid, we will print an error and return failure. After loading, we will reset all processes to initialize their metrics and state, then set the output parameters and return success.
    int capacity = 16;
    int count = 0;
    Process *processes = (Process *)malloc((size_t)capacity * sizeof(Process));
    if (processes == NULL)
    {
        fclose(fp);
        fprintf(stderr, "Error: memory allocation failed.\n");
        return 0;
    }

    // read each line from the file, trim it, and if it's not blank or a comment, parse it into PID, arrival_time, and burst_time. We will validate the format and values, and if valid, store it in the processes array. If we reach capacity, we will double the size of the array. If any line is invalid, we will print an error and return failure.
    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        // trim the line and check if it's blank or a comment; if so, skip it
        if (is_blank_or_comment(line))
            continue;

        char pid[16];
        int arrival_time;
        int burst_time;

        // parse the line into pid, arrival_time, and burst_time. We expect exactly 3 fields separated by spaces. If the format is invalid, print an error and return failure.
        if (sscanf(line, "%15s %d %d", pid, &arrival_time, &burst_time) != 3)
        {
            fprintf(stderr, "Error: invalid workload line: %s", line);
            free(processes);
            fclose(fp);
            return 0;
        }

        // if we have reached capacity, double the size of the processes array. If realloc fails, print an error and return failure.
        if (count == capacity)
        {
            capacity *= 2;
            Process *resized = (Process *)realloc(processes, (size_t)capacity * sizeof(Process));
            // check for allocation failure
            if (resized == NULL)
            {
                fprintf(stderr, "Error: memory allocation failed.\n");
                free(processes);
                fclose(fp);
                return 0;
            }
            processes = resized;
        }

        // store the process information in the array, ensuring to null-terminate the PID and validate that arrival_time and burst_time are non-negative. If validation fails, print an error and return failure.
        strncpy(processes[count].pid, pid, sizeof(processes[count].pid) - 1);
        processes[count].pid[sizeof(processes[count].pid) - 1] = '\0';
        processes[count].arrival_time = arrival_time;
        processes[count].burst_time = burst_time;

        count++;
    }

    // close the file after reading
    fclose(fp);

    // if we found no valid processes, print an error and return failure
    if (count == 0)
    {
        fprintf(stderr, "Error: no processes found in input file '%s'.\n", path);
        free(processes);
        return 0;
    }

    // reset all processes to initialize their metrics and state, then set the output parameters and return success
    process_reset_all(processes, count);

    // set output parameters and return success
    *out_processes = processes;
    *out_n = count;
    return 1;
}

// print_usage displays the usage information for the program, including the available command-line options and their descriptions. It takes the program name as an argument to customize the usage message.
static void print_usage(const char *prog_name)
{
    printf("Usage:\n");
    printf("  %s --algorithm=FCFS|SJF|STCF|RR|MLFQ --input=workload.txt [options]\n", prog_name);
    printf("  %s --algorithm=FCFS|SJF|STCF|RR|MLFQ --workload=\"P1,0,5;P2,1,3\" [options]\n", prog_name);
    printf("  %s --algorithm=FCFS|SJF|STCF|RR|MLFQ --processes=\"P1:0:5,P2:1:3\" [options]\n", prog_name);
    printf("  %s --compare --input=workload.txt [options]\n", prog_name);
    printf("  %s --compare --workload=\"P1,0,5;P2,1,3\" [options]\n", prog_name);
    printf("  %s --compare --processes=\"P1:0:5,P2:1:3\" [options]\n", prog_name);
    printf("\nOptions:\n");
    printf("  --quantum=<q>           Time quantum for RR (default: 30)\n");
    printf("  --mlfq-config=<file>    MLFQ config file path\n");
    printf("  --compare               Run all algorithms on the same workload\n");
    printf("  --workload=<spec>       Inline workload: PID,ARRIVAL,BURST;...\n");
    printf("  --processes=<spec>      Inline processes: PID:ARRIVAL:BURST,...\n");
}

// parse_cli processes the command-line arguments and populates a CliOptions structure with the parsed values. It validates the arguments and returns 1 on success or 0 on failure, printing error messages and usage information as needed.
static int parse_cli(int argc, char *argv[], CliOptions *options)
{
    strncpy(options->algorithm, "FCFS", sizeof(options->algorithm) - 1);
    options->algorithm[sizeof(options->algorithm) - 1] = '\0';
    options->input_path = NULL;
    options->inline_workload = NULL;
    options->inline_processes = NULL;
    options->mlfq_config_path = NULL;
    options->quantum = 30;
    options->compare = 0;

    for (int i = 1; i < argc; i++)
    {
        // check if the argument starts with a known prefix for each option, and if so, parse the value and store it in the options structure. We will also validate the values as we parse them. If an argument is unknown or if any validation fails, we will print an error message and usage information, and return failure.
        if (strncmp(argv[i], "--algorithm=", 12) == 0)
        {
            // for the algorithm option, we will extract the algorithm name after the prefix, validate that it is not too long for our options structure, and store it. We will also validate that the algorithm name is one of the supported algorithms (FCFS, SJF, STCF, RR, MLFQ). If validation fails, print an error and return failure.
            const char *alg = argv[i] + 12;
            if (strlen(alg) >= sizeof(options->algorithm))
            {
                fprintf(stderr, "Error: algorithm name too long: '%s'.\n", alg);
                return 0;
            }

            strncpy(options->algorithm, alg, sizeof(options->algorithm) - 1);
            options->algorithm[sizeof(options->algorithm) - 1] = '\0';
        }
        // for the input option, we will extract the file path after the prefix and store it. We will not validate the file path here, but we will check that it is not empty. If validation fails, print an error and return failure.
        else if (strncmp(argv[i], "--input=", 8) == 0)
        {
            options->input_path = argv[i] + 8;
        }
        // for the inline workload option, we will extract the specification string after the prefix and store it. We will validate that it is not empty. If validation fails, print an error and return failure.
        else if (strncmp(argv[i], "--workload=", 11) == 0)
        {
            options->inline_workload = argv[i] + 11;
        }
        // for the inline processes option, we will extract the specification string after the prefix and store it. We will validate that it is not empty. If validation fails, print an error and return failure.
        else if (strncmp(argv[i], "--processes=", 12) == 0)
        {
            options->inline_processes = argv[i] + 12;
        }
        // for the quantum option, we will extract the value after the prefix, parse it as an integer, and store it. We will validate that it is a positive integer. If validation fails, print an error and return failure.
        else if (strncmp(argv[i], "--quantum=", 10) == 0)
        {
            options->quantum = atoi(argv[i] + 10);
        }
        // for the MLFQ config option, we will extract the file path after the prefix and store it. We will validate that it is not empty. If validation fails, print an error and return failure.
        else if (strncmp(argv[i], "--mlfq-config=", 14) == 0)
        {
            options->mlfq_config_path = argv[i] + 14;
        }
        // for the compare option, we will set the compare flag to 1. This option does not take a value, so we just check for an exact match. If the argument is unknown, print an error and return failure.
        else if (equals_ignore_case(argv[i], "--compare"))
        {
            options->compare = 1;
        }
        // for the help option, we will print the usage information and return success. This option does not take a value, so we just check for an exact match.
        else if (equals_ignore_case(argv[i], "--help"))
        {
            print_usage(argv[0]);
            return 0;
        }
        // if the argument does not match any known option, print an error message and usage information, and return failure.
        else
        {
            fprintf(stderr, "Error: unknown argument '%s'.\n", argv[i]);
            print_usage(argv[0]);
            return 0;
        }
    }

    // after parsing all arguments, we will validate that exactly one workload source is provided (either --input, --workload, or --processes), that the algorithm name is valid, that the quantum value is valid if RR is selected, and that the MLFQ config file is provided if MLFQ is selected without compare mode. If any validation fails, print an error message and usage information, and return failure. If all validation passes, return success.
    int sources = 0;
    // count how many workload sources are provided among --input, --workload, and --processes. We will validate that exactly one is provided. If validation fails, print an error and return failure.
    if (options->input_path != NULL)
        sources++;
    // if the inline workload option is provided, count it as a source
    if (options->inline_workload != NULL)
        sources++;
    // if the inline processes option is provided, count it as a source
    if (options->inline_processes != NULL)
        sources++;

    // validate that exactly one workload source is provided. If none are provided, print an error and usage information, and return failure. If more than one are provided, print an error and usage information, and return failure.
    if (sources == 0)
    {
        fprintf(stderr, "Error: provide one workload source: --input, --workload, or --processes.\n");
        print_usage(argv[0]);
        return 0;
    }

    // validate that the algorithm name is valid. If not, print an error and usage information, and return failure.
    if (sources > 1)
    {
        fprintf(stderr, "Error: use only one workload source: --input, --workload, or --processes.\n");
        print_usage(argv[0]);
        return 0;
    }

    // validate that the algorithm name is one of the supported algorithms (FCFS, SJF, STCF, RR, MLFQ). If validation fails, print an error and usage information, and return failure.
    if (equals_ignore_case(options->algorithm, "RR") && options->quantum <= 0)
    {
        fprintf(stderr, "Error: RR quantum must be positive.\n");
        return 0;
    }

    // if MLFQ is selected without compare mode, validate that the MLFQ config file is provided. If validation fails, print an error and usage information, and return failure.
    if (!options->compare && equals_ignore_case(options->algorithm, "MLFQ") && options->mlfq_config_path == NULL)
    {
        fprintf(stderr, "Error: --mlfq-config is required for MLFQ.\n");
        return 0;
    }

    return 1;
}

// main is the entry point of the program. It parses command-line arguments, loads the workload, initializes the scheduler state, and runs the specified scheduling algorithm(s) while printing the results. It also handles error cases and resource cleanup.
int main(int argc, char *argv[])
{
    // parse command-line arguments into a CliOptions structure, and if parsing fails, return with an error code
    CliOptions options;
    if (!parse_cli(argc, argv, &options))
        return 1;

    // load the workload based on the provided options (input file, inline workload, or inline processes), and if loading fails, return with an error code. The loaded processes and their count will be stored in the processes pointer and n variable.
    Process *processes = NULL;
    int n = 0;

    if (options.input_path != NULL)
    {
        // if an input file path is provided, load the workload from the file. If loading fails, return with an error code.
        if (!load_workload(options.input_path, &processes, &n))
            return 1;
    }
    // if an inline processes specification is provided, load the processes from the specification. If loading fails, return with an error code.
    else if (options.inline_processes != NULL)
    {
        // if an inline processes specification is provided, load the processes from the specification. If loading fails, return with an error code.
        if (!load_processes_inline(options.inline_processes, &processes, &n))
            return 1;
    }
    // if an inline workload specification is provided, load the workload from the specification. If loading fails, return with an error code.
    else
    {
        // if an inline workload specification is provided, load the workload from the specification. If loading fails, return with an error code.
        if (!load_workload_inline(options.inline_workload, &processes, &n))
            return 1;
    }

    // initialize the scheduler state with the loaded processes, their count, and the trace function pointers
    SchedulerState state;
    state.processes = processes;
    state.num_processes = n;
    state.current_time = 0;
    state.trace_reset_fn = trace_reset;
    state.trace_add_segment_fn = trace_add_segment;
    state.trace_set_context_switches_fn = trace_set_context_switches;
    state.trace_is_quiet_fn = trace_is_quiet;

    // if an MLFQ config file is provided, parse the MLFQ configuration from the file; otherwise, fill the MLFQConfig structure with default values. If parsing fails, free resources and return with an error code.
    MLFQConfig mlfq_config;
    if (options.mlfq_config_path != NULL) {
        // if an MLFQ config file is provided, parse the MLFQ configuration from the file. If parsing fails, free resources and return with an error code.
        if (!parse_mlfq_config(options.mlfq_config_path, &mlfq_config)) {
            trace_free();
            free(processes);
            return 1;
        }
    }
    // if no MLFQ config file is provided, fill the MLFQConfig structure with default values for a 3-level MLFQ configuration. 
    else {
        fill_default_mlfq_config(&mlfq_config);
    }

    // if the compare flag is set, run all algorithms on the same workload and print a comparison table.
    if (options.compare) {
        const char *source = options.input_path != NULL ? options.input_path : "<inline-workload>";
        if (!run_compare_algorithms(&state, options.quantum, &mlfq_config, processes, n, source)) {
            free(processes);
            return 1;
        }
    } 
    // if the compare flag is not set, run only the specified algorithm on the workload and print the results. If running the algorithm fails, free resources and return with an error code.
    else {
        if (!run_single_algorithm(&state, options.algorithm, options.quantum, &mlfq_config, processes, n)) {
            trace_free();
            free(processes);
            return 1;
        }
    }  

    // free the processes array before exiting
    free(processes);
    return 0;
}
