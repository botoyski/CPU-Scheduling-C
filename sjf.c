#include <stdio.h>
#include <limits.h>
#include "scheduler.h"
#include "trace.h"

/*
Shortest Job First
Non-preemptive
Select job with smallest burst time among arrived processes

At each decision point, choose smallest burst among ready processes
*/

int schedule_sjf(SchedulerState *state)
{
    Process *p = state->processes;
    int n = state->num_processes;

    void (*trace_reset_fn)(void) = state->trace_reset_fn ? state->trace_reset_fn : trace_reset;
    int (*trace_add_segment_fn)(int, int, int) = state->trace_add_segment_fn ? state->trace_add_segment_fn : trace_add_segment;
    void (*trace_set_context_switches_fn)(int) = state->trace_set_context_switches_fn ? state->trace_set_context_switches_fn : trace_set_context_switches;

    trace_reset_fn();

    int completed = 0;
    int time = 0;

    int visited[n];

    for (int i = 0; i < n; i++)
        visited[i] = 0;

    while (completed < n)
    {
        int idx = -1;
        int min_burst = INT_MAX;

        /* find shortest available job */
        for (int i = 0; i < n; i++)
        {
            if (p[i].arrival_time <= time && !visited[i])
            {
                if (p[i].burst_time < min_burst)
                {
                    min_burst = p[i].burst_time;
                    idx = i;
                }
            }
        }

        /* if no job available */
        if (idx == -1)
        {
            time++;
            continue;
        }

        p[idx].start_time = time;
        int start = time;

        time += p[idx].burst_time;
        if (!trace_add_segment_fn(idx, start, time))
        {
            fprintf(stderr, "Error: memory allocation failed in SJF trace.\n");
            return -1;
        }

        p[idx].finish_time = time;

        p[idx].turnaround_time =
            p[idx].finish_time - p[idx].arrival_time;

        p[idx].waiting_time =
            p[idx].turnaround_time - p[idx].burst_time;

        p[idx].response_time =
            p[idx].start_time - p[idx].arrival_time;

        visited[idx] = 1;
        completed++;
    }

    trace_set_context_switches_fn(0);
    state->current_time = time;
    return 0;
}