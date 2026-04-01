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
    int verbose = scheduler_is_verbose(state);
    SchedulerTraceOps trace;
    scheduler_get_trace_ops(state, &trace);

    trace.reset();

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
        scheduler_mark_process_started(&p[idx], time);
        if (verbose)
            printf("t=%d: Process %s selected (FCFS)\n", time, p[idx].pid);

        /* Execute full burst */
        // record execution segment for Gantt chart
        int start = time;
        time += p[idx].burst_time;
        // add segment to trace, merging with previous if same process and contiguous
        if (!trace.add_segment(idx, start, time))
        {
            fprintf(stderr, "Error: memory allocation failed in FCFS trace.\n");
            return -1;
        }

        // 
        /* Process completes */
        scheduler_mark_process_completed(&p[idx], time);
        if (verbose)
            printf("t=%d: Process %s completes\n", time, p[idx].pid);

        done[idx] = 1;
        completed++;
    }

    // FCFS is non-preemptive, so context switches = 0
    trace.set_context_switches(0);
    state->current_time = time;
    return 0;
}