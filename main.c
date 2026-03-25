#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process.h"
#include "scheduler.h"
#include "trace.h"

typedef struct {
    char algorithm[16];
    const char *input_path;
    const char *mlfq_config_path;
    int quantum;
    int compare;
} CliOptions;

static int equals_ignore_case(const char *a, const char *b)
{
    while (*a && *b)
    {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static void reset_processes(Process p[], int n)
{
    for (int i = 0; i < n; i++)
    {
        p[i].remaining_time = p[i].burst_time;
        p[i].start_time = -1;
        p[i].finish_time = -1;
        p[i].response_time = 0;
        p[i].turnaround_time = 0;
        p[i].waiting_time = 0;
        p[i].started = 0;
        p[i].priority = 0;
        p[i].time_in_queue = 0;
    }
}

static int is_blank_or_comment(const char *line)
{
    while (*line)
    {
        if (*line == '#')
            return 1;
        if (!isspace((unsigned char)*line))
            return 0;
        line++;
    }
    return 1;
}

static int load_workload(const char *path, Process **out_processes, int *out_n)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: cannot open input file '%s'.\n", path);
        return 0;
    }

    int capacity = 16;
    int count = 0;
    Process *processes = (Process *)malloc((size_t)capacity * sizeof(Process));
    if (processes == NULL)
    {
        fclose(fp);
        fprintf(stderr, "Error: memory allocation failed.\n");
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (is_blank_or_comment(line))
            continue;

        char pid[16];
        int arrival_time;
        int burst_time;

        if (sscanf(line, "%15s %d %d", pid, &arrival_time, &burst_time) != 3)
        {
            fprintf(stderr, "Error: invalid workload line: %s", line);
            free(processes);
            fclose(fp);
            return 0;
        }

        if (count == capacity)
        {
            capacity *= 2;
            Process *resized = (Process *)realloc(processes, (size_t)capacity * sizeof(Process));
            if (resized == NULL)
            {
                fprintf(stderr, "Error: memory allocation failed.\n");
                free(processes);
                fclose(fp);
                return 0;
            }
            processes = resized;
        }

        strncpy(processes[count].pid, pid, sizeof(processes[count].pid) - 1);
        processes[count].pid[sizeof(processes[count].pid) - 1] = '\0';
        processes[count].arrival_time = arrival_time;
        processes[count].burst_time = burst_time;

        count++;
    }

    fclose(fp);

    if (count == 0)
    {
        fprintf(stderr, "Error: no processes found in input file '%s'.\n", path);
        free(processes);
        return 0;
    }

    reset_processes(processes, count);

    *out_processes = processes;
    *out_n = count;
    return 1;
}

static int parse_mlfq_config(const char *path, MLFQConfig *config)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: cannot open MLFQ config file '%s'.\n", path);
        return 0;
    }

    config->levels = 0;
    config->boost_period = 200;
    for (int i = 0; i < MLFQ_MAX_LEVELS; i++)
    {
        config->quantum[i] = 0;
        config->allotment[i] = 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (is_blank_or_comment(line))
            continue;

        if (strncmp(line, "BOOST_PERIOD", 12) == 0)
        {
            int value;
            if (sscanf(line, "BOOST_PERIOD %d", &value) != 1 || value <= 0)
            {
                fprintf(stderr, "Error: invalid BOOST_PERIOD entry: %s", line);
                fclose(fp);
                return 0;
            }
            config->boost_period = value;
            continue;
        }

        int qid;
        int quantum;
        int allotment;
        if (sscanf(line, "Q%d %d %d", &qid, &quantum, &allotment) == 3)
        {
            if (qid < 0 || qid >= MLFQ_MAX_LEVELS)
            {
                fprintf(stderr, "Error: queue id out of range in line: %s", line);
                fclose(fp);
                return 0;
            }

            if (quantum == 0 || quantum < -1)
            {
                fprintf(stderr, "Error: invalid quantum in line: %s", line);
                fclose(fp);
                return 0;
            }

            if (allotment == 0 || allotment < -1)
            {
                fprintf(stderr, "Error: invalid allotment in line: %s", line);
                fclose(fp);
                return 0;
            }

            if ((quantum == -1 && allotment != -1) || (quantum != -1 && allotment == -1))
            {
                fprintf(stderr, "Error: FCFS queue must use -1 -1 and RR queues must use positive values: %s", line);
                fclose(fp);
                return 0;
            }

            config->quantum[qid] = quantum;
            config->allotment[qid] = allotment;
            if (qid + 1 > config->levels)
                config->levels = qid + 1;
            continue;
        }

        fprintf(stderr, "Error: invalid MLFQ config line: %s", line);
        fclose(fp);
        return 0;
    }

    fclose(fp);

    if (config->levels < 3)
    {
        fprintf(stderr, "Error: MLFQ requires at least 3 queue levels.\n");
        return 0;
    }

    for (int lvl = 0; lvl < config->levels; lvl++)
    {
        if (config->quantum[lvl] == 0 || config->allotment[lvl] == 0)
        {
            fprintf(stderr, "Error: missing queue definition for Q%d.\n", lvl);
            return 0;
        }
    }

    int saw_fcfs = 0;
    for (int lvl = 0; lvl < config->levels; lvl++)
    {
        if (config->quantum[lvl] == -1)
        {
            saw_fcfs = 1;
        }
        else if (saw_fcfs)
        {
            fprintf(stderr, "Error: RR queue cannot appear below an FCFS queue in MLFQ levels.\n");
            return 0;
        }
    }

    if (config->boost_period <= 0)
    {
        fprintf(stderr, "Error: boost_period must be positive.\n");
        return 0;
    }

    return 1;
}

