#include <stdio.h>
#include "scheduler.h"
#include "gantt.h"

/*
FCFS Scheduling
First process that arrives gets CPU first
Non-preemptive

executes: arrival order → run until completion
*/

// 
int schedule_fcfs(SchedulerState *state)
{
    Process *p = state->processes;
    int n = state->num_processes;

    // use provided trace functions if available, otherwise default to gantt implementations
    void (*trace_reset_fn)(void) = state->trace_reset_fn ? state->trace_reset_fn : trace_reset;
    int (*trace_add_segment_fn)(int, int, int) = state->trace_add_segment_fn ? state->trace_add_segment_fn : trace_add_segment;
    void (*trace_set_context_switches_fn)(int) = state->trace_set_context_switches_fn ? state->trace_set_context_switches_fn : trace_set_context_switches;

    trace_reset_fn();

    int completed = 0;
    int time = 0;
    int done[n];

    // initialize done array to track completed processes
    for (int i = 0; i < n; i++)
        done[i] = 0;

    // continue until all processes are completed
    while (completed < n)
    {
        int idx = -1;
        int min_arrival = 2147483647;

        // find next process to schedule (earliest arrival time among not done)
        for (int i = 0; i < n; i++)
        {
            // if process i is not done and has arrived, check if it has the earliest arrival time
            if (!done[i] && p[i].arrival_time <= time)
            {
                // if this process arrives earlier than the current earliest, select it
                if (p[i].arrival_time < min_arrival)
                {
                    min_arrival = p[i].arrival_time;
                    idx = i;
                }
            }
        }

        // if no process is ready, advance time to next arrival
        if (idx == -1)
        {
            // find the next arrival time among not done processes
            int next_arrival = 2147483647;
            for (int i = 0; i < n; i++)
            {
                // if process i is not done and arrives earlier than the current next arrival, update next_arrival
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
        // record execution segment for Gantt chart
        int start = time;
        time += p[idx].burst_time;
        // add segment to trace, merging with previous if same process and contiguous
        if (!trace_add_segment_fn(idx, start, time))
        {
            fprintf(stderr, "Error: memory allocation failed in FCFS trace.\n");
            return -1;
        }

        // 
        /* Process completes */
        p[idx].finish_time = time;

        /* Metrics */
        p[idx].turnaround_time = p[idx].finish_time - p[idx].arrival_time;
        p[idx].waiting_time = p[idx].turnaround_time - p[idx].burst_time;

        done[idx] = 1;
        completed++;
    }

    // FCFS is non-preemptive, so context switches = 0
    trace_set_context_switches_fn(0);
    state->current_time = time;
    return 0;
}