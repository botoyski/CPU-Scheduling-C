#include <stdio.h>
#include <stdlib.h>

#include "scheduler.h"
#include "trace.h"
#include "queue.h"

static int highest_non_empty_queue(int levels, IntQueue queues[])
{
    for (int lvl = 0; lvl < levels; lvl++)
    {
        if (!queue_empty(&queues[lvl]))
            return lvl;
    }
    return -1;
}

static void cleanup_mlfq_resources(IntQueue queues[], int levels, int *arrived, int *completed)
{
    if (queues != NULL)
    {
        for (int lvl = 0; lvl < levels; lvl++)
            queue_free(&queues[lvl]);
        free(queues);
    }
    free(arrived);
    free(completed);
}

static int enqueue_new_arrivals(Process p[], int n, int time, int arrived[], IntQueue *q0, int (*trace_is_quiet_fn)(void))
{
    for (int i = 0; i < n; i++)
    {
        if (!arrived[i] && p[i].arrival_time <= time)
        {
            p[i].priority = 0;
            p[i].time_in_queue = 0;
            if (!queue_push(q0, i)) {
                fprintf(stderr, "Error: queue overflow in enqueue_new_arrivals.\n");
                return 0;
            }
            arrived[i] = 1;
            int quiet = trace_is_quiet_fn ? trace_is_quiet_fn() : trace_is_quiet();
            if (!quiet)
                printf("t=%d: Process %s enters Q0\n", p[i].arrival_time, p[i].pid);
        }
    }
    return 1;
}

static int do_priority_boost(Process p[], int n, int completed[], int levels, IntQueue queues[], int time, int (*trace_is_quiet_fn)(void))
{
    for (int lvl = 1; lvl < levels; lvl++)
    {
        while (!queue_empty(&queues[lvl]))
        {
            int idx;
            if (!queue_pop(&queues[lvl], &idx)) {
                fprintf(stderr, "Error: queue underflow in MLFQ boost.\n");
                return 0;
            }
            if (!completed[idx])
            {
                p[idx].priority = 0;
                p[idx].time_in_queue = 0;
                if (!queue_push(&queues[0], idx)) {
                    fprintf(stderr, "Error: queue overflow in MLFQ boost.\n");
                    return 0;
                }
            }
        }
    }

    /*
    Snapshot queue size so we only rotate jobs that were already in Q0
    before this boost. This avoids reprocessing entries that are enqueued
    during this loop.
    */
    int q0_size = queues[0].size;
    for (int i = 0; i < q0_size; i++)
    {
        int idx;
        if (!queue_pop(&queues[0], &idx)) {
            fprintf(stderr, "Error: queue underflow in MLFQ boost.\n");
            return 0;
        }
        if (!completed[idx])
        {
            p[idx].priority = 0;
            p[idx].time_in_queue = 0;
            if (!queue_push(&queues[0], idx)) {
                fprintf(stderr, "Error: queue overflow in MLFQ boost.\n");
                return 0;
            }
        }
    }

    int quiet = trace_is_quiet_fn ? trace_is_quiet_fn() : trace_is_quiet();
    if (!quiet)
        printf("t=%d: Priority boost: all ready processes -> Q0\n", time);

    return 1;
}

