#include <stdio.h>
#include <limits.h>
#include "scheduler.h"
#include "gantt.h"

/*
Shortest Time to Completion First
Preemptive version of SJF
Every time unit find process with smallest remaining time

At every clock tick, scheduler checks which process has smallest remaining CPU time
*/

int schedule_stcf(SchedulerState *state)
{
    Process *p = state->processes;
    int n = state->num_processes;

    void (*trace_reset_fn)(void) = state->trace_reset_fn ? state->trace_reset_fn : trace_reset;
    int (*trace_add_segment_fn)(int, int, int) = state->trace_add_segment_fn ? state->trace_add_segment_fn : trace_add_segment;
    void (*trace_set_context_switches_fn)(int) = state->trace_set_context_switches_fn ? state->trace_set_context_switches_fn : trace_set_context_switches;

    trace_reset_fn();

    int time = 0;
    int completed = 0;
    int current_pid = -1;
    int segment_start = -1;
    int context_switches = 0;

    while (completed < n)
    {
        int idx = -1;
        int min_remaining = INT_MAX;

        /* find shortest remaining job */
        for (int i = 0; i < n; i++)
        {
            if (p[i].arrival_time <= time &&
                p[i].remaining_time > 0)
            {
                if (p[i].remaining_time < min_remaining)
                {
                    min_remaining = p[i].remaining_time;
                    idx = i;
                }
            }
        }

        /* CPU idle */
        if (idx == -1)
        {
            if (current_pid != -1)
            {
                if (!trace_add_segment_fn(current_pid, segment_start, time))
                {
                    fprintf(stderr, "Error: memory allocation failed in STCF trace.\n");
                    return -1;
                }
                current_pid = -1;
                segment_start = -1;
            }
            time++;
            continue;
        }

        if (current_pid != idx)
        {
            if (current_pid != -1)
            {
                if (!trace_add_segment_fn(current_pid, segment_start, time))
                {
                    fprintf(stderr, "Error: memory allocation failed in STCF trace.\n");
                    return -1;
                }
                context_switches++;
            }
            current_pid = idx;
            segment_start = time;
        }

        /* first execution */
        if (!p[idx].started)
        {
            p[idx].start_time = time;
            p[idx].response_time =
                time - p[idx].arrival_time;

            p[idx].started = 1;
        }

        /* execute 1 time unit */
        p[idx].remaining_time--;

        time++;

        /* process finished */
        if (p[idx].remaining_time == 0)
        {
            completed++;

            if (!trace_add_segment_fn(idx, segment_start, time))
            {
                fprintf(stderr, "Error: memory allocation failed in STCF trace.\n");
                return -1;
            }
            current_pid = -1;
            segment_start = -1;

            p[idx].finish_time = time;

            p[idx].turnaround_time =
                p[idx].finish_time - p[idx].arrival_time;

            p[idx].waiting_time =
                p[idx].turnaround_time - p[idx].burst_time;
        }
    }

    trace_set_context_switches_fn(context_switches);
    state->current_time = time;
    return 0;
}