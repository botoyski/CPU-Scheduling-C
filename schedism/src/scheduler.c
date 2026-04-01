#include <stdio.h>
#include <string.h>

#include "scheduler.h"
#include "gantt.h"
#include "utils.h"

void scheduler_get_trace_ops(const SchedulerState *state, SchedulerTraceOps *ops)
{
    ops->reset = state->trace_reset_fn ? state->trace_reset_fn : trace_reset;
    ops->add_segment = state->trace_add_segment_fn ? state->trace_add_segment_fn : trace_add_segment;
    ops->set_context_switches = state->trace_set_context_switches_fn ? state->trace_set_context_switches_fn : trace_set_context_switches;
    ops->is_quiet = state->trace_is_quiet_fn ? state->trace_is_quiet_fn : trace_is_quiet;
}

int scheduler_is_verbose(const SchedulerState *state)
{
    SchedulerTraceOps ops;
    scheduler_get_trace_ops(state, &ops);
    return state->verbose && !ops.is_quiet();
}

void scheduler_mark_process_started(Process *process, int time)
{
    process->start_time = time;
    process->response_time = time - process->arrival_time;
    process->started = 1;
}

void scheduler_mark_process_completed(Process *process, int finish_time)
{
    process->finish_time = finish_time;
    process->turnaround_time = process->finish_time - process->arrival_time;
    process->waiting_time = process->turnaround_time - process->burst_time;
}

int parse_mlfq_config(const char *path, MLFQConfig *config)
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

        char key[32];
        int value;
        char extra[32];
        int parsed = sscanf(line, "%31s %d %31s", key, &value, extra);
        if (parsed >= 1 && strcmp(key, "BOOST_PERIOD") == 0)
        {
            if (parsed != 2 || value <= 0)
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

void fill_default_mlfq_config(MLFQConfig *config)
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

static void print_compare_table(const char *input_path, int rr_quantum, const MetricsSummary rows[5])
{
    const char *names[] = {"FCFS", "SJF", "STCF", "RR", "MLFQ"};
    printf("\n=== Algorithm Comparison for %s ===\n\n", input_path);
    printf("Algorithm | Avg TT | Avg WT | Avg RT | Context Switches\n");
    printf("----------|--------|--------|--------|------------------\n");
    for (int i = 0; i < 5; i++)
    {
        if (i == 3)
            printf("%s (q=%d) | %6.1f | %6.1f | %6.1f | %16d\n", names[i], rr_quantum,
                   rows[i].avg_turnaround, rows[i].avg_waiting, rows[i].avg_response, rows[i].context_switches);
        else
            printf("%-9s | %6.1f | %6.1f | %6.1f | %16d\n", names[i],
                   rows[i].avg_turnaround, rows[i].avg_waiting, rows[i].avg_response, rows[i].context_switches);
    }
}

int run_algorithm(SchedulerState *state, const char *alg, int quantum, MLFQConfig *mlfq_config)
{
    if (equals_ignore_case(alg, "FCFS"))
        return schedule_fcfs(state);
    if (equals_ignore_case(alg, "SJF"))
        return schedule_sjf(state);
    if (equals_ignore_case(alg, "STCF"))
        return schedule_stcf(state);
    if (equals_ignore_case(alg, "RR"))
        return schedule_rr(state, quantum);
    if (equals_ignore_case(alg, "MLFQ"))
        return schedule_mlfq(state, mlfq_config);
    return 1;
}

int run_single_algorithm(SchedulerState *state, const char *alg, int quantum, MLFQConfig *mlfq_config,
                         Process *processes, int n)
{
    if (!equals_ignore_case(alg, "FCFS") && !equals_ignore_case(alg, "SJF") &&
        !equals_ignore_case(alg, "STCF") && !equals_ignore_case(alg, "RR") &&
        !equals_ignore_case(alg, "MLFQ"))
    {
        fprintf(stderr, "Error: unknown algorithm '%s'.\n", alg);
        return 0;
    }

    printf("\nRunning %s\n", alg);
    if (equals_ignore_case(alg, "RR"))
        printf("Using time quantum q=%d\n", quantum);

    if (run_algorithm(state, alg, quantum, mlfq_config) != 0)
        return 0;

    print_results(processes, n);
    trace_free();
    return 1;
}

int run_compare_algorithms(SchedulerState *state, int rr_quantum, MLFQConfig *mlfq_config, Process *processes,
                           int n, const char *source_label)
{
    MetricsSummary rows[5];
    const char *algs[] = {"FCFS", "SJF", "STCF", "RR", "MLFQ"};
    trace_set_quiet(1);

    for (int i = 0; i < 5; i++)
    {
        process_reset_all(processes, n);
        state->current_time = 0;
        if (run_algorithm(state, algs[i], rr_quantum, mlfq_config) != 0)
        {
            trace_set_quiet(0);
            trace_free();
            return 0;
        }
        compute_metrics_summary(processes, n, &rows[i]);
    }

    trace_set_quiet(0);
    print_compare_table(source_label, rr_quantum, rows);
    trace_free();
    return 1;
}