int schedule_mlfq(SchedulerState *state, MLFQConfig *config)
{
    if (state == NULL || config == NULL || state->processes == NULL)
    {
        fprintf(stderr, "Error: invalid scheduler state/config for MLFQ.\n");
        return -1;
    }

    if (config->levels <= 0 || config->levels > MLFQ_MAX_LEVELS)
    {
        fprintf(stderr, "Error: invalid MLFQ levels=%d.\n", config->levels);
        return -1;
    }

    if (config->boost_period <= 0)
    {
        fprintf(stderr, "Error: invalid MLFQ boost period=%d.\n", config->boost_period);
        return -1;
    }

    Process *p = state->processes;
    int n = state->num_processes;

    void (*trace_reset_fn)(void) = state->trace_reset_fn ? state->trace_reset_fn : trace_reset;
    int (*trace_add_segment_fn)(int, int, int) = state->trace_add_segment_fn ? state->trace_add_segment_fn : trace_add_segment;
    void (*trace_set_context_switches_fn)(int) = state->trace_set_context_switches_fn ? state->trace_set_context_switches_fn : trace_set_context_switches;
    int (*trace_is_quiet_fn)(void) = state->trace_is_quiet_fn ? state->trace_is_quiet_fn : trace_is_quiet;

    trace_reset_fn();

    int completed_count = 0;
    int time = 0;
    int context_switches = 0;
    int prev_pid = -1;

    int *arrived = (int *)calloc((size_t)n, sizeof(int));
    int *completed = (int *)calloc((size_t)n, sizeof(int));

    IntQueue *queues = (IntQueue *)malloc((size_t)config->levels * sizeof(IntQueue));
    if (arrived == NULL || completed == NULL || queues == NULL)
    {
        fprintf(stderr, "Error: memory allocation failed in MLFQ scheduler.\n");
        free(queues);
        free(arrived);
        free(completed);
        return -1;
    }

    for (int lvl = 0; lvl < config->levels; lvl++)
    {
        if (!queue_init(&queues[lvl], n + 1))
        {
            fprintf(stderr, "Error: memory allocation failed in MLFQ queue setup.\n");
            cleanup_mlfq_resources(queues, lvl, arrived, completed);
            return -1;
        }
    }

    int next_boost = config->boost_period;

    if (!trace_is_quiet_fn())
    {
        printf("\n=== MLFQ Configuration ===\n");
        for (int lvl = 0; lvl < config->levels; lvl++)
        {
            if (config->quantum[lvl] == -1)
                printf("Queue %d: FCFS", lvl);
            else
                printf("Queue %d: q=%d", lvl, config->quantum[lvl]);

            if (config->allotment[lvl] != -1)
                printf(", allotment=%d", config->allotment[lvl]);

            if (lvl == 0)
                printf(" (highest priority)");
            if (lvl == config->levels - 1)
                printf(" (lowest priority)");

            printf("\n");
        }
        printf("Boost period: %d\n", config->boost_period);
        printf("\n=== Execution Trace ===\n");
    }

    while (completed_count < n)
    {
        if (!enqueue_new_arrivals(p, n, time, arrived, &queues[0], trace_is_quiet_fn))
        {
            cleanup_mlfq_resources(queues, config->levels, arrived, completed);
            return -1;
        }

        int level = highest_non_empty_queue(config->levels, queues);
        if (level == -1)
        {
            time++;
            if (config->boost_period > 0 && time == next_boost)
            {
                if (!do_priority_boost(p, n, completed, config->levels, queues, time, trace_is_quiet_fn))
                {
                    cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                    return -1;
                }
                next_boost += config->boost_period;
            }
            continue;
        }

        int idx;
        if (!queue_pop(&queues[level], &idx)) {
            fprintf(stderr, "Error: queue underflow in MLFQ main loop.\n");
            cleanup_mlfq_resources(queues, config->levels, arrived, completed);
            return -1;
        }
        if (completed[idx])
            continue;

        if (prev_pid != -1 && prev_pid != idx)
            context_switches++;

        if (!p[idx].started)
        {
            p[idx].start_time = time;
            p[idx].response_time = time - p[idx].arrival_time;
            p[idx].started = 1;
        }

        if (config->allotment[level] != -1 && p[idx].time_in_queue >= config->allotment[level])
        {
            if (level < config->levels - 1)
            {
                p[idx].priority = level + 1;
                p[idx].time_in_queue = 0;
                if (!queue_push(&queues[level + 1], idx)) {
                    fprintf(stderr, "Error: queue overflow in MLFQ demotion.\n");
                    cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                    return -1;
                }
                  if (!trace_is_quiet_fn())
                      printf("t=%d: Process %s -> Q%d (exhausted Q%d allotment)\n",
                          time,
                          p[idx].pid,
                          level + 1,
                          level);
            }
            else
            {
                if (!queue_push(&queues[level], idx)) {
                    fprintf(stderr, "Error: queue overflow in MLFQ.\n");
                    cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                    return -1;
                }
            }
            prev_pid = idx;
            continue;
        }

        int run_for;
        if (config->quantum[level] == -1)
            run_for = p[idx].remaining_time;
        else
            run_for = config->quantum[level] < p[idx].remaining_time ? config->quantum[level] : p[idx].remaining_time;

        if (config->allotment[level] != -1)
        {
            int remaining_allot = config->allotment[level] - p[idx].time_in_queue;
            if (remaining_allot < run_for)
                run_for = remaining_allot;
        }

        if (config->boost_period > 0)
        {
            int to_boost = next_boost - time;
            if (to_boost > 0 && to_boost < run_for)
                run_for = to_boost;
        }

        if (run_for <= 0)
            run_for = 1;

        int segment_start = time;

        for (int tick = 0; tick < run_for; tick++)
        {
            p[idx].remaining_time--;
            p[idx].time_in_queue++;
            time++;

            if (!enqueue_new_arrivals(p, n, time, arrived, &queues[0], trace_is_quiet_fn))
            {
                cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                return -1;
            }

            if (p[idx].remaining_time == 0)
                break;

            if (config->boost_period > 0 && time == next_boost)
                break;
        }

        if (!trace_add_segment_fn(idx, segment_start, time))
        {
            fprintf(stderr, "Error: memory allocation failed while recording MLFQ Gantt chart.\n");
            cleanup_mlfq_resources(queues, config->levels, arrived, completed);
            return -1;
        }

        if (p[idx].remaining_time == 0)
        {
            completed[idx] = 1;
            completed_count++;
            p[idx].finish_time = time;
            p[idx].turnaround_time = p[idx].finish_time - p[idx].arrival_time;
            p[idx].waiting_time = p[idx].turnaround_time - p[idx].burst_time;
            if (!trace_is_quiet_fn())
                printf("t=%d: Process %s completes\n", time, p[idx].pid);
        }
        else if (config->boost_period > 0 && time == next_boost)
        {
            p[idx].priority = 0;
            p[idx].time_in_queue = 0;
            if (!queue_push(&queues[0], idx)) {
                fprintf(stderr, "Error: queue overflow in MLFQ boost.\n");
                cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                return -1;
            }
            if (!do_priority_boost(p, n, completed, config->levels, queues, time, trace_is_quiet_fn))
            {
                cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                return -1;
            }
            next_boost += config->boost_period;
        }
        else
        {
            int demoted = 0;
            if (config->allotment[level] != -1 && p[idx].time_in_queue >= config->allotment[level])
            {
                if (level < config->levels - 1)
                {
                    p[idx].priority = level + 1;
                    p[idx].time_in_queue = 0;
                    if (!queue_push(&queues[level + 1], idx)) {
                        fprintf(stderr, "Error: queue overflow in MLFQ demotion.\n");
                        cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                        return -1;
                    }
                    if (!trace_is_quiet_fn())
                        printf("t=%d: Process %s -> Q%d (exhausted Q%d allotment)\n",
                               time,
                               p[idx].pid,
                               level + 1,
                               level);
                    demoted = 1;
                }
            }

            if (!demoted) {
                if (!queue_push(&queues[level], idx)) {
                    fprintf(stderr, "Error: queue overflow in MLFQ.\n");
                    cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                    return -1;
                }
            }
        }

        prev_pid = idx;
    }

    trace_set_context_switches_fn(context_switches);
    state->current_time = time;

    cleanup_mlfq_resources(queues, config->levels, arrived, completed);
    return 0;
}
