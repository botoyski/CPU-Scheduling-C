#include <stdio.h>
#include <stdlib.h>

#include "scheduler.h"
#include "trace.h"
#include "queue.h"

int schedule_rr(SchedulerState *state, int quantum)
{
    Process *p = state->processes;
    int n = state->num_processes;

    void (*trace_reset_fn)(void) = state->trace_reset_fn ? state->trace_reset_fn : trace_reset;
    int (*trace_add_segment_fn)(int, int, int) = state->trace_add_segment_fn ? state->trace_add_segment_fn : trace_add_segment;
    void (*trace_set_context_switches_fn)(int) = state->trace_set_context_switches_fn ? state->trace_set_context_switches_fn : trace_set_context_switches;

    trace_reset_fn();

    int completed = 0;
    int time = 0;
    int context_switches = 0;
    int prev_pid = -1;

    int result = -1;
    int *arrived = (int *)calloc((size_t)n, sizeof(int));
    if (arrived == NULL) {
        fprintf(stderr, "Error: memory allocation failed in RR scheduler.\n");
        goto cleanup;
    }

    IntQueue ready_queue;
    int queue_initialized = 0;
    if (!queue_init(&ready_queue, n + 1))
    {
        fprintf(stderr, "Error: memory allocation failed in RR scheduler.\n");
        goto cleanup;
    }
    queue_initialized = 1;

    while (completed < n)
    {
        for (int i = 0; i < n; i++)
        {
            if (!arrived[i] && p[i].arrival_time <= time)
            {
                if (!queue_push(&ready_queue, i)) {
                    fprintf(stderr, "Error: queue overflow in RR scheduler.\n");
                    goto cleanup;
                }
                arrived[i] = 1;
            }
        }

        if (queue_empty(&ready_queue))
        {
            time++;
            continue;
        }

        int idx;
        if (!queue_pop(&ready_queue, &idx)) {
            fprintf(stderr, "Error: queue underflow in RR scheduler.\n");
            goto cleanup;
        }

        if (prev_pid != -1 && prev_pid != idx)
            context_switches++;

        if (!p[idx].started)
        {
            p[idx].start_time = time;
            p[idx].response_time = time - p[idx].arrival_time;
            p[idx].started = 1;
        }

        int run_for = quantum;
        if (p[idx].remaining_time < run_for)
            run_for = p[idx].remaining_time;

        int segment_start = time;

        for (int tick = 0; tick < run_for; tick++)
        {
            p[idx].remaining_time--;
            time++;

            for (int i = 0; i < n; i++)
            {
                if (!arrived[i] && p[i].arrival_time <= time)
                {
                    if (!queue_push(&ready_queue, i)) {
                        fprintf(stderr, "Error: queue overflow in RR scheduler.\n");
                        goto cleanup;
                    }
                    arrived[i] = 1;
                }
            }

            if (p[idx].remaining_time == 0)
                break;
        }

        if (!trace_add_segment_fn(idx, segment_start, time))
        {
            fprintf(stderr, "Error: memory allocation failed while recording RR Gantt chart.\n");
            goto cleanup;
        }

        if (p[idx].remaining_time == 0)
        {
            completed++;
            p[idx].finish_time = time;
            p[idx].turnaround_time = p[idx].finish_time - p[idx].arrival_time;
            p[idx].waiting_time = p[idx].turnaround_time - p[idx].burst_time;
        }
        else
        {
            if (!queue_push(&ready_queue, idx)) {
                fprintf(stderr, "Error: queue overflow in RR scheduler.\n");
                goto cleanup;
            }
        }

        prev_pid = idx;
    }

    trace_set_context_switches_fn(context_switches);
    state->current_time = time;
    result = 0;

cleanup:
    free(arrived);
    if (queue_initialized)
        queue_free(&ready_queue);
    return result;
}