static void print_usage(const char *prog_name)
{
    printf("Usage:\n");
    printf("  %s --algorithm=FCFS|SJF|STCF|RR|MLFQ --input=workload.txt [options]\n", prog_name);
    printf("  %s --compare --input=workload.txt [options]\n", prog_name);
    printf("\nOptions:\n");
    printf("  --quantum=<q>           Time quantum for RR (default: 30)\n");
    printf("  --mlfq-config=<file>    MLFQ config file path\n");
    printf("  --compare               Run all algorithms on the same workload\n");
}

static int parse_cli(int argc, char *argv[], CliOptions *options)
{
    strcpy(options->algorithm, "FCFS");
    options->input_path = NULL;
    options->mlfq_config_path = NULL;
    options->quantum = 30;
    options->compare = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strncmp(argv[i], "--algorithm=", 12) == 0)
        {
            strncpy(options->algorithm, argv[i] + 12, sizeof(options->algorithm) - 1);
            options->algorithm[sizeof(options->algorithm) - 1] = '\0';
        }
        else if (strncmp(argv[i], "--input=", 8) == 0)
        {
            options->input_path = argv[i] + 8;
        }
        else if (strncmp(argv[i], "--quantum=", 10) == 0)
        {
            options->quantum = atoi(argv[i] + 10);
        }
        else if (strncmp(argv[i], "--mlfq-config=", 14) == 0)
        {
            options->mlfq_config_path = argv[i] + 14;
        }
        else if (equals_ignore_case(argv[i], "--compare"))
        {
            options->compare = 1;
        }
        else if (equals_ignore_case(argv[i], "--help"))
        {
            print_usage(argv[0]);
            return 0;
        }
        else
        {
            fprintf(stderr, "Error: unknown argument '%s'.\n", argv[i]);
            print_usage(argv[0]);
            return 0;
        }
    }

    if (options->input_path == NULL)
    {
        fprintf(stderr, "Error: --input is required.\n");
        print_usage(argv[0]);
        return 0;
    }

    if (equals_ignore_case(options->algorithm, "RR") && options->quantum <= 0)
    {
        fprintf(stderr, "Error: RR quantum must be positive.\n");
        return 0;
    }

    if (!options->compare && equals_ignore_case(options->algorithm, "MLFQ") && options->mlfq_config_path == NULL)
    {
        fprintf(stderr, "Error: --mlfq-config is required for MLFQ.\n");
        return 0;
    }

    return 1;
}

static void fill_default_mlfq_config(MLFQConfig *config)
{
    config->levels = 3;
    config->quantum[0] = 10;
    config->allotment[0] = 50;
    config->quantum[1] = 30;
    config->allotment[1] = 150;
    config->quantum[2] = -1;
    config->allotment[2] = -1;
    config->boost_period = 200;
}

static void print_compare_table(const char *input_path,
                                int rr_quantum,
                                const MetricsSummary rows[5])
{
    printf("\n=== Algorithm Comparison for %s ===\n\n", input_path);
    printf("Algorithm | Avg TT | Avg WT | Avg RT | Context Switches\n");
    printf("----------|--------|--------|--------|------------------\n");
    printf("FCFS      | %6.1f | %6.1f | %6.1f | %16d\n",
           rows[0].avg_turnaround,
           rows[0].avg_waiting,
           rows[0].avg_response,
           rows[0].context_switches);
    printf("SJF       | %6.1f | %6.1f | %6.1f | %16d\n",
           rows[1].avg_turnaround,
           rows[1].avg_waiting,
           rows[1].avg_response,
           rows[1].context_switches);
    printf("STCF      | %6.1f | %6.1f | %6.1f | %16d\n",
           rows[2].avg_turnaround,
           rows[2].avg_waiting,
           rows[2].avg_response,
           rows[2].context_switches);
    printf("RR (q=%d) | %6.1f | %6.1f | %6.1f | %16d\n",
           rr_quantum,
           rows[3].avg_turnaround,
           rows[3].avg_waiting,
           rows[3].avg_response,
           rows[3].context_switches);
    printf("MLFQ      | %6.1f | %6.1f | %6.1f | %16d\n",
           rows[4].avg_turnaround,
           rows[4].avg_waiting,
           rows[4].avg_response,
           rows[4].context_switches);
}

