#include <stdio.h>
#include "scheduler.h"
#include "trace.h"

/*
FCFS Scheduling
First process that arrives gets CPU first
Non-preemptive

executes: arrival order → run until completion
*/

int schedule_fcfs(SchedulerState *state)
{
    Process *p = state->processes;
    int n = state->num_processes;

    trace_reset();

    int completed = 0;
    int time = 0;
    int done[n];

    for (int i = 0; i < n; i++)
        done[i] = 0;

    while (completed < n)
    {
        int idx = -1;
        int min_arrival = 2147483647;

        for (int i = 0; i < n; i++)
        {
            if (!done[i] && p[i].arrival_time <= time)
            {
                if (p[i].arrival_time < min_arrival)
                {
                    min_arrival = p[i].arrival_time;
                    idx = i;
                }
            }
        }

        if (idx == -1)
        {
            int next_arrival = 2147483647;
            for (int i = 0; i < n; i++)
            {
                if (!done[i] && p[i].arrival_time < next_arrival)
                    next_arrival = p[i].arrival_time;
            }
            time = next_arrival;
            continue;
        }

        /* Process starts */
        p[idx].start_time = time;
        p[idx].response_time = p[idx].start_time - p[idx].arrival_time;
        p[idx].started = 1;

        /* Execute full burst */
        int start = time;
        time += p[idx].burst_time;
        if (!trace_add_segment(idx, start, time))
        {
            fprintf(stderr, "Error: memory allocation failed in FCFS trace.\n");
            return -1;
        }

        /* Process completes */
        p[idx].finish_time = time;

        /* Metrics */
        p[idx].turnaround_time = p[idx].finish_time - p[idx].arrival_time;
        p[idx].waiting_time = p[idx].turnaround_time - p[idx].burst_time;

        done[idx] = 1;
        completed++;
    }

    trace_set_context_switches(0);
    state->current_time = time;
    return 0;
}