int main(int argc, char *argv[])
{
    CliOptions options;
    if (!parse_cli(argc, argv, &options))
        return 1;

    Process *processes = NULL;
    int n = 0;

    if (!load_workload(options.input_path, &processes, &n))
        return 1;

    SchedulerState state;
    state.processes = processes;
    state.num_processes = n;
    state.current_time = 0;

    if (options.compare)
    {
        MLFQConfig config;
        if (options.mlfq_config_path != NULL)
        {
            if (!parse_mlfq_config(options.mlfq_config_path, &config))
            {
                free(processes);
                return 1;
            }
        }
        else
        {
            fill_default_mlfq_config(&config);
        }

        MetricsSummary rows[5];
        trace_set_quiet(1);

        reset_processes(processes, n);
        state.current_time = 0;
        if (schedule_fcfs(&state) != 0)
        {
            trace_set_quiet(0);
            free(processes);
            return 1;
        }
        compute_metrics_summary(processes, n, &rows[0]);

        reset_processes(processes, n);
        state.current_time = 0;
        if (schedule_sjf(&state) != 0)
        {
            trace_set_quiet(0);
            free(processes);
            return 1;
        }
        compute_metrics_summary(processes, n, &rows[1]);

        reset_processes(processes, n);
        state.current_time = 0;
        if (schedule_stcf(&state) != 0)
        {
            trace_set_quiet(0);
            free(processes);
            return 1;
        }
        compute_metrics_summary(processes, n, &rows[2]);

        reset_processes(processes, n);
        state.current_time = 0;
        if (schedule_rr(&state, options.quantum) != 0)
        {
            trace_set_quiet(0);
            free(processes);
            return 1;
        }
        compute_metrics_summary(processes, n, &rows[3]);

        reset_processes(processes, n);
        state.current_time = 0;
        if (schedule_mlfq(&state, &config) != 0)
        {
            trace_set_quiet(0);
            free(processes);
            return 1;
        }
        compute_metrics_summary(processes, n, &rows[4]);

        trace_set_quiet(0);
        print_compare_table(options.input_path, options.quantum, rows);

        trace_free();
        free(processes);
        return 0;
    }

    if (equals_ignore_case(options.algorithm, "FCFS"))
    {
        printf("\nRunning FCFS\n");
        if (schedule_fcfs(&state) != 0)
        {
            free(processes);
            return 1;
        }
        print_results(processes, n);
        trace_free();
    }
    else if (equals_ignore_case(options.algorithm, "SJF"))
    {
        printf("\nRunning SJF\n");
        if (schedule_sjf(&state) != 0)
        {
            free(processes);
            return 1;
        }
        print_results(processes, n);
        trace_free();
    }
    else if (equals_ignore_case(options.algorithm, "STCF"))
    {
        printf("\nRunning STCF\n");
        if (schedule_stcf(&state) != 0)
        {
            free(processes);
            return 1;
        }
        print_results(processes, n);
        trace_free();
    }
    else if (equals_ignore_case(options.algorithm, "RR"))
    {
        printf("\nRunning RR\n");
        printf("Using time quantum q=%d\n", options.quantum);
        if (schedule_rr(&state, options.quantum) != 0)
        {
            free(processes);
            return 1;
        }
        print_results(processes, n);
        trace_free();
    }
    else if (equals_ignore_case(options.algorithm, "MLFQ"))
    {
        MLFQConfig config;
        if (!parse_mlfq_config(options.mlfq_config_path, &config))
        {
            free(processes);
            return 1;
        }

        printf("\nRunning MLFQ\n");
        if (schedule_mlfq(&state, &config) != 0)
        {
            free(processes);
            return 1;
        }
        print_results(processes, n);
        trace_free();
    }
    else
    {
        fprintf(stderr, "Error: unknown algorithm '%s'.\n", options.algorithm);
        free(processes);
        return 1;
    }

    free(processes);
    return 0;
}
